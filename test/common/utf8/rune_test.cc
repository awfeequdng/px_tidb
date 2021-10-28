#include "common/utf8/rune.hh"

#include <gtest/gtest.h>

TEST(RuneTest, utf8) {
    common::utf8::rune_t rune{0x4e2d};
    //    EXPECT_EQ(encode(rune).width, 3);
    //    std::cout << encode(rune).data[0] << std::endl;
    std::string str = (std::string)rune;
    EXPECT_EQ(str, "中");
}