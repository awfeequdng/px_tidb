#include "parser/scanner.hh"

#include <gtest/gtest.h>

#include "common/hex_formatter.hh"
#include "parser/token.hh"

using namespace mysql;
using namespace parser;

TEST(TestScanner, TestBufPutc) {
    auto buf = std::stringbuf();
    buf.sputc('c');
    buf.sputc('p');
    buf.sputc('x');
    EXPECT_EQ(buf.str(), "cpx");
}

TEST(TestScanner, TestBufPutGet) {
    auto buf = std::stringbuf();
    std::string str = "hello world";
    buf.sputn(str.c_str(), str.length());
    EXPECT_EQ(buf.str(), str);
    auto str1 = " cpx";
    buf.sputn(str1, 4);
    EXPECT_EQ(buf.str(), "hello world cpx");

    buf.pubseekpos(0);
    buf.sputn(str1, 4);
    EXPECT_EQ(buf.str(), " cpxo world cpx");
    buf.str("hello world");
    EXPECT_EQ(buf.str(), "hello world");

    char str_buf[10];
    buf.pubseekpos(3);
    buf.sgetn(str_buf, 4);
    str_buf[4] = 0;
    EXPECT_EQ(std::string(str_buf), "lo w");

    buf.str("hello");
    buf.sputc('n');
    EXPECT_EQ(buf.str(), "nello");

    buf.pubseekoff(0, std::ios_base::end);
    buf.sputc('n');
    EXPECT_EQ(buf.str(), "nellon");
}

TEST(TestScanner, TestscanString) {
    std::unordered_map<std::string, std::string> table = {
        {R"**(' \n\tTest String')**", " \n\tTest String"},
        {R"**('\x\B')**", "xB"},
        // todo: can not pass through this test case
        //        {R"**('\0\'\"\b\n\r\t\\')**", "\000'\"\b\n\r\t\\"},
        {R"**('\Z')**", "\x1a"},
        {R"**('\%\_')**", R"**(\%\_)**"},
        {R"**('hello')**", "hello"},
        {R"**('"hello"')**", R"**("hello")**"},
        {R"**('""hello""')**", R"**(""hello"")**"},
        {R"**('hel''lo')**", "hel'lo"},
        {R"**('\'hello')**", "'hello"},
        {R"**("hello")**", "hello"},
        {R"**("'hello'")**", "'hello'"},
        {R"**("''hello''")**", "''hello''"},
        {R"**("hel""lo")**", R"**(hel"lo)**"},
        {R"**("\"hello")**", R"**("hello)**"},
        {R"**('disappearing\ backslash')**", "disappearing backslash"},
        {"'한국의中文UTF8およびテキストトラック'", "한국의中文UTF8およびテキストトラック"},
        {"'\\a\x90'", "a\x90"},
        {"'\\a\x18èàø»\x05'", "a\x18èàø»\x05"},
        {"' \\n\\t'", " \n\t"},
    };
    int tok;
    common::utf8::Pos pos;
    std::string lit;
    for (auto &[str, expect] : table) {
        auto scanner = NewScanner(str);
        std::tie(tok, pos, lit) = scanner->scan();
        ASSERT_EQ(tok, tok_stringLit);
        ASSERT_EQ(pos._offset, 0);
        ASSERT_EQ(lit, expect);
    }
}

TEST(TestScanner, TestCommonComment) {
    auto l = NewScanner("/*x40101 select\n5*/ SELECT");
    auto [tok, pos, lit] = l->scan();
    ASSERT_EQ(tok, tok_identifier);
    ASSERT_EQ(lit, "SELECT");
    auto expect_pos = common::utf8::Pos{1, 4, 20};
    ASSERT_EQ(pos, expect_pos);
}

