#pragma once
#include <stack>
#include <string_view>
#include <unordered_map>

#include "common/utf8/rune.hh"

namespace common::utf8 {

class reader_t final {
public:
    reader_t(std::string_view slice);

    bool seek(size_t index);

    [[nodiscard]] size_t length();

    rune_t curr();

    rune_t next();

    rune_t prev();

    bool move_prev();

    bool move_next();

    [[nodiscard]] bool eof() const;

    [[nodiscard]] Pos pos() const;

    [[nodiscard]] size_t index() const;

    [[nodiscard]] uint32_t width() const;

    [[nodiscard]] std::string_view make_slice(size_t offset, size_t length) const;

    [[nodiscard]] std::string_view slice() const;

    rune_t incAsLongAs(std::function<bool(rune_t)> fn);

    std::string data(const Pos &from) {
        std::string_view str_view = make_slice(from._offset, _index - from._offset);
        return {str_view.begin(), str_view.end()};
    }

private:
    rune_t read(uint32_t &width) const;

private:
    size_t _index{};
    std::string_view _slice;
    Pos _pos{1, 1, 0};
    std::unordered_map<size_t, Pos> _idx2pos;
    std::stack<uint32_t> _width_stack;
};
}  // namespace common::utf8