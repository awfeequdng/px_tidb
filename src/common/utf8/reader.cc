#include "common/utf8/reader.hh"

namespace common::utf8 {

    reader_t::reader_t(
            std::string_view slice) : _slice(slice),
                                      _mark_stack(),
                                      _width_stack() {
    }

    void reader_t::push_mark() {
        _mark_stack.push(_index);
    }

    bool reader_t::eof() const {
        return _index > _slice.size() - 1;
    }

    size_t reader_t::pop_mark() {
        const auto mark = _mark_stack.top();
        _mark_stack.pop();
        return mark ? mark : _index;
    }

    size_t reader_t::pos() const {
        return _index;
    }

    size_t reader_t::current_mark() {
        auto mark = _mark_stack.top();
        return mark ? mark : _index;
    }

    uint32_t reader_t::width() const {
        uint32_t width;
        read(width);
        return width;
    }

    bool reader_t::seek(size_t index) {
        if (index > _slice.size() - 1)
            return false;
        _index = index;
        while (!_width_stack.empty())
            _width_stack.pop();
        while (!_mark_stack.empty())
            _mark_stack.pop();
        return true;
    }

    rune_t reader_t::curr() {
        if (eof()) return rune_eof;

        uint32_t width;
        return read(width);
    }

    rune_t reader_t::next() {
        if (eof()) return rune_eof;

        uint32_t width;
        auto rune = read(width);
        _index += width;
        _width_stack.push(width);
        return rune;
    }

    void reader_t::restore_top_mark() {
        if (_mark_stack.empty())
            return;
        _index = _mark_stack.top();
    }

    rune_t reader_t::prev() {
        if (_index == 0 || _width_stack.empty()) {
            return rune_invalid;
        }
        _index -= _width_stack.top();
        _width_stack.pop();
        uint32_t width;
        return read(width);
    }

    bool reader_t::move_prev() {
        if (_index == 0 || _width_stack.empty()) {
            return false;
        }

        _index -= _width_stack.top();
        _width_stack.pop();

        return true;
    }

    bool reader_t::move_next() {
        if (eof()) {
            return false;
        }

        uint32_t width;
        read(width);
        _index += width;
        _width_stack.push(width);
        return true;
    }

    std::string_view reader_t::slice() const {
        return _slice;
    }

    rune_t reader_t::read(uint32_t& width) const {
        width = 1;
        uint8_t ch = _slice[_index];
        auto rune = rune_t(ch);
        if (ch == 0) {
            return rune_invalid;
        } else if (ch >= 0x80) {
            auto cp = decode(
                    (char*)(_slice.data() + _index),
                    _slice.size() - _index);
            width = cp.width;
            rune = cp.value;
            if (rune == rune_invalid && width == 1) {
                return rune_invalid;
            } else if (rune == rune_bom && _index > 0) {
                return rune_invalid;
            }
        }
        return rune;
    }

    std::string_view reader_t::make_slice(size_t offset, size_t length) const {
        return std::string_view(_slice.data() + offset, length);
    }

}