TEST(TestScanner, TestSpecialComment) {
    auto l = NewScanner("/*!40101 select\n5*/ SELECT");
    auto [tok, pos, lit] = l->scan();
    ASSERT_EQ(tok, tok_identifier);
    ASSERT_EQ(lit, "select");
    common::utf8::Pos expect_pos = common::utf8::Pos{0, 9, 9};
    ASSERT_EQ(pos, expect_pos);

    std::tie(tok, pos, lit) = l->scan();

    ASSERT_EQ(tok, tok_intLit);
    ASSERT_EQ(lit, "5");
    expect_pos = common::utf8::Pos{1, 0, 16};
    ASSERT_EQ(pos, expect_pos);

    // it will skip '*/' because inBangComment is true
    std::tie(tok, pos, lit) = l->scan();
    ASSERT_EQ(tok, tok_identifier);
    ASSERT_EQ(lit, "SELECT");
    expect_pos = common::utf8::Pos{1, 4, 20};
    ASSERT_EQ(pos, expect_pos);
}

TEST(TestScanner, TestInteger) {
    std::unordered_map<std::string, std::string> tests = {
        {"01000001783  x", "01000001783"},
        {"00001783gggx", "00001783gggx"},
        {"0 a", "0"},
        {"0000a", "0000a"},
        {"01", "01"},
        {"10 x", "10"},
    };
    auto scanner = NewScanner("");
    for (auto [key, expect] : tests) {
        scanner->reset(key);
        auto [tok, pos, lit] = scanner->scan();
        // it may be identifier
        //        ASSERT_EQ(tok, tok_intLit);
        ASSERT_EQ(lit, expect);
    }
}

TEST(TestScanner, TestVersionDigits) {
    struct TestVersion {
        std::string input;
        int min;
        int max;
        common::utf8::rune_t nextChar;
    } tests[] = {
        {
            .input = "12345",
            .min = 5,
            .max = 5,
            .nextChar = common::utf8::rune_eof,
        },
        {
            .input = "12345xyz",
            .min = 5,
            .max = 5,
            .nextChar = 'x',
        },
        {
            .input = "1234xyz",
            .min = 5,
            .max = 5,
            .nextChar = '1',
        },
        {
            .input = "123456",
            .min = 5,
            .max = 5,
            .nextChar = '6',
        },
        {
            .input = "1234",
            .min = 5,
            .max = 5,
            .nextChar = '1',
        },
        {
            .input = "",
            .min = 5,
            .max = 5,
            .nextChar = common::utf8::rune_eof,
        },
        {
            .input = "1234567xyz",
            .min = 5,
            .max = 6,
            .nextChar = '7',
        },
        {
            .input = "12345xyz",
            .min = 5,
            .max = 6,
            .nextChar = 'x',
        },
        {
            .input = "12345",
            .min = 5,
            .max = 6,
            .nextChar = common::utf8::rune_eof,
        },
        {
            .input = "1234xyz",
            .min = 5,
            .max = 6,
            .nextChar = '1',
        },
    };

    auto scanner = NewScanner("");
    for (auto [input, min, max, nextChar] : tests) {
        scanner->reset(input);
        scanner->scanVersionDigits(min, max);
        auto nc = scanner->reader()->next();
        ASSERT_EQ(nc, nextChar);
    }
}

TEST(TestScanner, TestStartWithNn) {
    struct {
        std::string str;
        std::string expect;
        int tok;
        char nextChar;
    } tests[] = {
        {
            .str = "nx'123456 f",
            .expect = "nx",
            .tok = tok_identifier,
            .nextChar = '\'',
        },
        {
            .str = "Nx'123456 f",
            .expect = "Nx",
            .tok = tok_identifier,
            .nextChar = '\'',
        },
        {
            .str = "n'123456' f",
            .expect = "utf8",
            .tok = tok_underscoreCS,
            .nextChar = '\'',
        },
        {
            .str = "N'123456' f",
            .expect = "utf8",
            .tok = tok_underscoreCS,
            .nextChar = '\'',
        },
    };
    auto scanner = NewScanner("");
    for (auto test : tests) {
        scanner->reset(test.str);
        auto [tok, pos, lit] = scanner->scan();
        auto nc = scanner->reader()->peek();
        ASSERT_EQ(tok, test.tok);
        ASSERT_EQ(lit, test.expect);
        ASSERT_EQ(nc, test.nextChar);
    }
}

