#pragma once

#include <memory>
#include <sstream>
#include <tuple>

#include "common/utf8/reader.hh"
#include "parser/mysql/const.hh"

namespace parser {

class Scanner {
public:
    void reset(std::string sql);
    std::string stmtText() const;
    common::utf8::rune_t getNextToken();
    std::tuple<int, common::utf8::Pos, std::string> scan();
    common::utf8::rune_t skipWhitespace();
    int isTokenIdentifier(std::string lit, int offset);
    std::shared_ptr<common::utf8::reader_t> reader() { return _reader; }

    std::tuple<int, common::utf8::Pos, std::string> scanString();

    // SetSQLMode sets the SQL mode for scanner.
    void SetSQLMode(mysql::SQLMode mode) { _sqlMode = mode; }

    // GetSQLMode return the SQL mode of scanner.
    mysql::SQLMode GetSQLMode() { return _sqlMode; }

    // EnableWindowFunc controls whether the scanner recognize the keywords of window function.
    void EnableWindowFunc(bool val) { _supportWindowFunc = val; }

    // InheritScanner returns a new scanner object which inherits configurations from the parent scanner.
    std::shared_ptr<Scanner> InheritScanner(std::string sql) {
        auto scanner = std::make_shared<Scanner>();
        scanner->reset(sql);
        scanner->_sqlMode = _sqlMode;
        scanner->_supportWindowFunc = _supportWindowFunc;
        return scanner;
    }

    void scanVersionDigits(int min, int max);

    std::stringbuf &buf() { return _buf; }

    void scanOct();
    void scanHex();
    void scanBit();
    std::tuple<int, common::utf8::Pos, std::string> scanFloat(const common::utf8::Pos &beg);
    std::string scanDigits();

    // private:
    std::shared_ptr<common::utf8::reader_t> _reader;

    std::stringbuf _buf;

    int _stmtStartPos;

    // _inBangComment is true if we are inside a `/*! ... */` block.
    // It is used to ignore a stray `*/` when scanning.
    bool _inBangComment;

    mysql::SQLMode _sqlMode;
    // If the lexer should recognize keywords for window function.
    // It may break the compatibility when support those keywords,
    // because some application may already use them as identifiers.
    bool _supportWindowFunc;

    // Whether record the original text keyword position to the AST node.
    bool _skipPositionRecording;

    // lastScanOffset indicates last offset returned by scan().
    // It's used to substring sql in syntax error message.
    int _lastScanOffset;

    // lastKeyword records the previous keyword returned by scan().
    // determine whether an optimizer hint should be parsed or ignored.
    int _lastKeyword;
    // lastKeyword2 records the keyword before lastKeyword, it is used
    // to disambiguate hint after for update, which should be ignored.
    int _lastKeyword2;
    // lastKeyword3 records the keyword before lastKeyword2, it is used
    // to disambiguate hint after create binding for update, which should
    // be pertained.
    int _lastKeyword3;

    // hintPos records the start position of the previous optimizer hint.
    common::utf8::Pos _lastHintPos;

    // true if a dot follows an identifier
    bool _identifierDot;
};

std::shared_ptr<Scanner> NewScanner(std::string);
}  // namespace parser