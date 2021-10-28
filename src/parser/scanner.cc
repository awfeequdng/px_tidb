#include "parser/scanner.hh"

#include "parser/misc.hh"
#include "parser/token.hh"

namespace parser {

void Scanner::reset(std::string sql) {
    _reader = std::make_shared<common::utf8::reader_t>(sql);
    _stmtStartPos = 0;
    _inBangComment = false;
    _lastKeyword = 0;
}

std::string_view Scanner::stmtText() const {
    auto endIndex = _reader->index();
    std::string_view text = _reader->make_slice(_stmtStartPos, endIndex - _stmtStartPos);
    return text;
}

common::utf8::rune_t Scanner::getNextToken() { return common::utf8::rune_invalid; }

std::tuple<int, common::utf8::Pos, std::string> scanIdentifier(Scanner *scanner) {
    auto pos = scanner->reader()->pos();
    scanner->reader()->incAsLongAs(isIdentChar);
    return {tok_identifier, pos, scanner->reader()->data(pos)};
}

std::tuple<int, common::utf8::Pos, std::string> Scanner::scan() {
    auto ch0 = _reader->curr();
    if (ch0.is_space()) {
        ch0 = skipWhitespace();
    }
    auto pos = _reader->pos();
    if (_reader->eof()) {
        // when scanner meets EOF, the returned token should be 0,
        // because 0 is a special token id to remind the parser that stream is end.
        return {0, pos, ""};
    }
    if (isIdentExtend(ch0)) {
        return scanIdentifier(this);
    }

    auto node = getRuleTable();
    while (ch0 >= 0 && ch0 <= 255) {
        int id = (int)ch0;
        if (node->childs[id] == nullptr || _reader->eof()) {
            break;
        }
        node = node->childs[id];
        if (node->fn != nullptr) {
            return node->fn(this);
        }
        ch0 = _reader->next();
    }

    return {node->token, pos, _reader->data(pos)};
}

common::utf8::rune_t Scanner::skipWhitespace() {
    while (true) {
        auto rune = _reader->next();
        if (rune.is_errored()) return common::utf8::rune_invalid;
        if (rune.is_space()) continue;
        return rune;
    }
}

}  // namespace parser