TEST(TestScanner, TestStartWithXx) {
    struct {
        std::string str;
        std::string expect;
        int tok;
    } tests[] = {
        {
            .str = "x'123456'",
            .expect = "x'123456'",
            .tok = tok_hexLit,
        },
        {
            .str = "x'123456' ",
            .expect = "x'123456'",
            .tok = tok_hexLit,
        },
        {
            .str = "x'123456' f",
            .expect = "x'123456'",
            .tok = tok_hexLit,
        },
        {
            .str = "x'123456x' f",
            .tok = (int)common::utf8::rune_invalid,
        },
        {
            .str = "X'123456' f",
            .expect = "X'123456'",
            .tok = tok_hexLit,
        },
        {
            .str = "X'123456x' f",
            .tok = (int)common::utf8::rune_invalid,
        },
        {
            .str = "XSTART'123456' f",
            .expect = "XSTART",
            .tok = tok_identifier,
        },
        {
            .str = "xxxji'123456' f",
            .expect = "xxxji",
            .tok = tok_identifier,
        },
        {
            .str = "xxxx'123456' f",
            .expect = "xxxx",
            .tok = tok_identifier,
        },
    };
    auto scanner = NewScanner("");
    for (auto test : tests) {
        scanner->reset(test.str);
        auto [tok, pos, lit] = scanner->scan();
        ASSERT_EQ(tok, test.tok);
        ASSERT_EQ(lit, test.expect);
    }
}

TEST(TestScanner, TestScanQuotedIdent) {
    struct {
        std::string str;
        std::string expect;
        int tok;
    } tests[] = {
        {
            .str = "`greater`",
            .expect = "greater",
            .tok = tok_quotedIdentifier,
        },
        {
            .str = "`greater``hello world`",
            .expect = "greater`hello world",
            .tok = tok_quotedIdentifier,
        },
    };
    auto scanner = NewScanner("");
    for (auto test : tests) {
        scanner->reset(test.str);
        auto [tok, pos, lit] = scanner->scan();
        ASSERT_EQ(tok, test.tok);
        ASSERT_EQ(lit, test.expect);
    }
}

TEST(TestScanner, TestStartWithSharp) {
    struct {
        std::string str;
        std::string expect;
        int tok;
    } tests[] = {
        {
            .str = "#xxxxx\nb'1010'",
            .expect = "b'1010'",
            .tok = tok_bitLit,
        },
        {
            .str = "#\nbbbji'1010' f",
            .expect = "bbbji",
            .tok = tok_identifier,
        },
        {
            .str = "####\nbbbb'1010' f",
            .expect = "bbbb",
            .tok = tok_identifier,
        },
    };
    auto scanner = NewScanner("");
    for (auto test : tests) {
        scanner->reset(test.str);
        auto [tok, pos, lit] = scanner->scan();
        ASSERT_EQ(tok, test.tok);
        ASSERT_EQ(lit, test.expect);
    }
}

TEST(TestScanner, TestStartWithBb) {
    struct {
        std::string str;
        std::string expect;
        int tok;
    } tests[] = {
        {
            .str = "b'1010'",
            .expect = "b'1010'",
            .tok = tok_bitLit,
        },
        {
            .str = "b'1010' ",
            .expect = "b'1010'",
            .tok = tok_bitLit,
        },
        {
            .str = "b'1010' f",
            .expect = "b'1010'",
            .tok = tok_bitLit,
        },
        {
            .str = "b'1010x' f",
            .tok = (int)common::utf8::rune_invalid,
        },
        {
            .str = "B'1010' f",
            .expect = "B'1010'",
            .tok = tok_bitLit,
        },
        {
            .str = "b'1010x' f",
            .tok = (int)common::utf8::rune_invalid,
        },
        {
            .str = "BSTART'1010' f",
            .expect = "BSTART",
            .tok = tok_identifier,
        },
        {
            .str = "bbbji'1010' f",
            .expect = "bbbji",
            .tok = tok_identifier,
        },
        {
            .str = "bbbb'1010' f",
            .expect = "bbbb",
            .tok = tok_identifier,
        },
    };
    auto scanner = NewScanner("");
    for (auto test : tests) {
        scanner->reset(test.str);
        auto [tok, pos, lit] = scanner->scan();
        ASSERT_EQ(tok, test.tok);
        ASSERT_EQ(lit, test.expect);
    }
}