#ifndef ZEN_STRING_H
#define ZEN_STRING_H

#include "zen_num.h"
#include <cstring>
#include <string_view>

namespace zen {

using std::string_view;

template<usize N>
struct sstring {
    using size_type = num::with::max_value<N>;
    using value_type = char;

    constexpr sstring() = default;

    constexpr operator string_view() const noexcept { return string_view{m_data, m_size}; }
    constexpr string_view     view() const noexcept { return string_view{m_data, m_size}; }

    static constexpr size_type      max_size()                  noexcept { return N; }
    ZEN_ND constexpr bool           empty()               const noexcept { return m_size == 0; }
    ZEN_ND constexpr size_type      size()                const noexcept { return m_size; }
    ZEN_ND constexpr char*          data()                      noexcept { return m_data; }
    ZEN_ND constexpr const char*    data()                const noexcept { return m_data; }
    ZEN_ND constexpr char*          begin()                     noexcept { return m_data; }
    ZEN_ND constexpr const char*    begin()               const noexcept { return m_data; }
    ZEN_ND constexpr char*          end()                       noexcept { return m_data + m_size; }
    ZEN_ND constexpr const char*    end()                 const noexcept { return m_data + m_size; }
    ZEN_ND constexpr char&          front()                     noexcept { return *m_data; }
    ZEN_ND constexpr const char     front()               const noexcept { return *m_data; }
    ZEN_ND constexpr char&          back()                      noexcept { return *(m_data + m_size - 1); }
    ZEN_ND constexpr const char     back()                const noexcept { return *(m_data + m_size - 1); }
    ZEN_ND constexpr char&          operator[](usize i)         noexcept { return m_data[i]; }
    ZEN_ND constexpr const char     operator[](usize i)   const noexcept { return m_data[i]; }

    constexpr void clear()                          noexcept { m_size = 0; }
    constexpr void append(char c)                   noexcept { m_data[m_size++] = c; }
    constexpr void append(const char* c, usize n)   noexcept { memcpy(end(), c, n); m_size += size_type(n);  }
    constexpr void append(string_view s)            noexcept { memcpy(end(), s.data(), s.size()); m_size += s.size(); }
    constexpr void set_size(size_type s)            noexcept { m_size = s; }

    template<typename Out>
    friend Out& operator<<(Out& o, const sstring& v) noexcept { return o << string_view(v); }

private:    
    char      m_data[N]{};
    size_type m_size{};
};

}

#endif // ZEN_STRING_H
