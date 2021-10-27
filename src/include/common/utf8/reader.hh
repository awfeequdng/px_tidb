#pragma once
#include <stack>
#include <string_view>
#include "common/utf8/rune.hh"

namespace common::utf8 {
    class reader_t final {
    public:
        reader_t(std::string_view slice);

        void push_mark();

        size_t pop_mark();

        size_t current_mark();

        bool seek(size_t index);

        void restore_top_mark();

        rune_t curr();

        rune_t next();

        rune_t prev();

        bool move_prev();

        bool move_next();

        [[nodiscard]] bool eof() const;

        [[nodiscard]] size_t pos() const;

        [[nodiscard]] uint32_t width() const;

        [[nodiscard]] std::string_view make_slice(
                size_t offset,
                size_t length) const;

        [[nodiscard]] std::string_view slice() const;

    private:
        rune_t read(uint32_t& width) const;

    private:
        size_t _index{};
        std::string_view _slice;
        std::stack<size_t> _mark_stack;
        std::stack<uint32_t> _width_stack;
    };
}