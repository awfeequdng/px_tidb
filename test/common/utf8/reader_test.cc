#include "common/utf8/reader.hh"

#include <gtest/gtest.h>

using namespace common::utf8;

TEST(Utf8ReaderTest, read) {
    std::string str = "中国x";
    reader_t reader(str);

    auto rune = reader.next();
    EXPECT_FALSE(reader.eof());

    EXPECT_EQ((std::string)rune, "中");
    rune = reader.next();

    EXPECT_FALSE(reader.eof());
    EXPECT_EQ((std::string)rune, "国");

    EXPECT_FALSE(reader.eof());
    rune = reader.next();

    EXPECT_EQ((std::string)rune, "x");
    EXPECT_TRUE(reader.eof());
}
