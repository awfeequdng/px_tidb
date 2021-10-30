#include "parser/lexer.hh"

#include "parser/misc.hh"
#include "parser/token.hh"

namespace parser {
std::tuple<int, common::utf8::Pos, std::string> startWithAt(Scanner &s) { return {}; }

std::tuple<int, common::utf8::Pos, std::string> startWithSlash(Scanner &s) {
    auto pos = s.reader()->pos();
    s.reader()->next();
    if (s.reader()->peek() != '*') {
        auto tok = int('/');
        auto lit = "/";
        return {tok, pos, lit};
    }

    auto isOptimizerHint = false;
    auto currentCharIsStar = false;
    s.reader()->next();  // we see '/*' so far.

    switch ((int)s.reader()->next()) {
        case '!':  // '/*!' MySQL-specific comments
            // See http://dev.mysql.com/doc/refman/5.7/en/comments.html
            // in '/*!', which we always recognize regardless of version.
            s.scanVersionDigits(5, 5);
            s._inBangComment = true;
            return s.scan();
            //  todo: not implemented yet
            //        case 'T': // '/*T' maybe TiDB-specific comments
            //            if (s.reader()->peek() != '!') {
            //                // '/*TX' is just normal comment.
            //                break;
            //            }
            //            s.reader()->next();
            //            // in '/*T!', try to match the pattern '/*T![feature1,feature2,...]'.
            //            auto features = s.scanFeatureIDs();
            //            if (tidbfeature.CanParseFeature(features...)) {
            //                s._inBangComment = true;
            //                return s.scan();
            //            }
            //            break;

        case 'M':  // '/*M' maybe MariaDB-specific comments
            // no special treatment for now.
            break;
            //  todo: not implemented yet
            //        case '+': // '/*+' optimizer hints
            //            // See https://dev.mysql.com/doc/refman/5.7/en/optimizer-hints.html
            //            if _, ok := hintedTokens[s.lastKeyword]; ok {
            //                // only recognize optimizers hints directly followed by certain
            //                // keywords like SELECT, INSERT, etc., only a special case "FOR UPDATE" needs to be
            //                handled
            //                // we will report a warning in order to match MySQL's behavior, but the hint content will
            //                be ignored if s.lastKeyword2 == forKwd {
            //                    if s.lastKeyword3 == binding {
            //                                // special case of `create binding for update`
            //                                isOptimizerHint = true
            //                        } else {
            //                        s.warns = append(s.warns, ParseErrorWith(s.r.data(&pos), s.r.p.Line))
            //                    }
            //                } else {
            //                    isOptimizerHint = true
            //                }
            //        }

        case '*':  // '/**' if the next char is '/' it would close the comment.
            currentCharIsStar = true;
        default:
            break;
    }

    // standard C-like comment. read until we see '*/' then drop it.
    while (true) {
        auto fn = [](common::utf8::rune_t ch) -> bool { return ch != '*'; };
        if (currentCharIsStar || s.reader()->incAsLongAs(fn) == '*') {
            switch ((int)s.reader()->next()) {
                case '/':
                    // Meets */, means comment end.
                    if (isOptimizerHint) {
                        s._lastHintPos = pos;
                        return {tok_hintComment, pos, s.reader()->data(pos)};
                    } else {
                        return s.scan();
                    }
                case '*':
                    currentCharIsStar = true;
                    continue;
                default:
                    currentCharIsStar = false;
                    continue;
            }
        }
        // unclosed comment or other errors.
        // todo: add some error information here
        return {};
    }
}
std::tuple<int, common::utf8::Pos, std::string> startWithStar(Scanner &s) { return {}; }
std::tuple<int, common::utf8::Pos, std::string> startWithDash(Scanner &s) { return {}; }
std::tuple<int, common::utf8::Pos, std::string> startWithSharp(Scanner &s) { return {}; }
std::tuple<int, common::utf8::Pos, std::string> startWithXx(Scanner &s) { return {}; }
std::tuple<int, common::utf8::Pos, std::string> startWithNn(Scanner &s) { return {}; }
std::tuple<int, common::utf8::Pos, std::string> startWithBb(Scanner &s) { return {}; }
std::tuple<int, common::utf8::Pos, std::string> startWithDot(Scanner &s) { return {}; }

std::tuple<int, common::utf8::Pos, std::string> scanIdentifier(Scanner &scanner) {
    auto pos = scanner.reader()->pos();
    scanner.reader()->incAsLongAs(isIdentChar);
    return {tok_identifier, pos, scanner.reader()->data(pos)};
}
std::tuple<int, common::utf8::Pos, std::string> scanQuotedIdent(Scanner &s) { return {}; }

std::tuple<int, common::utf8::Pos, std::string> startWithNumber(Scanner &s) {
    if (s._identifierDot) {
        return scanIdentifier(s);
    }
    auto pos = s.reader()->pos();
    auto tok = tok_intLit;
    auto ch0 = s.reader()->next();

    if (ch0 == '0') {
        tok = tok_intLit;
        auto ch1 = s.reader()->peek();
        if (ch1 >= '0' && ch1 <= '7') {
            s.reader()->next();
            s.scanOct();
        } else if (ch1 == 'x' || ch1 == 'X') {
            s.reader()->next();
            auto p1 = s.reader()->pos();
            s.scanHex();
            auto p2 = s.reader()->pos();
            // 0x, 0x7fz3 are identifier
            if (p1 == p2 || isDigit(s.reader()->peek())) {
                s.reader()->incAsLongAs(isIdentChar);
                return {tok_identifier, pos, s.reader()->data(pos)};
            }
            tok = tok_hexLit;
        } else if (ch1 == 'b') {
            s.reader()->next();
            auto p1 = s.reader()->pos();
            s.scanBit();
            auto p2 = s.reader()->pos();
            // 0b, 0b123, 0b1ab are identifier
            if (p1 == p2 || isDigit(s.reader()->peek())) {
                s.reader()->incAsLongAs(isIdentChar);
                return {tok_identifier, pos, s.reader()->data(pos)};
            }
            tok = tok_bitLit;
        } else if (ch1 == '.') {
            return s.scanFloat(pos);
        } else if (ch1 == 'B') {
            s.reader()->incAsLongAs(isIdentChar);
            return {tok_identifier, pos, s.reader()->data(pos)};
        }
    }

    s.scanDigits();
    ch0 = s.reader()->peek();
    if (ch0 == '.' || ch0 == 'e' || ch0 == 'E') {
        return s.scanFloat(pos);
    }

    // Identifiers may begin with a digit but unless quoted may not consist solely of digits.
    if (!s.reader()->eof() && isIdentChar(ch0)) {
        s.reader()->incAsLongAs(isIdentChar);
        return {tok_identifier, pos, s.reader()->data(pos)};
    }
    auto lit = s.reader()->data(pos);
    return {tok, pos, lit};
}
std::tuple<int, common::utf8::Pos, std::string> startString(Scanner &s) { return s.scanString(); }
}  // namespace parser