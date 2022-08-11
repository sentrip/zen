#ifndef ZEN_SPAN_H
#define ZEN_SPAN_H

#include "zen_config.h"

namespace zen {

template<typename T>
struct span {
    using value_type = T;

    constexpr span() = default;
    constexpr span(T* d, usize s)   noexcept : m_data{d}, m_size{s} {}
    constexpr span(T* b, T* e)      noexcept : m_data{b}, m_size{static_cast<usize>(e-b)} {}

    template<usize N>
    constexpr span(T(&v)[N])        noexcept : m_data{&v[0]}, m_size{N} {}
    
    template<typename U, typename = std::void_t<decltype(std::declval<U&>().data() + std::declval<U&>().size())>>
    constexpr span(U&& v)           noexcept : m_data{v.data()}, m_size{v.size()} {}

    ZEN_ND constexpr bool        empty()               const noexcept { return m_size == 0; }
    ZEN_ND constexpr usize       size()                const noexcept { return m_size; }
    ZEN_ND constexpr T*          data()                const noexcept { return m_data; }
    ZEN_ND constexpr T*          begin()               const noexcept { return m_data; }
    ZEN_ND constexpr T*          end()                 const noexcept { return m_data + m_size; }
    ZEN_ND constexpr T&          front()               const noexcept { return *m_data; }
    ZEN_ND constexpr T&          back()                const noexcept { return *(m_data + m_size - 1); }
    ZEN_ND constexpr T&          operator[](usize i)   const noexcept { return m_data[i]; }

private:
    T*     m_data{};
    usize  m_size{};
};

// Deduction guides
template<typename T>          span(T*, usize) -> span<T>;
template<typename T>          span(T*, T*)    -> span<T>;
template<typename T, usize N> span(T(&)[N])   -> span<T>;

}

#endif // ZEN_SPAN_H
