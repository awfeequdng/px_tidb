#include "parser/lexer.hh"

#include <gtest/gtest.h>

#include "parser/misc.hh"

TEST(TestLexer, test_scan) {
    auto token_map = parser::getTokenMap();
    for (auto [str, tok] : token_map) {
        auto lexer = parser::NewScanner(str);
    }
}