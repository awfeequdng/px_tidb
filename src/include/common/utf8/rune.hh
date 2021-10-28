#pragma once

#include <cctype>
#include <cstdint>
#include <string>

#include "utf8proc.h"

namespace common::utf8 {
class Pos {
public:
    int _line;
    int _col;
    int _offset;
};

struct rune_t final {
    constexpr rune_t() = default;

    explicit constexpr rune_t(int32_t value) : _value(value) {}

    constexpr rune_t(const rune_t &rhs) : _value(rhs._value) {}

    rune_t &operator=(int32_t rhs);

    explicit operator char() const;

    bool operator==(char rhs) const;

    bool operator!=(char rhs) const;

    bool operator<(int32_t rhs) const;

    bool operator>(int32_t rhs) const;

    bool operator<=(int32_t rhs) const;

    bool operator>=(int32_t rhs) const;

    [[nodiscard]] bool is_eof() const;

    [[nodiscard]] bool is_bom() const;

    explicit operator int32_t() const;

    [[nodiscard]] bool is_digit() const;

    [[nodiscard]] bool is_alpha() const;

    [[nodiscard]] bool is_space() const;

    [[nodiscard]] bool is_alnum() const;

    [[nodiscard]] bool is_xdigit() const;

    [[nodiscard]] bool is_errored() const;

    [[nodiscard]] bool is_invalid() const;

    explicit operator std::size_t() const;

    explicit operator std::string() const;

    bool operator<(const rune_t &rhs) const;

    bool operator>(const rune_t &rhs) const;

    bool operator<=(const rune_t &rhs) const;

    bool operator>=(const rune_t &rhs) const;

    bool operator==(const rune_t &rhs) const;

    bool operator!=(const rune_t &rhs) const;

private:
    int32_t _value = 0xfffd;
};

std::ostream &operator<<(std::ostream &os, const rune_t &rune);

struct rune_hash_t {
    std::size_t operator()(const rune_t &rune) const noexcept { return static_cast<std::size_t>(rune); }
};

///////////////////////////////////////////////////////////////////////////

static constexpr rune_t rune_invalid = rune_t(0xfffd);
static constexpr rune_t rune_max = rune_t(0x0010ffff);
static constexpr rune_t rune_bom = rune_t(0xfeff);
static constexpr rune_t rune_eof = rune_t(-1);

static inline const uint8_t s_utf8_first[256] = {
    0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0,  // 0x00-0x0F
    0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0,  // 0x10-0x1F
    0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0,  // 0x20-0x2F
    0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0,  // 0x30-0x3F
    0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0,  // 0x40-0x4F
    0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0,  // 0x50-0x5F
    0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0,  // 0x60-0x6F
    0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0,  // 0x70-0x7F
    0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1,  // 0x80-0x8F
    0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1,  // 0x90-0x9F
    0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1,  // 0xA0-0xAF
    0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1,  // 0xB0-0xBF
    0xf1, 0xf1, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,  // 0xC0-0xCF
    0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,  // 0xD0-0xDF
    0x13, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x23, 0x03, 0x03,  // 0xE0-0xEF
    0x34, 0x04, 0x04, 0x04, 0x44, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1,  // 0xF0-0xFF
};

struct utf8_accept_range_t {
    uint8_t low;
    uint8_t high;
};

static inline const utf8_accept_range_t s_utf8_accept_ranges[] = {
    {.low = 0x80, .high = 0xbf}, {.low = 0xa0, .high = 0xbf}, {.low = 0x80, .high = 0x9f},
    {.low = 0x90, .high = 0xbf}, {.low = 0x80, .high = 0x8f},
};

struct codepoint_t {
    size_t width = 1;
    rune_t value = rune_invalid;
};

struct encoded_rune_t {
    size_t width = 1;
    rune_t value = rune_invalid;
    uint8_t data[4];
};

int64_t strlen(std::string_view str);

encoded_rune_t encode(const rune_t &r);

codepoint_t decode(const char *str, size_t length);

}  // namespace common::utf8