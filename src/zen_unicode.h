
#ifndef ZEN_UNICODE_H
#define ZEN_UNICODE_H

#include "zen_config.h"

namespace zen::unicode {

// Unicode codepoint iterator function
constexpr char32_t next(const char* text, usize& cursor)
{
    const u32 character = u32(u8(text[cursor]));
    usize end = cursor;
    u32 mask = 0x7f;

    // Sequence size
    if(ZEN_LIKELY(character < 128)) {
        end += 1;
    } else if((character & 0xe0) == 0xc0) {
        end += 2;
        mask = 0x1f;
    } else if((character & 0xf0) == 0xe0) {
        end += 3;
        mask = 0x0f;
    } else if((character & 0xf8) == 0xf0) {
        end += 4;
        mask = 0x07;
    }
    // Wrong sequence start
    else {
        ++cursor;
        return U'\xffffffff';
    }
    // Compute the codepoint
    char32_t result = character & mask;
    for(usize i = cursor + 1; i != end; ++i) {
        // Garbage in the sequence
        if(ZEN_UNLIKELY((text[i] & 0xc0) != 0x80)) {
            ++cursor;
            return U'\xffffffff';
        }
        result <<= 6;
        result |= (text[i] & 0x3f);
    }
    cursor = end;
    return result;
}


// C++ style iterator for unicode code points
struct iterator {
    constexpr iterator() = default;
    constexpr iterator(const char* text, char32_t v) : ptr{text}, val{v} {}
    constexpr char32_t  operator*()     const noexcept { return val; }
    constexpr iterator& operator++()          noexcept { usize c{}; val = next(ptr, c); ptr += c; return *this; }
    constexpr iterator  operator++(int)       noexcept { auto c = *this; this->operator++(); return c; }
    constexpr bool      operator==(const iterator& o) const noexcept { return ptr == o.ptr; }
    constexpr bool      operator!=(const iterator& o) const noexcept { return ptr != o.ptr; }

private:
    const char* ptr{};
    char32_t    val{};
};


// Range object for use in for loops
struct iter {
    const char* text{};
    const usize size{};

    iter(const char* text) noexcept : text{text}, size{strlen(text)} {}
    
    template<typename U, typename = std::void_t<decltype(std::declval<U>().data() + std::declval<U>().size())>>
    iter(U&& u) noexcept : text{u.data()}, size{u.size()} {}

    iterator begin() const noexcept {
        usize c{};
        auto v = next(text, c);
        return iterator{text + c, v};
    }

    iterator end() const noexcept {
        return iterator{text + size + 1, 0};
    }
};

}

#endif ZEN_UNICODE_H
