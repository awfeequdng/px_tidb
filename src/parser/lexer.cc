#include "parser/lexer.hh"

namespace parser {
std::tuple<int, common::utf8::Pos, std::string> startWithAt(Scanner *s) { return {}; }
std::tuple<int, common::utf8::Pos, std::string> startWithSlash(Scanner *s) { return {}; }
std::tuple<int, common::utf8::Pos, std::string> startWithStar(Scanner *s) { return {}; }
std::tuple<int, common::utf8::Pos, std::string> startWithDash(Scanner *s) { return {}; }
std::tuple<int, common::utf8::Pos, std::string> startWithSharp(Scanner *s) { return {}; }
std::tuple<int, common::utf8::Pos, std::string> startWithXx(Scanner *s) { return {}; }
std::tuple<int, common::utf8::Pos, std::string> startWithNn(Scanner *s) { return {}; }
std::tuple<int, common::utf8::Pos, std::string> startWithBb(Scanner *s) { return {}; }
std::tuple<int, common::utf8::Pos, std::string> startWithDot(Scanner *s) { return {}; }
std::tuple<int, common::utf8::Pos, std::string> scanIdentifier(Scanner *s) { return {}; }
std::tuple<int, common::utf8::Pos, std::string> scanQuotedIdent(Scanner *s) { return {}; }
std::tuple<int, common::utf8::Pos, std::string> startWithNumber(Scanner *s) { return {}; }
std::tuple<int, common::utf8::Pos, std::string> startString(Scanner *s) { return {}; }
}  // namespace parser