#ifndef ZEN_BIT_H
#define ZEN_BIT_H

#include "zen_config.h"
#include <cstring>

#ifdef ZEN_COMPILER_MSVC
#include <intrin.h>
#pragma intrinsic(_BitScanForward)
#pragma intrinsic(_BitScanForward64)
#pragma intrinsic(_BitScanReverse)
#pragma intrinsic(_BitScanReverse64)
#endif


// Some std functions are only constexpr in c++20
#ifdef ZEN_CPP20
    #define CPP20_CONSTEXPR constexpr
#else
    #define CPP20_CONSTEXPR
#endif


namespace zen {

// Fast mod for powers of two
template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
ZEN_FORCEINLINE constexpr T fast_mod(T value, T mod) noexcept { 
    return value & (mod - 1); 
}

// Even faster compile time mod for powers of two
template<usize Mod, typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
ZEN_FORCEINLINE constexpr T fast_mod(T value) noexcept { 
    return value & T(Mod - 1); 
}

// Everyone needs min/max and it seems to bring along a lot of standard header bloat...
template<typename T>
ZEN_FORCEINLINE constexpr const T& min(const T& a, const T& b) noexcept { 
    return a < b ? a : b; 
}

// Everyone needs min/max and it seems to bring along a lot of standard header bloat...
template<typename T>
ZEN_FORCEINLINE constexpr const T& max(const T& a, const T& b) noexcept { 
    return a < b ? b : a; 
}

// Never hard-code this simple logic again!
template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
ZEN_FORCEINLINE constexpr T align_up(T value, T align) noexcept {
    return (value + align - T(1)) & ~(align - T(1));
}

// Never hard-code this simple logic again!
template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
ZEN_FORCEINLINE constexpr T align_down(T value, T align) noexcept {
    return value & ~(align - T(1));
}

// Never hard-code this simple logic again!
template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
ZEN_FORCEINLINE constexpr T round_up(T value, T multiple) noexcept {
    return ((value + multiple - T(1)) / multiple) * multiple;
}

// Never hard-code this simple logic again!
template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
ZEN_FORCEINLINE constexpr T round_down(T value, T multiple) noexcept {
    return (value / multiple) * multiple;
}

// Saturated integer addition
template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
ZEN_FORCEINLINE constexpr T add_sat(T target, T value) noexcept {
    T current = target;
    target += value;
    target |= -T(target < current);
    return target;
}

// Saturated integer subtraction
template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
ZEN_FORCEINLINE constexpr T sub_sat(T target, T value) noexcept {
    T current = target;
    target -= value;
    target &= -(target <= current);
    return target;
}

// Count number of leading zeros (0btttt11111llll)
template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
ZEN_FORCEINLINE constexpr usize leading_zeros(T value) noexcept {
    #ifdef ZEN_COMPILER_MSVC
        unsigned long n{};
        if constexpr(sizeof(T) == sizeof(u64)) { if (ZEN_UNLIKELY(!_BitScanForward(&n, value))) { n = 64; } }
        else                                   { if (ZEN_UNLIKELY(!_BitScanForward64(&n, value))) { n = 32; } }
        return usize(n);
    #else
        if constexpr(sizeof(T) == sizeof(u64)) { return __builtin_clzl(value); }
        else                                   { return __builtin_clz(value); }
    #endif
}

// Count number of trailing zeros (0btttt11111llll)
template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
ZEN_FORCEINLINE constexpr usize trailing_zeros(T value) noexcept {
    #ifdef ZEN_COMPILER_MSVC
        unsigned long n{};
        if constexpr(sizeof(T) == sizeof(u64)) { if (ZEN_UNLIKELY(!_BitScanReverse(&n, value))) { n = 0; } n = 64 - n; }
        else                                   { if (ZEN_UNLIKELY(!_BitScanReverse64(&n, value))) { n = 0; } n = 32 - n; }
        return usize(n);
    #else
        if constexpr(sizeof(T) == sizeof(u64)) { return __builtin_ctzl(x); }
        else                                   { return __builtin_ctz(x); }
    #endif
}

// Count number of set bits
template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
ZEN_FORCEINLINE constexpr usize bit_count(T value) noexcept {
    #ifdef ZEN_COMPILER_MSVC
        if constexpr(sizeof(T) == sizeof(u64)) { return __popcnt(value); }
        else                                   { return __popcnt64(value); }
    #else
        if constexpr(sizeof(T) == sizeof(u64)) { return __builtin_popcountl(value); }
        else                                   { return __builtin_popcount(value); }
    #endif
}

// Integer log-2
template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
ZEN_FORCEINLINE constexpr T ilog2(T x) noexcept {
    const T l = sizeof(T) * 8 - leading_zeros(x) - 1;
    return l + ((x - (T(1) << l)) > 0);
}

// Cast from one primitive type to another transparently
template<typename To, typename From>
static constexpr bool cast_valid = sizeof(To) == sizeof(From) 
    && std::is_trivially_copyable_v<From> 
    && std::is_trivially_copyable_v<To>
    && std::is_trivially_constructible_v<To>;

template <typename To, typename From, typename = std::enable_if_t<cast_valid<To, From>, To>>
ZEN_FORCEINLINE CPP20_CONSTEXPR To bit_cast(const From& src) noexcept {
    To dst;
    std::memcpy(&dst, &src, sizeof(To));
    return dst;
}

// Bit equality
template <typename L, typename R, typename = std::enable_if_t<sizeof(L) == sizeof(R), L>>
ZEN_FORCEINLINE CPP20_CONSTEXPR bool bit_equal(const L& l, const R& r) noexcept {
    return memcmp(&l, &r, sizeof(L)) == 0;
}

// Bit equality for arrays
template <typename L, typename R, typename = std::enable_if_t<sizeof(L) == sizeof(R), L>>
ZEN_FORCEINLINE CPP20_CONSTEXPR bool bit_equal(const L* l, const R* r, usize n) noexcept {
    return memcmp(l, r, n * sizeof(L)) == 0;
}

// Range contains
template <typename R, typename T, typename = std::void_t<decltype(std::declval<R>().begin() == std::declval<R>().end())>>
ZEN_FORCEINLINE constexpr bool contains(const R& range, const T& value) noexcept {
    for (const auto& v: range) {
        if (v == value)
            return true;        
    }
    return false;
}


// Power of two functions
namespace po2 {

// Check if integer is power of two
template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
ZEN_FORCEINLINE constexpr bool check(T n) noexcept {
    return n != T(0) && (n & (n - T(1))) == T(0);
}

// Round up to next power-of-two
template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
ZEN_FORCEINLINE constexpr T round_up(T v) noexcept {
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    if constexpr(sizeof(T) > sizeof(u8))  { v |= v >> 8; }
    if constexpr(sizeof(T) > sizeof(u16)) { v |= v >> 16; }
    if constexpr(sizeof(T) > sizeof(u32)) { v |= v >> 32; }
    v++;
    return v;
}

}

}

#undef CPP20_CONSTEXPR

#endif // ZEN_BIT_H
