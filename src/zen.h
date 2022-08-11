#ifndef ZEN_CONFIG_H
#define ZEN_CONFIG_H

// Only lightweight standard library includes
#include <cstdint>
#include <type_traits>


// Standard integer definitions
using i8        = int8_t;
using i16       = int16_t;
using i32       = int32_t;
using i64       = int64_t;
using u8        = uint8_t;
using u16       = uint16_t;
using u32       = uint32_t;
using u64       = uint64_t;
using f32       = float;
using f64       = double;
using usize     = size_t;


// World's smallest pair
template<typename T1, typename T2 = T1> struct pair { T1 first; T2 second; };
template<typename T1, typename T2 = T1> pair(T1&&, T2&&) -> pair<T1, T2>;
template<typename T1, typename T2 = T1> pair(const T1&, const T2&) -> pair<T1, T2>;


// Compiler/platform detection
#ifdef _MSC_VER

    #define ZEN_PLATFORM_WINDOWS
    #define ZEN_COMPILER_MSVC

#elif defined(__linux__)

    #define ZEN_PLATFORM_LINUX

    #ifdef __clang__
        #define ZEN_COMPILER_CLANG
    #elif defined(__GNUC__)
        #define ZEN_COMPILER_GCC
    #else
        #error "zen does not recognize this linux compiler"
    #endif

#else
    #error "zen does not support this platform"
#endif


// C++20 support detection
#ifdef ZEN_COMPILER_MSVC
    #define ZEN_CPP_VERSION _MSVC_LANG
#else
    #define ZEN_CPP_VERSION __cplusplus
#endif
#if ZEN_CPP_VERSION >= 202002L
    #define ZEN_CPP20
#endif


// Likely/unlikely/inline/noreturn
#if defined(ZEN_COMPILER_CLANG) || defined(ZEN_COMPILER_GCC)
    #define ZEN_LIKELY(x)           __builtin_expect(!!(x), 1)
    #define ZEN_UNLIKELY(x)         __builtin_expect(!!(x), 0)
    #define ZEN_FORCEINLINE         inline __attribute__((always_inline))
    #define ZEN_NEVERINLINE         inline __attribute__((never_inline))
    #define ZEN_NORETURN            [[noreturn]]

#elif defined(ZEN_COMPILER_MSVC)
    #define ZEN_LIKELY(x)           x
    #define ZEN_UNLIKELY(x)         x
    #define ZEN_FORCEINLINE         __forceinline
    #define ZEN_NEVERINLINE         __declspec(noinline)
    #define ZEN_NORETURN            __declspec(noreturn)

#else
    #define LIKELY(x)               x
    #define UNLIKELY(x)             x
    #define ZEN_FORCEINLINE         inline
    #define ZEN_NEVERINLINE
    #define ZEN_NORETURN            [[noreturn]]
#endif


// Faster than std::forward
#define ZEN_FWD(...) static_cast<decltype(__VA_ARGS__) &&>(__VA_ARGS__)


// Cleaner than [[nodiscard]]
#define ZEN_ND [[nodiscard]]


// Always used but rarely defined
#define ZEN_CACHE_LINE  64
#define ZEN_MEMORY_PAGE 4096


// std forward declarations used in template deduction guides
namespace std {

template<typename T>
class initializer_list;

}

#endif // ZEN_CONFIG_H

#ifndef ZEN_ALLOC_H
#define ZEN_ALLOC_H

#include <memory_resource>

namespace zen {

// The std:: names can get really long...
template<typename T = u8>
using alloc_t       = std::pmr::polymorphic_allocator<T>;

using mem_resource  = std::pmr::memory_resource;
using mem_buffer    = std::pmr::monotonic_buffer_resource;
using mem_pool      = std::pmr::unsynchronized_pool_resource;
using mem_pool_sync = std::pmr::synchronized_pool_resource;


namespace mem {

// Aligned buffer for container storage
template<typename T, usize NBytes = sizeof(T), usize Align = alignof(T)>
struct aligned_buffer {
    aligned_buffer() = default;
    operator T*      ()        noexcept  { return reinterpret_cast<T*>(&buf[0]); }
    operator const T*()  const noexcept  { return reinterpret_cast<const T*>(&buf[0]); }
private:
    alignas(Align) char buf[NBytes]{};
};


// I don't know why casting to void got so complicated in C++20...
#if __cplusplus >= 202002L
#define VOIDIFY(ptr) const_cast<void*>(static_cast<const volatile void*>(ptr))
#else
#define VOIDIFY(ptr) static_cast<void*>(ptr)
#endif


template<typename T, typename... Args>
ZEN_FORCEINLINE constexpr T* construct_at(T* p, Args&&... args) {
    return ::new (VOIDIFY(p)) T(ZEN_FWD(args)...);
}

template<typename T>
ZEN_FORCEINLINE constexpr void move(T* first, T* last, T* dst) {
    while (first != last)
        *dst++ = std::move(*first++);
}

template<typename T>
ZEN_FORCEINLINE constexpr void move_backward(T* first, T* last, T* dst) {
    while (first != last)
        *(dst++) = std::move(*(--last));
}

template<typename T>
ZEN_FORCEINLINE constexpr void uninit_fill(T* first, T* last) {
    for (; first != last; ++first)
        ::new (VOIDIFY(first)) T;
}

template<typename T>
ZEN_FORCEINLINE constexpr void uninit_fill(T* first, T* last, const T& value) {
    for (; first != last; ++first)
        ::new (VOIDIFY(first)) T(value);
}

template<typename It, typename T>
ZEN_FORCEINLINE constexpr void uninit_copy(It first, It last, T* dst) {
    for (; first != last; ++dst, (void)++first)
        ::new (VOIDIFY(dst)) T(*first);
}

template<typename T>
ZEN_FORCEINLINE constexpr void uninit_move(T* first, T* last, T* dst) {
    for (; first != last; ++dst, (void) ++first)
        ::new (VOIDIFY(dst)) T(std::move(*first));
}

template<typename T> 
ZEN_FORCEINLINE constexpr void destroy_at(T* p) { 
    if constexpr(!std::is_trivially_destructible_v<T>)
        p->~T(); 
}

template<typename T> 
ZEN_FORCEINLINE constexpr void destroy_n(T* p, usize n) { 
    if constexpr(!std::is_trivially_destructible_v<T>) {
        for (; n > 0; (void) ++p, --n)
            p->~T();
    }
}

template<typename T>
ZEN_FORCEINLINE constexpr void swap_ranges(T* left, usize left_size, T* right, usize right_size) {
    const usize n = left_size < right_size ? left_size : right_size;
    for (usize i = 0; i < n; ++i) std::swap(left[i], right[i]);
    const auto* dst = left_size < right_size ? left : right;
    const auto* src = left_size < right_size ? right : left;
    const usize mn = left_size < right_size ? right_size : left_size;
    move(src + n, src + mn, dst + n);
}

#undef VOIDIFY

}

}

#endif // ZEN_ALLOC_H

#ifndef ZEN_ARRAY_H
#define ZEN_ARRAY_H


namespace zen {

template <typename T, usize N>
struct array {
    using value_type = T;
    using size_type = usize;
    using reference = T&;
    using const_reference = const T&;
    using iterator = T*;
    using const_iterator = const T*;

    value_type m_data[N];

    constexpr void                      fill(const_reference v)       noexcept { for (auto& a: m_data) a = v; }

    ZEN_ND constexpr bool               empty()                 const noexcept { return N == 0; }
    ZEN_ND constexpr size_type          size()                  const noexcept { return N; }
    static constexpr size_type          max_size()                    noexcept { return N; }
    ZEN_ND constexpr T*                 data()                        noexcept { return &m_data[0]; }
    ZEN_ND constexpr const T*           data()                  const noexcept { return &m_data[0]; }

    ZEN_ND constexpr reference          operator[](size_type i)       noexcept { return m_data[i]; }
    ZEN_ND constexpr const_reference    operator[](size_type i) const noexcept { return m_data[i]; }
    ZEN_ND constexpr const_reference    at(size_type i)         const noexcept { return m_data[i]; }
    ZEN_ND constexpr reference          at(size_type i)               noexcept { return m_data[i]; }
    ZEN_ND constexpr reference          front()                       noexcept { return m_data[0]; }
    ZEN_ND constexpr const_reference    front()                 const noexcept { return m_data[0]; }
    ZEN_ND constexpr reference          back()                        noexcept { return m_data[N-1]; }
    ZEN_ND constexpr const_reference    back()                  const noexcept { return m_data[N-1]; }

    ZEN_ND constexpr iterator           begin()                       noexcept { return &m_data[0]; }
    ZEN_ND constexpr const_iterator     begin()                 const noexcept { return &m_data[0]; }
    ZEN_ND constexpr iterator           end()                         noexcept { return &m_data[N]; }
    ZEN_ND constexpr const_iterator     end()                   const noexcept { return &m_data[N]; }
};


template <typename T>
struct array<T, 0> {
    using value_type = T;
    using size_type = usize;
    using reference = T&;
    using const_reference = const T&;
    using iterator = T*;
    using const_iterator = const T*;

    constexpr void                      fill(const_reference v)       noexcept {}

    ZEN_ND constexpr bool               empty()                 const noexcept { return true; }
    ZEN_ND constexpr size_type          size()                  const noexcept { return 0; }
    static constexpr size_type          max_size()                    noexcept { return 0; }
    ZEN_ND constexpr T*                 data()                        noexcept { return nullptr; }
    ZEN_ND constexpr const T*           data()                  const noexcept { return nullptr; }

    ZEN_ND constexpr reference          operator[](size_type i)       noexcept { return *data(); }
    ZEN_ND constexpr const_reference    operator[](size_type i) const noexcept { return *data(); }
    ZEN_ND constexpr const_reference    at(size_type i)         const noexcept { return *data(); }
    ZEN_ND constexpr reference          at(size_type i)               noexcept { return *data(); }
    ZEN_ND constexpr reference          front()                       noexcept { return *data(); }
    ZEN_ND constexpr const_reference    front()                 const noexcept { return *data(); }
    ZEN_ND constexpr reference          back()                        noexcept { return *data(); }
    ZEN_ND constexpr const_reference    back()                  const noexcept { return *data(); }

    ZEN_ND constexpr iterator           begin()                       noexcept { return data(); }
    ZEN_ND constexpr const_iterator     begin()                 const noexcept { return data(); }
    ZEN_ND constexpr iterator           end()                         noexcept { return data(); }
    ZEN_ND constexpr const_iterator     end()                   const noexcept { return data(); }
};


// Deduction guides
// NOTE: Only the type of the first parameter is used EXPLICITLY to prevent
// having to rewrite types in array initializers, if the rest of the arguments cannot
// be converted to the first type then a compile error is raised anyway
template<typename T0, typename... T> array(T0, T...) -> array<T0, 1 + sizeof...(T)>;

}

#endif ZEN_ARRAY_H

#ifndef ZEN_BIT_H
#define ZEN_BIT_H

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

#ifndef ZEN_NUM_H
#define ZEN_NUM_H


namespace zen::num {

// Compile-time index
template<usize I>
struct index_t {
    static constexpr usize value = I;
    constexpr operator usize() const noexcept { return I; }
};
template<usize I>
constexpr index_t<I> index = index_t<I>{};


// Limits
template<typename T>
struct limits;

#define DEFINE_LIMITS(T, mn, mx) template<> struct limits<T> { \
    static constexpr T min() noexcept { return mn; } \
    static constexpr T max() noexcept { return mx; } \
}
DEFINE_LIMITS(u8,                                  0, 255);
DEFINE_LIMITS(u16,                                 0, 65535);
DEFINE_LIMITS(u32,                                 0, 4294967295u);
DEFINE_LIMITS(u64,                                 0, UINT64_C(18446744073709551615));
DEFINE_LIMITS(i8 ,                          -127 - 1, 127);
DEFINE_LIMITS(i16,                        -32767 - 1, 32767);
DEFINE_LIMITS(i32,                   -2147483647 - 1, 2147483647);
DEFINE_LIMITS(i64, INT64_C(-9223372036854775807) - 1, UINT64_C(9223372036854775807));

#undef DEFINE_LIMITS


// Integer types based on conditions
namespace with {

// Byte width
namespace detail {
template<usize N, bool Signed = false, typename = void> 
struct byte_width;

template<usize N> struct byte_width<N, false, std::enable_if_t<N <= 1           >> { using type = u8; };
template<usize N> struct byte_width<N, false, std::enable_if_t<N == 2           >> { using type = u16; };
template<usize N> struct byte_width<N, false, std::enable_if_t<N >= 3 && N <= 4 >> { using type = u32; };
template<usize N> struct byte_width<N, false, std::enable_if_t<N >= 5 && N <= 8 >> { using type = u64; };
template<usize N> struct byte_width<N, true , std::enable_if_t<N <= 1           >> { using type = i8; };
template<usize N> struct byte_width<N, true , std::enable_if_t<N == 2           >> { using type = i16; };
template<usize N> struct byte_width<N, true , std::enable_if_t<N >= 3 && N <= 4 >> { using type = i32; };
template<usize N> struct byte_width<N, true , std::enable_if_t<N >= 5 && N <= 8 >> { using type = i64; };
}
template<auto N>
using byte_width = typename detail::byte_width<usize(N), std::is_signed_v<decltype(N)>>::type;


// Bit width
namespace detail {
template<usize NBits, bool Signed = false, typename = void> 
struct bit_width;

template<usize NBits> struct bit_width<NBits, false, std::enable_if_t<NBits <= 8                >> { using type = u8; };
template<usize NBits> struct bit_width<NBits, false, std::enable_if_t<NBits >= 9  && NBits <= 16>> { using type = u16; };
template<usize NBits> struct bit_width<NBits, false, std::enable_if_t<NBits >= 17 && NBits <= 32>> { using type = u16; };
template<usize NBits> struct bit_width<NBits, false, std::enable_if_t<NBits >= 33 && NBits <= 64>> { using type = u64; };
template<usize NBits> struct bit_width<NBits, true , std::enable_if_t<NBits <= 8                >> { using type = i8; };
template<usize NBits> struct bit_width<NBits, true , std::enable_if_t<NBits >= 9  && NBits <= 16>> { using type = i16; };
template<usize NBits> struct bit_width<NBits, true , std::enable_if_t<NBits >= 17 && NBits <= 32>> { using type = i16; };
template<usize NBits> struct bit_width<NBits, true , std::enable_if_t<NBits >= 33 && NBits <= 64>> { using type = i64; };
}

template<auto NBits>
using bit_width = typename detail::bit_width<usize(NBits), std::is_signed_v<decltype(NBits)>>::type;


// Max value
namespace detail {
template<u64 N, typename = void> 
struct max_uvalue;

template<i64 N, typename = void> 
struct max_ivalue;

template<auto N, typename = void> 
struct max_value;

template<u64 N> struct max_uvalue<N, std::enable_if_t<N <= 256                      >> { using type = u8; };
template<u64 N> struct max_uvalue<N, std::enable_if_t<N >= 256 && N < 65536         >> { using type = u16; };
template<u64 N> struct max_uvalue<N, std::enable_if_t<N >= 65536 && N < 4294967296u >> { using type = u16; };
template<u64 N> struct max_uvalue<N, std::enable_if_t<N >= 4294967296u              >> { using type = u64; };
template<i64 N> struct max_ivalue<N, std::enable_if_t<N <= 128                      >> { using type = i8; };
template<i64 N> struct max_ivalue<N, std::enable_if_t<N >= 128 && N < 32768         >> { using type = i16; };
template<i64 N> struct max_ivalue<N, std::enable_if_t<N >= 32768 && N < 2147483647  >> { using type = i16; };
template<i64 N> struct max_ivalue<N, std::enable_if_t<N >= 2147483647               >> { using type = i64; };

template<auto N> struct max_value<N, std::enable_if_t<std::is_unsigned_v<decltype(N)>>> { using type = typename max_uvalue<u64(N)>::type; };
template<auto N> struct max_value<N, std::enable_if_t<std::is_signed_v<decltype(N)>>>   { using type = typename max_ivalue<u64(N)>::type; };
}

template<auto N>
using max_value = typename detail::max_value<N>::type;

}

// Sizeof that compiles for undefined types
namespace impl {
template<typename T, typename = void>
struct sizeof_type { static constexpr usize value = 0; };

template<typename T>
struct sizeof_type<T, std::void_t<decltype(sizeof(T))>> { static constexpr usize value = sizeof(T); };
}
template<typename T>
static constexpr usize sizeof_type = impl::sizeof_type<T>::value;

}

namespace zen {
    
// Compile time type index with context
namespace impl {
    
template<typename Context = void>
struct type_id_provider {
    static u32 next() noexcept {
        static u32 value{};
        return value++;
    }
};

template<typename Type, typename Context = void>
struct type_id {
    static u32 value() noexcept {
        static const u32 v = type_id_provider<Context>::next();
        return v;
    }
};

}

template<typename T, typename Context = void>
constexpr u32 type_id() noexcept { return impl::type_id<T, Context>::value(); }

}

#endif // ZEN_NUM_H

#ifndef ZEN_BITSET_H
#define ZEN_BITSET_H


namespace zen {

template<typename W = u64, typename I = u32>
struct bit_view;


template<typename W = u64, typename I = u32>
using cbit_view = bit_view<const W, I>;


#define nth_bit(T, i) T(1) << ((i) & ((sizeof(T) * 8) - 1))


// Bit view with storage
template<usize N, typename W = u64>
struct bitset {
    using word_type = W;
    static constexpr W     word_max   = num::limits<W>::max();
    static constexpr usize word_nbits = 8 * sizeof(W);
    static constexpr usize n_words    = 1 + ((N - 1) / word_nbits);
    
    ZEN_FORCEINLINE constexpr bitset() = default;

    // Bit manipulation
           ZEN_FORCEINLINE constexpr void reset     ()                       noexcept { for (auto& w: words) w = 0; }
           ZEN_FORCEINLINE constexpr void set       (usize i)                noexcept { words[i / word_nbits] |= nth_bit(W, i); }
           ZEN_FORCEINLINE constexpr void clear     (usize i)                noexcept { words[i / word_nbits] &= ~nth_bit(W, i); }
    ZEN_ND ZEN_FORCEINLINE constexpr bool test      (usize i)          const noexcept { return (words[i / word_nbits] & nth_bit(W, i)) != 0; }
    ZEN_ND ZEN_FORCEINLINE constexpr bool operator[](usize i)          const noexcept { return (words[i / word_nbits] & nth_bit(W, i)) != 0; }
    ZEN_ND ZEN_FORCEINLINE constexpr bool any       ()                 const noexcept { 
        for(const auto w: words) { 
            if (w != 0) 
                return true; 
        }
        return false; 
    }
    ZEN_ND ZEN_FORCEINLINE constexpr bool all       ()                 const noexcept { 
        for(const auto w: words) { 
            if (w != word_max) 
                return false; 
        }
        return true; 
    }
    
    // Word manipulation
    ZEN_ND ZEN_FORCEINLINE constexpr       W* data()                       noexcept { return words; }
    ZEN_ND ZEN_FORCEINLINE constexpr const W* data()                 const noexcept { return words; }
    ZEN_ND ZEN_FORCEINLINE constexpr       W* begin()                      noexcept { return words; }
    ZEN_ND ZEN_FORCEINLINE constexpr const W* begin()                const noexcept { return words; }
    ZEN_ND ZEN_FORCEINLINE constexpr       W* end()                        noexcept { return words + n_words; }
    ZEN_ND ZEN_FORCEINLINE constexpr const W* end()                  const noexcept { return words + n_words; }

    template<typename Out>
    friend Out& operator<<(Out& o, const bitset& v) noexcept { for (usize i = 0; i < N; ++i) o << u32(v[i]); return o; }

private:
    W words[n_words]{};
};


// Bit view with no storage
template<typename W, typename I>
struct bit_view {
    // NOTE: WORK IN PROGRESS
    using word_type = W;
    static constexpr W     word_max   = num::limits<W>::max();
    static constexpr I     word_nbits = 8 * sizeof(W);
    
    ZEN_FORCEINLINE constexpr bit_view() = default;

    ZEN_FORCEINLINE constexpr bit_view(W* words, I count) : 
        words{words}, begin{0}, end{count} {}

    ZEN_FORCEINLINE constexpr bit_view(W* begin, I offset, I count) : 
        words{begin + offset / word_nbits}, begin{offset & (word_nbits - 1)}, end{begin + count} {}

    ZEN_FORCEINLINE constexpr bit_view(W* begin, I begin_offset, W* end, I end_offset) : 
        words{begin + begin_offset / word_nbits}, begin{begin_offset & (word_nbits - 1)}, end{(end - begin) * word_nbits + end_offset} {}

    // Bit manipulation
    ZEN_ND ZEN_FORCEINLINE constexpr W    prefix()                     const noexcept { return words[0] & bit_shift_right_safe(word_max, word_nbits - begin); }
    ZEN_ND ZEN_FORCEINLINE constexpr W    suffix()                     const noexcept { return words[end / word_nbits] & (word_max << (end & (word_nbits - 1))); }
    ZEN_ND ZEN_FORCEINLINE constexpr bool test      (usize i)          const noexcept { I x = i + begin; return (words[x / word_nbits] & nth_bit(W, x)) != 0; }
    ZEN_ND ZEN_FORCEINLINE constexpr bool operator[](usize i)          const noexcept { I x = i + begin; return (words[x / word_nbits] & nth_bit(W, x)) != 0; }
    ZEN_ND ZEN_FORCEINLINE constexpr bool any       ()                 const noexcept { 
        if (prefix() != 0)
            return true;
        for(const auto w: *this) { 
            if (w != 0) 
                return true; 
        }
        return suffix() != 0; 
    }
    ZEN_ND ZEN_FORCEINLINE constexpr bool all       ()                 const noexcept { 
        if (prefix() != word_max)
            return true;
        for(const auto w: *this) { 
            if (w != word_max) 
                return false; 
        }
        return suffix() == word_max;
    }

    // Word manipulation
    ZEN_ND ZEN_FORCEINLINE constexpr       W* begin()                      noexcept { return words; }
    ZEN_ND ZEN_FORCEINLINE constexpr const W* begin()                const noexcept { return words; }
    ZEN_ND ZEN_FORCEINLINE constexpr       W* end()                        noexcept { return words; }
    ZEN_ND ZEN_FORCEINLINE constexpr const W* end()                  const noexcept { return words; }

private:
    W* words{};
    I  begin{}, end{};
};

#undef nth_bit

// Safely shift bits for widths >= width of type
template<typename T>
ZEN_FORCEINLINE constexpr T bit_shift_left_safe(T word, usize shift) noexcept {
    const auto sh = shift / 2;
    return (word << sh) << (shift - sh); 
}

// Safely shift bits for widths >= width of type
template<typename T>
ZEN_FORCEINLINE constexpr T bit_shift_right_safe(T word, usize shift) noexcept {
    const auto sh = shift / 2;
    return (word >> sh) >> (shift - sh); 
}

// Set range of bits with no offset
template<bool On = true, typename T>
constexpr void bit_range_set(T* data, usize bit_end) noexcept {
    constexpr usize NBits  = sizeof(T) * 8;
    constexpr T MAX        = num::limits<T>::max();
    const auto aligned_end = align_down(bit_end, NBits);
    const auto word_end    = aligned_end / NBits;
    const auto mask        = bit_shift_right_safe(MAX, (NBits - (bit_end & (NBits - 1))));
    if constexpr(On) {
        memset(data, 0xff, word_end * sizeof(T));
        data[word_end] |= mask;
    } else {
        memset(data, 0, word_end * sizeof(T));
        data[word_end] &= ~mask;
    }
}

// Set range of bits
template<bool On = true, usize MaxSize = SIZE_MAX, typename T>
constexpr void bit_range_set(T* data, usize bit_begin, usize bit_end) noexcept {
    constexpr usize NBits  = sizeof(T) * 8;
    constexpr T MAX        = num::limits<T>::max();
    const auto n           = bit_end - bit_begin;
    const auto word_begin  = bit_begin / NBits;
    const auto word_end    = bit_end / NBits;
    const auto shift_begin = bit_begin & (NBits - 1);
    const auto shift_end   = NBits - (bit_end & (NBits - 1));
    const auto mask_begin  = MAX << shift_begin;
    const auto mask_end    = bit_shift_right_safe(MAX, shift_end);
    // If bit_begin is aligned to a word boundary, we can use the fast version
    if (ZEN_UNLIKELY(shift_begin == 0)) {
        const auto offset  = bit_begin / NBits;
        return bit_range_set<On>(data + offset, bit_end - (offset * NBits));
    }

    // For ranges that are fully contained in a single word, we only need to write data in a single word and return
    if (ZEN_LIKELY(word_begin == word_end)) {
        if constexpr(On) {
            data[word_begin] |= (mask_begin & mask_end);
        } else {
            data[word_begin] &= ~(mask_begin & mask_end);
        }
        return;
    }

    // Otherwise we need to write two or more words
    if constexpr(On) {
        data[word_begin] |= mask_begin;
        data[word_end]   |= mask_end;
    } else {
        data[word_begin] &= ~mask_begin;
        data[word_end]   &= ~mask_end;
    }
    
    // If the data overflows a single word, then we need to loop and fill the middle
    if constexpr(MaxSize <= NBits) { return; }
    if (ZEN_LIKELY(n <= NBits)) { return; }

    const auto aligned_begin = align_up(bit_begin, NBits);
    const auto aligned_end   = align_down(bit_end, NBits);
    const auto middle        = (aligned_end - aligned_begin) / NBits;
    auto* begin              = data + (aligned_begin / NBits);
    memset(begin, On ? 0xff : 0, middle);
}

// Copy range of bits from src to dst
template<bool ClearBeforeWrite = true, usize MaxSize = SIZE_MAX, typename T>
constexpr void bit_range_copy(T* dst, usize dst_begin, const T* src, usize src_begin, usize src_end) noexcept {
    constexpr usize NBits      = sizeof(T) * 8;
    constexpr T MAX            = num::limits<T>::max();
    const auto bit_dst         = dst_begin & (NBits - 1);
    const auto bit_src         = src_begin & (NBits - 1);
    const auto n               = src_end - src_begin;
    const auto dst_end         = dst_begin + n;
    const auto dst_word_begin  = dst_begin / NBits;
    const auto dst_word_end    = dst_end / NBits;
    const auto src_word_begin  = src_begin / NBits;
    const auto src_word_end    = src_end / NBits;
    const auto src_shift_begin = src_begin & (NBits - 1);
    const auto src_shift_end   = NBits - (src_end & (NBits - 1));
    const auto src_mask_begin  = MAX << src_shift_begin;
    const auto src_mask_end    = bit_shift_right_safe(MAX, src_shift_end);
    const auto src_w_begin     = src[src_word_begin] & src_mask_begin;
    const auto src_w_end       = src[src_word_end] & src_mask_end;
        
    // If the local bit index is the same for src and dst, we only need to mask 
    // the first and last word, all other words in the middle can be byte-copied
    if (bit_dst == bit_src) {
        if constexpr(ClearBeforeWrite) {
            dst[dst_word_begin] &= ~src_mask_begin;
            dst[dst_word_end] &= ~src_mask_end;
        }
        dst[dst_word_begin]     |= src_w_begin;
        dst[dst_word_end]       |= src_w_end;
    }
    // Every single word will overflow, so we need a slower, more generic loop
    else {
        const auto dst_shift_begin = dst_begin & (NBits - 1);
        const auto dst_shift_end   = NBits - (dst_end & (NBits - 1));
        const auto dst_mask_begin  = MAX << dst_shift_begin;
        const auto dst_mask_end    = bit_shift_right_safe(MAX, dst_shift_end);
        if constexpr(ClearBeforeWrite) {
            dst[dst_word_begin] &= ~dst_mask_begin;
            dst[dst_word_end] &= ~dst_mask_end;
        }

        if (bit_dst > bit_src) {
            const auto delta         = bit_dst - bit_src;
            dst[dst_word_begin]     |= src_w_begin << delta;
            dst[dst_word_begin + 1] |= bit_shift_right_safe(src_w_begin, NBits - delta);
            dst[dst_word_end]       |= src_w_end << delta;
            dst[dst_word_end - 1]   |= bit_shift_right_safe(src_w_end, NBits - delta);
        } else {
            const auto delta       = bit_src - bit_dst;
            dst[dst_word_begin]     |= src_w_begin >> delta;
            dst[dst_word_begin + 1] |= bit_shift_left_safe(src_w_begin, NBits - delta);
            dst[dst_word_end]       |= src_w_end >> delta;
            dst[dst_word_end - 1]   |= bit_shift_left_safe(src_w_end, NBits - delta);
        }
    }

    // If the data overflows a single word, then we need to loop and fill the middle        
    if constexpr(MaxSize <= NBits) { return; }
    if (ZEN_LIKELY(n <= NBits)) { return; }
       
    const auto dst_aligned_begin    = align_up(dst_begin, NBits);
    const auto src_aligned_begin    = align_up(src_begin, NBits);
    const auto src_aligned_end      = align_down(src_end, NBits);
    auto* dst_data_begin            = dst + (dst_aligned_begin / NBits);
    const auto* src_data_begin      = src + (src_aligned_begin / NBits);

    // The middle can just be byte copied
    if (bit_dst == bit_src) {
        memcpy(dst_data_begin, src_data_begin, (src_aligned_end - src_aligned_begin) / 8);
    }
    // Otherwise we must handle overflow for every single word
    else {
        const auto middle               = (src_aligned_end - src_aligned_begin) / NBits;
        const auto* src_data_end        = src_data_begin + middle;        
        if (bit_dst > bit_src) {
            const auto delta            = bit_dst - bit_src;
            for (; src_data_begin != src_data_end; ++src_data_begin, (void) ++dst_data_begin) {
                if constexpr(ClearBeforeWrite) { *dst_data_begin = 0; }
                *dst_data_begin        |= *src_data_begin << delta;
                *(dst_data_begin + 1)  |= bit_shift_right_safe(*src_data_begin, NBits - delta);
            }
        }
        else {
            const auto delta            = bit_src - bit_dst;
            for (; src_data_begin != src_data_end; ++src_data_begin, (void) ++dst_data_begin) {
                if constexpr(ClearBeforeWrite) { *dst_data_begin = 0; }
                *dst_data_begin        |= *src_data_begin >> delta;
                *(dst_data_begin - 1)  |= bit_shift_left_safe(*src_data_begin, NBits - delta);
            }
        }
    }
}

}

#endif //ZEN_BITSET_H

#ifndef ZEN_BSWAP_H
#define ZEN_BSWAP_H


namespace zen::bytes {

// Basic bswap functions using GCC/clang/MSVC intrinsics.
#ifdef ZEN_COMPILER_MSVC
#include <cstdlib>
constexpr u8  bswap(u8 v)  { return v; }
static    u16 bswap(u16 v) { return _byteswap_ushort(v); }
static    u32 bswap(u32 v) { return _byteswap_ulong(v); }
static    u64 bswap(u64 v) { return _byteswap_uint64(v); }
#else
constexpr u8  bswap(u8 v)  { return v; }
static    u16 bswap(u16 v) { return __builtin_bswap16(v); }
static    u32 bswap(u32 v) { return __builtin_bswap32(v); }
static    u64 bswap(u64 v) { return __builtin_bswap64(v); }
#endif

// Load an integer from memory
template <class T>
static T load(const void* Ptr) {
    static_assert(std::is_integral<T>::value, "T must be an integer!");
    T Ret;
    memcpy(&Ret, Ptr, sizeof(T));
    return Ret;
}

// Store an integer to memory
template <class T>
static void store(void* Ptr, const T V) {
    static_assert(std::is_integral<T>::value, "T must be an integer!");
    memcpy(Ptr, &V, sizeof(V));
}


// Decodes little-endian input (u8) into any-endian output (T). Assumes len is a multiple of sizeof(T).
template<typename T>
constexpr void decode(T* output, const u8* input, usize len) 
{
    static_assert(!std::is_same_v<T, u8>, "use memcpy to copy single bytes");
    static_assert(std::is_same_v<T, u16> || std::is_same_v<T, u32> || std::is_same_v<T, u64>, "T must be an unsigned integer");

    #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        #define BYTE_INDEX(x) (x)
    #else
        #define BYTE_INDEX(x) (sizeof(T) - x - 1)
    #endif

    for (usize i = 0, j = 0; j < len; i++, j += sizeof(T)) {
        output[i] = (T)input[j+BYTE_INDEX(0)];
        if constexpr(sizeof(T) > 1) {
            output[i] |= (((T)input[j+BYTE_INDEX(1)]) << 8);
            if constexpr(sizeof(T) > 2) {
                output[i] |= (((T)input[j+BYTE_INDEX(2)]) << 16) | (((T)input[j+BYTE_INDEX(3)]) << 24);
                if constexpr(sizeof(T) > 4) {
                    output[i] |= (((T)input[j+BYTE_INDEX(4)]) << 32) | (((T)input[j+BYTE_INDEX(5)]) << 40)
                              | (((T)input[j+BYTE_INDEX(6)]) << 48) | (((T)input[j+BYTE_INDEX(7)]) << 56);
                }
            }
        }
    }
    #undef BYTE_INDEX
}

// Encodes any-endian input (T) into little-endian output (u8). Assumes len is a multiple of sizeof(T).
template<typename T>
constexpr void encode(u8* output, const T* input, usize len) 
{
    static_assert(!std::is_same_v<T, u8>, "use memcpy to copy single bytes");
    static_assert(std::is_same_v<T, u16> || std::is_same_v<T, u32> || std::is_same_v<T, u64>, "T must be an unsigned integer");

    #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        #define BYTE_INDEX(x) (x)
    #else
        #define BYTE_INDEX(x) (sizeof(T) - x - 1)
    #endif

    for (usize i = 0, j = 0; j < len; i++, j += sizeof(T)) {
        output[j+BYTE_INDEX(0)] = input[i] & 0xff;
        if constexpr(sizeof(T) > 1) {
            output[j+BYTE_INDEX(1)] = (input[i] >> 8) & 0xff;
            if constexpr(sizeof(T) > 2) {
                output[j+BYTE_INDEX(2)] = (input[i] >> 16) & 0xff;
                output[j+BYTE_INDEX(3)] = (input[i] >> 24) & 0xff;
                if constexpr(sizeof(T) > 4) {
                    output[j+BYTE_INDEX(4)] = (input[i] >> 32) & 0xff;
                    output[j+BYTE_INDEX(5)] = (input[i] >> 40) & 0xff;
                    output[j+BYTE_INDEX(6)] = (input[i] >> 48) & 0xff;
                    output[j+BYTE_INDEX(7)] = (input[i] >> 56) & 0xff;
                }
            }
        }
    }
    #undef BYTE_INDEX
}

}

#endif // ZEN_BSWAP_H

#ifndef ZEN_MACROS_H
#define ZEN_MACROS_H


// Ye' Olde stringify
#define ZEN_STRINGIFY(x)    _stringify_impl(x)
#define _stringify_impl(x)  #x

// Basic identity macro
#ifndef IDENTITY
#define IDENTITY(x) x
#endif

// Preprocessor pack value extraction
//      PACK_GET(2, (a, b, c, d))   ->      c
#define PACK_GET(N, v) _pack_get ## N v
#define _pack_get0(x0, ...) x0
#define _pack_get1(x0, x1, ...) x1
#define _pack_get2(x0, x1, x2, ...) x2
#define _pack_get3(x0, x1, x2, x3, ...) x3
#define _pack_get4(x0, x1, x2, x3, x4, ...) x4
#define _pack_get5(x0, x1, x2, x3, x4, x5, ...) x5
#define _pack_get6(x0, x1, x2, x3, x4, x5, x6, ...) x6
#define _pack_get7(x0, x1, x2, x3, x4, x5, x6, x7, ...) x7


// For each macro (4 variants)
//    FOR_EACH(F, ...)              ->      F(index, value)...
//    FOR_EACH_ARG(F, A, ...)       ->      F(A, index, value)...
//    FOR_EACH_FOLD(F, S, ...)      ->      F(index, value) S ...
//    FOR_EACH_COMMA(F, ...)        ->      F(index, value) , ...
#ifdef ZEN_CPP20

#define FOR_EACH(macro, ...)            __VA_OPT__(_expand(_for_each_helper(macro, 0, __VA_ARGS__)))
#define FOR_EACH_ARG(macro, arg, ...)   __VA_OPT__(_expand(_for_each_arg_helper(macro, arg, 0, __VA_ARGS__)))
#define FOR_EACH_FOLD(macro, sep, ...)  __VA_OPT__(_expand(_for_each_fold_helper(macro, sep, 0, __VA_ARGS__)))
#define FOR_EACH_COMMA(macro, ...)      __VA_OPT__(_expand(_for_each_cma_helper(macro, 0, __VA_ARGS__)))

#define _for_each_again() _for_each_helper
#define _for_each_helper(macro, i, a1, ...) \
  macro(i, a1)                              \
  __VA_OPT__(_for_each_again _parens (macro, (i) + 1, __VA_ARGS__))


#define _for_each_arg_again() _for_each_arg_helper
#define _for_each_arg_helper(macro, arg, i, a1, ...) \
  macro(arg, i, a1)                                  \
  __VA_OPT__(_for_each_arg_again _parens (macro, arg, (i) + 1, __VA_ARGS__))


#define _for_each_fold_again() _for_each_fold_helper
#define _for_each_fold_helper(macro, sep, i, a1, ...) \
  macro(i, a1)                                       \
  __VA_OPT__(sep _for_each_fold_again _parens (macro, sep, (i) + 1, __VA_ARGS__))


#define _for_each_cma_again() _for_each_cma_helper
#define _for_each_cma_helper(macro, i, a1, ...) \
  macro(i, a1)                                  \
  __VA_OPT__(, _for_each_cma_again _parens (macro, (i) + 1, __VA_ARGS__))


#define _parens ()

#else

#define FOR_EACH(macro, ...)            _expand(_for_each1(macro, 0, __VA_ARGS__, ()()(), ()()(), ()()(), 0))
#define FOR_EACH_ARG(macro, arg, ...)   _expand(_for_each_arg1(macro, arg, 0, __VA_ARGS__, ()()(), ()()(), ()()(), 0))
#define FOR_EACH_FOLD(macro, sep, ...)  _expand(_for_each_fold1(macro, sep, 0, __VA_ARGS__, ()()(), ()()(), ()()(), 0))
#define FOR_EACH_COMMA(macro, ...)      _expand(_for_each_cma1(macro, 0, __VA_ARGS__, ()()(), ()()(), ()()(), 0))

#define _for_each_end(...)
#define _for_each_out
#define _for_each_cma ,

#define _for_each_get_end2()              0, _for_each_end
#define _for_each_get_end1(...)           _for_each_get_end2
#define _for_each_get_end(...)            _for_each_get_end1
#define _for_each_next0(test, next, ...)  next _for_each_out

#define _for_each_next1(test, next)             _for_each_next0(test, next, 0)
#define _for_each_next(test, next)              _for_each_next1(_for_each_get_end test, next)
#define _for_each0(f, i, x, peek, ...)          f(i, x) _for_each_next(peek, _for_each1)(f, (i) + 1, peek, __VA_ARGS__)
#define _for_each1(f, i, x, peek, ...)          f(i, x) _for_each_next(peek, _for_each0)(f, (i) + 1, peek, __VA_ARGS__)

#define _for_each_cma_next1(test, next)         _for_each_next0(test, _for_each_cma next, 0)
#define _for_each_cma_next(test, next)          _for_each_cma_next1(_for_each_get_end test, next)
#define _for_each_cma0(f, i, x, peek, ...)      f(i, x) _for_each_cma_next(peek, _for_each_cma1)(f, (i) + 1, peek, __VA_ARGS__)
#define _for_each_cma1(f, i, x, peek, ...)      f(i, x) _for_each_cma_next(peek, _for_each_cma0)(f, (i) + 1, peek, __VA_ARGS__)

#define _for_each_arg_next1(test, next)         _for_each_next0(test, next, 0)
#define _for_each_arg_next(test, next)          _for_each_arg_next1(_for_each_get_end test, next)
#define _for_each_arg0(f, a, i, x, peek, ...)   f(a, i, x) _for_each_arg_next(peek, _for_each_arg1)(f, a, (i) + 1, peek, __VA_ARGS__)
#define _for_each_arg1(f, a, i, x, peek, ...)   f(a, i, x) _for_each_arg_next(peek, _for_each_arg0)(f, a, (i) + 1, peek, __VA_ARGS__)

#define _for_each_fold_next1(test, s, next)     _for_each_next0(test, s next, 0)
#define _for_each_fold_next(test, s, next)      _for_each_fold_next1(_for_each_get_end test, s, next)
#define _for_each_fold0(f, s, i, x, peek, ...)  f(i, x) _for_each_fold_next(peek, s, _for_each_fold1)(f, s, (i) + 1, peek, __VA_ARGS__)
#define _for_each_fold1(f, s, i, x, peek, ...)  f(i, x) _for_each_fold_next(peek, s, _for_each_fold0)(f, s, (i) + 1, peek, __VA_ARGS__)

#endif
#define _expand(...) _expand4(_expand4(_expand4(_expand4(__VA_ARGS__))))
#define _expand4(...) _expand3(_expand3(_expand3(_expand3(__VA_ARGS__))))
#define _expand3(...) _expand2(_expand2(_expand2(_expand2(__VA_ARGS__))))
#define _expand2(...) _expand1(_expand1(_expand1(_expand1(__VA_ARGS__))))
#define _expand1(...) __VA_ARGS__


#endif // ZEN_MACROS_H

#ifndef ZEN_ENUM_H
#define ZEN_ENUM_H


namespace zen {

namespace impl {

template<typename Enum>
struct enum_name;

template<typename Enum>
struct enum_size;

}

template<typename Enum>
static constexpr usize enum_size = impl::enum_size<Enum>::value;

template<typename Enum>
static constexpr const char* enum_name(Enum v) noexcept { return impl::enum_name<Enum>::name(v); }


#define ZEN_ENUM(Name, ...) \
    _enum(Name, _pack_get1, __VA_ARGS__); \
    _enum_name(Name, _enum_name_helper_name_only, __VA_ARGS__)


#define ZEN_ENUM_VALUES(Name, ...) \
    _enum(Name, _enum_helper_name_value, __VA_ARGS__); \
    _enum_name(Name, _enum_name_helper_name_value, __VA_ARGS__)


#define ZEN_ENUM_FLAG(Name, first, ...) \
    enum class Name { first = 0, FOR_EACH_COMMA(_enum_helper_flag, __VA_ARGS__) }; \
    _enum_flag(Name, first, _enum_flag_debug_helper_name_only, __VA_ARGS__)


#define ZEN_ENUM_FLAG_VALUES(Name, first, ...) \
    enum class Name { first = 0, FOR_EACH_COMMA(_enum_helper_name_value, __VA_ARGS__) }; \
    _enum_flag(Name, first, _enum_flag_debug_helper_name_value, __VA_ARGS__)


#define _enum_helper_name_value(i, pack)                    PACK_GET(0, pack) = PACK_GET(1, pack)
#define _enum_helper_flag(i, pack)                          pack              = 1u << (i)
#define _enum_name_helper_name_only(Name, i, pack)          case Name::pack: return ZEN_STRINGIFY(pack);
#define _enum_name_helper_name_value(Name, i, pack)         _enum_name_helper_name_only(Name, i, PACK_GET(0, pack))
#define _enum_flag_debug_helper_name_only(Name, i, pack)    if (static_cast<u64>(v & Name::pack) != 0) { if ((idx++) > 0) { o << " | "; } o << ZEN_STRINGIFY(pack); }
#define _enum_flag_debug_helper_name_value(Name, i, pack)   _enum_flag_debug_helper_name_only(Name, i, PACK_GET(0, pack))

#define _enum(Name, helper, ...) \
    enum class Name { FOR_EACH_COMMA(helper, __VA_ARGS__) }; \
    template<> struct zen::impl::enum_size<Name> { \
        static constexpr usize value = zen::impl::enum_arg_count( FOR_EACH_COMMA(_pack_get0, __VA_ARGS__) ); \
    }

#define _enum_name(Name, helper, ...) \
    template<typename Out> Out& operator<<(Out& o, Name v) noexcept { return o << zen::enum_name<Name>(v); } \
    template<> struct zen::impl::enum_name<Name> { \
        static constexpr const char* name(Name v) { \
            switch (v) { \
                FOR_EACH_ARG(helper, Name, __VA_ARGS__) \
                default: return ZEN_STRINGIFY(Name) " - Unknown enum value"; \
            } \
        } \
    }

#define _enum_flag(Name, first, debug_helper, ...) \
    ZEN_FORCEINLINE static constexpr Name operator|(Name a, Name b) { return Name(static_cast<u64>(a) | static_cast<u64>(b)); } \
    ZEN_FORCEINLINE static constexpr Name operator&(Name a, Name b) { return Name(static_cast<u64>(a) & static_cast<u64>(b)); } \
    ZEN_FORCEINLINE static constexpr Name operator|=(Name& a, Name b) { return a = (a | b); } \
    ZEN_FORCEINLINE static constexpr Name operator&=(Name& a, Name b) { return a = (a & b); } \
    template<typename Out> Out& operator<<(Out& o, Name v) noexcept {  \
        if (v == Name::first) { \
            return o << ZEN_STRINGIFY(first); \
        } else { \
            usize idx{}; \
            FOR_EACH_ARG(debug_helper, Name, __VA_ARGS__) \
            return o; \
        } \
    }

namespace impl {

template<typename... Args>
static constexpr usize enum_arg_count(Args...) { return sizeof...(Args); }

}

}

#endif // ZEN_ENUM_H

#ifndef ZEN_STRING_H
#define ZEN_STRING_H

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

#ifndef ZEN_FMT_H
#define ZEN_FMT_H

#include <cstdio>

// Debugger trigger for zen::fmt::impl::assert_fail
#ifdef ZEN_COMPILER_MSVC
extern "C" void DebugBreak();
#else
#include <csignal>
#endif

// Todo macro for indicating unfinished implementations
#define todo(name)          assertf(false, "TODO: implement " name)

// Formatted assert with fmt library
#ifdef _DEBUG
#define assertf(expr, ...)  (static_cast <bool>(expr) \
    ? void (0) \
    : zen::fmt::impl::assert_fail(__FILE__, __FUNCTION__, __LINE__, #expr, __VA_ARGS__ ))
#else
#define assertf(...)
#endif

// Really Microsoft?
#ifdef ZEN_COMPILER_MSVC
#pragma warning(disable:4996)
#endif

namespace zen {

namespace fmt {

static constexpr usize DEFAULT_SIZE = 512;

template<usize N = DEFAULT_SIZE> 
struct buffer;
}

template<typename Out, typename... Args>
Out& format(Out& out, string_view fmt, Args&&... args) noexcept;


template<usize BufferSize=fmt::DEFAULT_SIZE, typename... Args>
void print(string_view fmt, Args&&... args) noexcept;


template<usize BufferSize=fmt::DEFAULT_SIZE, typename... Args>
void println(string_view fmt, Args&&... args) noexcept;


template<usize BufferSize=fmt::DEFAULT_SIZE, typename... Args>
ZEN_NORETURN
void panic(string_view fmt, Args&&... args) noexcept;


// Buffer / fmt fwd
namespace fmt {

template<typename T> struct binary    { T value{}; };
template<typename T> struct hex       { T value{}; };
template<typename T> struct hexu      { T value{}; };
template<typename T> struct octal     { T value{}; };
template<typename T> struct precisev  { T value{}; u8 precision{}; };


template<typename Type>
ZEN_ND constexpr string_view type_name() noexcept;


template<usize Base = 10, typename T>
constexpr bool chars_to_int(const char* begin, const char* end, T& value, num::index_t<Base> = {}) noexcept;


template<usize Base = 10, typename T>
char* int_to_chars(char* begin, char* end, const T& value, num::index_t<Base> = {}) noexcept;


template<typename T>
char* float_to_chars(char* begin, char* end, const T& value, usize precision = 0) noexcept;

namespace impl {
    
static constexpr char STYLE_NONE = 'i';
static constexpr usize HEX_UPPER = 0b100000000;

}

template<usize N>
struct buffer : sstring<N> {
    using sstring<N>::append;
    using sstring<N>::set_size;
    using sstring<N>::begin;
    using sstring<N>::end;
    using sstring<N>::max_size;
    using sstring<N>::size_type;
    
    buffer() = default;

    buffer& operator<<(char c)                 noexcept { append(c); return *this; }
    buffer& operator<<(const char* data)       noexcept { append(data, strlen(data)); return *this; }
    buffer& operator<<(string_view s)          noexcept { append(s.data(), s.size()); return *this; }

    template<usize M>
    buffer& operator<<(const char (&data)[M])  noexcept { append(data, M - 1); return *this; }
    
    template<typename U, typename = std::enable_if_t<std::is_same_v<const char*, decltype( std::declval<const U&>().data() + std::declval<U>().size() )>>>
    buffer& operator<<(U&& str_like)           noexcept { append(str_like.data(), str_like.size()); return *this; }
    
    buffer& operator<<(bool v)                 noexcept { append(v ? "true" : "false", 5 - v); return *this; }
    buffer& operator<<(u8 v)                   noexcept { set_size(size_type(int_to_chars(end(), begin() + max_size(), u16(v)) - begin())); return *this; }
    buffer& operator<<(u16 v)                  noexcept { set_size(size_type(int_to_chars(end(), begin() + max_size(), v) - begin())); return *this; }
    buffer& operator<<(u32 v)                  noexcept { set_size(size_type(int_to_chars(end(), begin() + max_size(), v) - begin())); return *this; }
    buffer& operator<<(u64 v)                  noexcept { set_size(size_type(int_to_chars(end(), begin() + max_size(), v) - begin())); return *this; }
    buffer& operator<<(i8 v)                   noexcept { set_size(size_type(int_to_chars(end(), begin() + max_size(), i16(v)) - begin())); return *this; }
    buffer& operator<<(i16 v)                  noexcept { set_size(size_type(int_to_chars(end(), begin() + max_size(), v) - begin())); return *this; }
    buffer& operator<<(i32 v)                  noexcept { set_size(size_type(int_to_chars(end(), begin() + max_size(), v) - begin())); return *this; }
    buffer& operator<<(i64 v)                  noexcept { set_size(size_type(int_to_chars(end(), begin() + max_size(), v) - begin())); return *this; }
    buffer& operator<<(f32 v)                  noexcept { set_size(size_type(float_to_chars(end(), begin() + max_size(), v) - begin())); return *this; }
    buffer& operator<<(f64 v)                  noexcept { set_size(size_type(float_to_chars(end(), begin() + max_size(), v) - begin())); return *this; }
    buffer& operator<<(const void* v)          noexcept { append("0x", 2); set_size(size_type(int_to_chars<16>(end(), begin() + max_size(), u64(reinterpret_cast<uintptr_t>(v))) - begin())); return *this; }

    template<typename T>
    buffer& operator<<(const binary<T>& v)     noexcept { append("0b", 2); set_size(size_type(int_to_chars<2>(end(), begin() + max_size(), v.value) - begin())); return *this; }

    template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
    buffer& operator<<(const hex<T>& v)        noexcept { append("0x", 2); set_size(size_type(int_to_chars<16>(end(), begin() + max_size(), v.value) - begin())); return *this; }

    template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
    buffer& operator<<(const hexu<T>& v)       noexcept { append("0x", 2); set_size(size_type(int_to_chars<16 | impl::HEX_UPPER>(end(), begin() + max_size(), v.value) - begin())); return *this; }
    
    template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
    buffer& operator<<(const octal<T>& v)      noexcept { append("0o", 2); set_size(size_type(int_to_chars<8>(end(), begin() + max_size(), v.value) - begin())); return *this; }

    template<typename T, typename = std::enable_if_t<std::is_floating_point_v<T>>>
    buffer& operator<<(const precisev<T>& v)   noexcept { set_size(size_type(float_to_chars(end(), begin() + max_size(), v.value, usize(v.precision)) - begin())); return *this; }
};

}

// Format impl
namespace fmt::impl {

constexpr bool format_parse_spec(string_view spec, char& style, char& fill, char& align, usize& n, usize& precision) {
    // Binary
    // {b:}
    // Hex
    // {x:}
    // Hex upper
    // {X:}
    // Octal
    // {o:}
    // General
    // {:}
    // {:5}
    // {:<}   ERROR
    // {:<5}
    // {:X<}  ERROR
    // {:X<5}
    // Float
    // {:.}   ERROR
    // {:.2}
    // {:<.}  ERROR
    // {:<.2}
    // {:X<.} ERROR
    // {:X<.2}
    const auto sep = spec.find_first_of(':');
    if (sep == string_view::npos)
        return false;
    
    if (sep > 0) {
        if (sep > 1) 
            return false;
        style = spec[sep - 1];
    }
    
    string_view n_str{};
    if (const auto alg = spec.find_first_of("<>^", sep); alg != string_view::npos) {
        align = spec[alg];
        n_str = {spec.data() + alg + 1, spec.size() - alg - 1};
        if (spec[alg - 1] != ':')
            fill = spec[alg - 1];
    } 
    else if (sep < spec.size() - 1) {
        const auto c = spec[sep + 1];
        if ((c >= '0' && c <= '9') || c == '.') {
            n_str = {spec.data() + sep + 1, spec.size() - sep - 1};
        } else {
            fill = c;
            n_str = {spec.data() + sep + 2, spec.size() - sep - 2};
        }
    }
    const bool is_precision = !n_str.empty() && n_str[0] == '.';
    if (is_precision) {
        n_str = {n_str.data() + 1, n_str.size() - 1};
        style = 'f';
    }
    if (!chars_to_int(n_str.data(), n_str.data() + n_str.size(), n)) {
        return false;
    }
    if (is_precision) {
        precision = n;
        n = 0;
    }
    return true;
}

template<typename Out, typename T>
void format_with_style(Out& out, char style, usize precision, T&& value) noexcept {
    using U = std::remove_reference_t<T>;
    if (ZEN_LIKELY(style == STYLE_NONE)) {
        out << value;
    } else {
        if constexpr(std::is_integral_v<U> && !std::is_same_v<U, bool>) {
            // panic if precision specified
            switch (style) {
                case 'b': out << binary<U>{ZEN_FWD(value)}; return;
                case 'x': out << hex<U>{ZEN_FWD(value)}; return;
                case 'X': out << hexu<U>{ZEN_FWD(value)}; return;
                case 'o': out << octal<U>{ZEN_FWD(value)}; return;
                default: out << "InvalidSpecifier(" << style << ")"; return;
            }
        } 
        else if constexpr(std::is_floating_point_v<U>) {
            // paanic if style != f
            // panic if type specified
            out << precisev<U>{ZEN_FWD(value), u8(precision)};
        }
        else {
            // panic invalid format specifier for type
        }
    }
}

template<typename Out, typename T>
void format_part(Out& out, string_view spec, T&& value) noexcept {
    if (ZEN_LIKELY(spec.empty())) {
        out << value;
    } else {
        usize n{}, precision{};
        char style{STYLE_NONE}, fill{' '}, align{'<'};
        if (!format_parse_spec(spec, style, fill, align, n, precision)) {
            // panic fail
        }
        if (ZEN_LIKELY(align == '<')) {
            const usize o = out.size();
            format_with_style(out, style, precision, ZEN_FWD(value));
            const usize used = out.size() - o;
            const usize remaining = n >= used ? n - used : 0;
            for (usize i = 0; i < remaining; ++i) out << fill;
        } else {
            buffer<512> tmp{};
            format_with_style(tmp, style, precision, ZEN_FWD(value));
            const usize remaining = n >= tmp.size() ? n - tmp.size() : 0;
            const usize after = align == '^' ? (remaining / 2) : 0;
            for (usize i = 0; i < remaining - after; ++i) out << fill;
            out << string_view(tmp);
            for (usize i = 0; i < after; ++i) out << fill;
        }
    }
}

template<typename Out, typename T, typename... Rest>
void format(Out& out, const string_view& fmt, T&& value, Rest&&... rest) noexcept {    
    usize offset{};
    bool inside_format = false;
    usize begin_spec{};
    for (; offset < fmt.size(); ++offset) {
        const auto c = fmt[offset];
        if (c == '{') {
            if (offset + 1 < fmt.size() && fmt[offset + 1] == '{') {
                out << '{';
                offset++;
                continue;
            } else {
                inside_format = true;
                begin_spec = offset + 1;
                continue;
            }
        }
        else if (c == '}') {
            if (offset + 1 < fmt.size() && fmt[offset + 1] == '}') {
                out << '}';
                offset++;
                continue;
            } else {
                format_part(out, string_view{fmt.data() + begin_spec, offset - begin_spec}, ZEN_FWD(value));
                offset++;
                inside_format = false;
                break;
            }
        }
        if (!inside_format) {
            out << c;
        }
    }
    if constexpr (sizeof...(Rest) > 0) {
        format(out, string_view{fmt.data() + offset, fmt.size() - offset}, ZEN_FWD(rest)...);
    } else {
        for (; offset < fmt.size(); ++offset) {
            const char c = fmt[offset];
            if (c == '{') {
                offset += 1 + (offset + 1 < fmt.size() && fmt[offset + 1] == '{');
                out << '{';
            }
            else if (c == '}') {
                offset += 1 + (offset + 1 < fmt.size() && fmt[offset + 1] == '{');
                out << '}';
            } 
            else {
                out << c;
            }
        }
        // out << string_view{fmt.data() + offset, fmt.size() - offset};
    }
}

template<typename... Args>
ZEN_NORETURN static void assert_fail(const char* file, const char* function, int line, const char* expr, Args&&... args) 
{
    if constexpr(sizeof...(Args) > 0) {
        buffer<4096> buf{};
        format(buf, ZEN_FWD(args)...);
        fprintf(stderr, "%s:%d: %s: Assertion `%s` failed. %s\n", file, line, function, expr, buf.data());
    } else {
        fprintf(stderr, "%s:%d: %s: Assertion `%s` failed.\n", file, line, function, expr);
    }
    #if defined(ZEN_COMPILER_MSVC)
        DebugBreak();
    #elif defined(SIGTRAP)
        raise(SIGTRAP);
    #else
        raise(SIGABRT);
    #endif
    exit(1);
}

}

// Number string conversion
namespace fmt {

template<usize Base, typename T>
constexpr bool chars_to_int(const char* begin, const char* end, T& value, num::index_t<Base>) noexcept
{
    if ((end - begin) == 1) {
        value = T(*begin - '0');
    } else {
        value = 0;
        constexpr T mult = Base;
        for (auto it = begin; it != end; ++it) {
            const auto c = *it;
            value += T(c - '0');
            value *= mult;
        }
        value /= mult;
    }
    return true;
}

template<usize Base, typename T>
char* int_to_chars(char* begin, char* end, const T& value, num::index_t<Base>) noexcept
{
    auto val = value;
    if constexpr(std::is_signed_v<T>) {
        if (val < 0) { *begin++ = '-'; val = -val; }
    }
    
    // Super-fast int formatter for base 10 (most common case)
    if constexpr(Base == 10) {
        static u16 const str100p[100] = {
            0x3030, 0x3130, 0x3230, 0x3330, 0x3430, 0x3530, 0x3630, 0x3730, 0x3830, 0x3930,
            0x3031, 0x3131, 0x3231, 0x3331, 0x3431, 0x3531, 0x3631, 0x3731, 0x3831, 0x3931,
            0x3032, 0x3132, 0x3232, 0x3332, 0x3432, 0x3532, 0x3632, 0x3732, 0x3832, 0x3932,
            0x3033, 0x3133, 0x3233, 0x3333, 0x3433, 0x3533, 0x3633, 0x3733, 0x3833, 0x3933,
            0x3034, 0x3134, 0x3234, 0x3334, 0x3434, 0x3534, 0x3634, 0x3734, 0x3834, 0x3934,
            0x3035, 0x3135, 0x3235, 0x3335, 0x3435, 0x3535, 0x3635, 0x3735, 0x3835, 0x3935,
            0x3036, 0x3136, 0x3236, 0x3336, 0x3436, 0x3536, 0x3636, 0x3736, 0x3836, 0x3936,
            0x3037, 0x3137, 0x3237, 0x3337, 0x3437, 0x3537, 0x3637, 0x3737, 0x3837, 0x3937,
            0x3038, 0x3138, 0x3238, 0x3338, 0x3438, 0x3538, 0x3638, 0x3738, 0x3838, 0x3938,
            0x3039, 0x3139, 0x3239, 0x3339, 0x3439, 0x3539, 0x3639, 0x3739, 0x3839, 0x3939, };

        u16 buffer[11];
        u16 *p = &buffer[10];        
        while(val >= 100) {
            const auto old = val;
            --p;
            val /= 100;
            *p = str100p[old - (val * 100)];
        }
        *(--p) = str100p[val];
        const auto* b = reinterpret_cast<const char*>(p) + (val < 10);
        const auto n = reinterpret_cast<const char*>(buffer + 10) - b;        
        assert((end - begin) >= n && "Not enough space to format integer");
        memcpy(begin, b, n);
        return begin + n;
    }
    // Regular int formatter for other cases
    else {
        static constexpr T DIVISOR           = Base & 0xff;
        static constexpr bool USE_UPPER      = DIVISOR == 16 && ((Base & ~UINT64_C(0xff)) == impl::HEX_UPPER);
        static constexpr char OUTPUT_CHARS[] = "0123456789abcdef0123456789ABCDEF";
        static constexpr const char* OUTPUT  = &OUTPUT_CHARS[USE_UPPER ? 16 : 0];
        char buffer[65];
        char* p = &buffer[64];
        while (val > 0) {
            const auto old = val;
            --p;
            val /= DIVISOR;
            *p = OUTPUT[old - (val * DIVISOR)];
        }
        const auto n = buffer + 64 - p;
        assert((end - begin) >= n && "Not enough space to format integer");
        memcpy(begin, p, n);
        return begin + n;
    }
}

template<typename T>
char* float_to_chars(char* begin, char* end, const T& value, usize precision) noexcept
{
    char fmt[16]{'%'};
    char* pfmt = fmt + 1;
    if (ZEN_UNLIKELY(precision != 0)) {
        *pfmt++ = '.';
        pfmt = int_to_chars(pfmt, fmt + sizeof(fmt), precision);
    }
    *pfmt++ = precision ? 'f' : 'g';
    return begin + snprintf(begin, end - begin, fmt, value);
}

}

// API impl
template<typename Out, typename... Args>
Out& format(Out& out, string_view fmt, Args&&... args) noexcept
{
    if constexpr(sizeof...(Args) > 0) {
        fmt::impl::format(out, fmt, ZEN_FWD(args)...);
    } else {
        out << fmt;
    }
    return out;
}


template<usize BufferSize, typename... Args>
void print(string_view fmt, Args&&... args) noexcept
{
    fmt::buffer<BufferSize> out{};
    format(out, fmt, ZEN_FWD(args)...);
    fprintf(stdout, "%s", string_view(out).data());
}


template<usize BufferSize, typename... Args>
void println(string_view fmt, Args&&... args) noexcept
{
    fmt::buffer<BufferSize> out{};
    format(out, fmt, ZEN_FWD(args)...);
    fprintf(stdout, "%s\n", string_view(out).data());
}


template<usize BufferSize, typename... Args>
void panic(string_view fmt, Args&&... args) noexcept
{
    fmt::buffer<BufferSize> out{};
    format(out, fmt, ZEN_FWD(args)...);
    fprintf(stderr, "%s\n", string_view(out).data());
    exit(1);
}


// Generic output operators for containers
template<typename Out, typename T0, typename T1>
Out& operator<<(Out& o, const pair<T0, T1>& v) noexcept {
    return o << '{' << v.first << ", " << v.second << '}';
}

template<typename Out, typename T, typename = std::void_t<decltype(std::declval<T>().begin() == std::declval<T>().end())>>
Out& operator<<(Out& o, const T& v) noexcept {    
    if constexpr(std::is_constructible_v<T, const char*>) {
        return o << string_view{v.data(), v.size()};        
    } else {
        o << '{';
        bool comma = false;
        auto end = v.end();
        for (auto it = v.begin(); it != end; ++it) {
            if (comma) { o << ", "; }
            comma = true;
            o << *it;
        }
        return o << '}';
    }
}
    

// Type name impl
namespace impl {

template<typename Type>
constexpr const char* type_name(usize& n) noexcept 
{
    // NOTE: In order for this trick to work, the only template allowed 
    // in the function signature is a user template. Therefore, to return
    // a string view (a template), we need a primitive helper function
    // that uses the function name trick to get the type name, which is
    // then used by the actual function to return a usable value
    #if defined(ZEN_COMPILER_GCC) || defined(ZEN_COMPILER_CLANG)
        #define ZEN_PRETTY_FUNCTION __PRETTY_FUNCTION__
        #define ZEN_PRETTY_FUNCTION_PREFIX '='
        #define ZEN_PRETTY_FUNCTION_SUFFIX ']'
    #elif defined(ZEN_COMPILER_MSVC)
        #define ZEN_PRETTY_FUNCTION __FUNCSIG__
        #define ZEN_PRETTY_FUNCTION_PREFIX '<'
        #define ZEN_PRETTY_FUNCTION_SUFFIX '>'
    #endif

    #ifndef ZEN_PRETTY_FUNCTION
    return "";
    #else
    string_view pretty_function{ZEN_PRETTY_FUNCTION};
    auto first = pretty_function.find_first_not_of(' ', pretty_function.find_first_of(ZEN_PRETTY_FUNCTION_PREFIX) + 1);
    auto value = pretty_function.substr(first, pretty_function.find_last_of(ZEN_PRETTY_FUNCTION_SUFFIX) - first);
    n = value.size();
    return value.data();
    #endif

    #undef ZEN_PRETTY_FUNCTION
}

}

template<typename Type>
constexpr string_view type_name() noexcept 
{
    usize n{};
    const char* c = impl::type_name<Type>(n);
    return string_view{c, n};
}

}

#endif // ZEN_FMT_H

#ifndef ZEN_HANDLE_H
#define ZEN_HANDLE_H


namespace zen {

// Handle define macros
#define ZEN_DEFINE_HANDLE(name, type)             using name = handle<type, 0, struct name ## tag>
#define ZEN_DEFINE_HANDLE_INFO(name, type, ninfo) using name = handle<type, ninfo, struct name ## tag>


// Handle with info bits
template<typename T, usize NInfoBits = 0, typename Tag = void>
struct handle {
    using type = T;

    ZEN_FORCEINLINE constexpr handle() : value{INVALID}, info{0} {}
    ZEN_FORCEINLINE constexpr handle(T id, T info) noexcept : value{id}, info{info} {}
    ZEN_FORCEINLINE explicit constexpr handle(T id) noexcept : value{id}, info{0} {}

    ZEN_FORCEINLINE          constexpr operator T   () const noexcept { return value; }
    ZEN_FORCEINLINE explicit constexpr operator bool() const noexcept { return value != INVALID; }

    ZEN_FORCEINLINE constexpr T      info()          const noexcept { return info; }
    ZEN_FORCEINLINE constexpr T      value()         const noexcept { return value; }
    ZEN_FORCEINLINE constexpr bool   valid()         const noexcept { return value != INVALID; }
    ZEN_FORCEINLINE constexpr handle with_info(T i)  const noexcept { return handle{value, i}; }
    ZEN_FORCEINLINE constexpr handle with_value(T v) const noexcept { return handle{v, info}; }

    ZEN_FORCEINLINE constexpr bool operator==(const handle& h) const noexcept { return value == h.value && info == h.info; }
    ZEN_FORCEINLINE constexpr bool operator!=(const handle& h) const noexcept { return value != h.value || info != h.info; }

private:
    static constexpr usize NBits = sizeof(T) * 8;
    static constexpr T INVALID   = 0;
    T value: NBits - NInfoBits;
    T info : NInfoBits;
};


// Handle with no info bits
template<typename T, typename Tag>
struct handle<T, 0, Tag> {
    using type = T;

    ZEN_FORCEINLINE constexpr handle() noexcept : value{INVALID} {}
    ZEN_FORCEINLINE explicit constexpr handle(T id) noexcept : value{id} {}

             constexpr operator T   () const noexcept { return value; }
    explicit constexpr operator bool() const noexcept { return value != INVALID; }

    constexpr T    value() const noexcept { return value; }
    constexpr bool valid() const noexcept { return value != INVALID; }

    ZEN_FORCEINLINE constexpr bool operator==(const handle& h) const noexcept { return value == h.value; }
    ZEN_FORCEINLINE constexpr bool operator!=(const handle& h) const noexcept { return value != h.value; }

private:
    static constexpr T INVALID   = T(UINT64_MAX);
    T value{};
};

}

#endif // ZEN_HANDLE_H

#ifndef ZEN_RESULT_H
#define ZEN_RESULT_H


namespace zen {

// Short circuit result evaluation
//      auto r0 = f();
//      RESULT_CHECK(r0);
//      r0.value() ...
//      auto r1 = f();
//      RESULT_CHECK(r1);
//      r1.value() ...
#define RESULT_CHECK(r) if (!r.ok()) { return r.code(); }


namespace impl {

template<typename Code>
struct result_success { static constexpr Code value = Code(0); };

template<>
struct result_success<bool> { static constexpr bool value = true; };

template<typename Code, typename = void>
struct result_code_is_integral: std::false_type{};

template<typename Code>
struct result_code_is_integral<Code, std::enable_if_t<std::is_integral_v<Code>>>: std::true_type{};

template<typename Code>
struct result_code_is_integral<Code, std::enable_if_t<std::is_integral_v<std::underlying_type_t<Code>>>>: std::true_type{};

}

struct empty_t{};
struct error_t{};
static constexpr error_t error{};

template<typename T, typename Code = bool, Code Success = impl::result_success<Code>::value>
struct result;


template<typename Code = bool, Code Success = impl::result_success<Code>::value>
using empty_result = result<empty_t, Code, Success>;


template<typename T, typename Code, Code Success>
struct result {
    using value_type = T;
    using code_type  = Code;
    static_assert(!std::is_same_v<T, Code>, "T must not be the same type as Code");
    static_assert(impl::result_code_is_integral<Code>::value, "Code must derive from an integral type");

    ZEN_FORCEINLINE constexpr result()                          noexcept : v{}, c{} {}
    ZEN_FORCEINLINE constexpr result(const T& value)            noexcept : v{value}, c{Success} {}
    ZEN_FORCEINLINE constexpr result(T&&  value)                noexcept : v{ZEN_FWD(value)}, c{Success} {}
    
    template<typename = std::enable_if_t<!std::is_same_v<Code, bool>>>
    ZEN_FORCEINLINE constexpr result(Code code)                 noexcept : c{code} {}

    template<typename = std::enable_if_t<std::is_same_v<Code, bool>>>
    ZEN_FORCEINLINE constexpr result(error_t)                   noexcept : c{false} {}

    ZEN_FORCEINLINE constexpr result& operator=(const T& value) noexcept { v = value; c = Success; return *this; }
    ZEN_FORCEINLINE constexpr result& operator=(T&&  value)     noexcept { v = std::move(value); c = Success; return *this; }
    ZEN_FORCEINLINE constexpr result& operator=(Code code)      noexcept { c = code; return *this; }

    ZEN_FORCEINLINE constexpr operator bool()             const noexcept { return c == Success; }
    ZEN_FORCEINLINE constexpr bool ok()                   const noexcept { return c == Success; }
    ZEN_FORCEINLINE constexpr Code code()                 const noexcept { return c; }
    ZEN_FORCEINLINE constexpr T&&  value()                &&    noexcept { return std::move(v); }
    ZEN_FORCEINLINE constexpr Code get(T& val)            &&    noexcept { if (ok()) val = std::move(v); return c; }
    ZEN_FORCEINLINE constexpr void tie(T& val, Code& cd)  &&    noexcept { if (ok()) val = std::move(v); else cd = c; }

    friend ZEN_FORCEINLINE constexpr bool operator==(const result& l, const result& r) noexcept { return l.ok() == r.ok() ? (l.v == r.v) : (l.c == r.c); }
    friend ZEN_FORCEINLINE constexpr bool operator!=(const result& l, const result& r) noexcept { return !(l == r); }

    template<typename Out>
    friend Out& operator<<(Out& o, const result& r) noexcept {
        if (r.ok()) {
            return o << "Ok(" << r.v << ")";
        } else {
            if constexpr(std::is_same_v<Code, bool>)
                return o << "Err()";
            else
                return o << "Err(" << r.c << ")";
        }
    }

private:
    T    v;
    Code c;
};

}

#endif // ZEN_RESULT_H

#ifndef ZEN_SPAN_H
#define ZEN_SPAN_H


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

#ifndef ZEN_SVECTOR_H
#define ZEN_SVECTOR_H


namespace zen {

// If the type is defined, try to ensure vector fits inside of a cache line
template<typename T>
static constexpr usize expected_svector_capacity = num::sizeof_type<T> 
    ? ((ZEN_CACHE_LINE - sizeof(u64)) / num::sizeof_type<T>) 
    : 8;


template<typename T, usize N = expected_svector_capacity<T>, bool Overflow = true>
struct svector;


template<typename T, usize N = expected_svector_capacity<T>>
using fvector = svector<T, N, false>;


namespace impl {

template<typename T, usize N, bool CanOverflow, typename = void>
struct svec_base;

}

template<typename T, usize N, bool Overflow>
struct svector : impl::svec_base<T, N, Overflow> {
    using value_type = T;
    using reference = T&;
    using const_reference = const T&;
    using pointer = T*;
    using const_pointer = const T*;
    using iterator = T*;
    using const_iterator = const T*;
    using init_list = std::initializer_list<T>;
    using base_type = impl::svec_base<T, N, Overflow>;
    using size_type = typename base_type::size_type;
    using base_type::base_type;

    template<typename InputIt>
    svector(InputIt b, InputIt e) { insert(end(), b, e); }

    template<typename R, typename = std::void_t<decltype(std::declval<R>().begin() == std::declval<R>().end())>>
    svector(R r) : svector(r.begin(), r.end()) {}

    template<typename... Rest>
    svector(T&& v0, T&& v1, Rest&&... rest) : svector() { push_back(v0); push_back(v1); (push_back(rest), ...); }

    svector& operator=(init_list values)  { clear(); ensure_capacity(values.size()); for(auto& v: values) push_back(v); }

    svector(const svector& v);
    svector& operator=(const svector& v);

    svector(svector&& v) noexcept;
    svector& operator=(svector&& v) noexcept;
    
    ~svector() { mem::destroy_n(begin(), m_size); }

    constexpr usize           size()                const noexcept { return m_size; }
    constexpr bool            empty()               const noexcept { return m_size == 0; }
    constexpr bool            small()               const noexcept { return base_type::small(); }
    constexpr usize           capacity()            const noexcept { return base_type::capacity(); }
    constexpr pointer         data()                      noexcept { return base_type::data(); }
    constexpr const_pointer   data()                const noexcept { return base_type::data(); }

    constexpr reference       operator[](usize n)         noexcept { assertf(n < size(), "Index {} out of range ({})", n, size()); return data()[n]; }
    constexpr const_reference operator[](usize n)   const noexcept { assertf(n < size(), "Index {} out of range ({})", n, size()); return data()[n]; }
    constexpr reference       at(usize n)                 noexcept { assertf(n < size(), "Index {} out of range ({})", n, size()); return data()[n]; }
    constexpr const_reference at(usize n)           const noexcept { assertf(n < size(), "Index {} out of range ({})", n, size()); return data()[n]; }
    constexpr reference       front()                     noexcept { return *data(); }
    constexpr const_reference front()               const noexcept { return *data(); }
    constexpr reference       back()                      noexcept { return *(data() + m_size - 1); }
    constexpr const_reference back()                const noexcept { return *(data() + m_size - 1); }

    constexpr iterator        begin()                     noexcept { return data(); }
    constexpr const_iterator  begin()               const noexcept { return data(); }
    constexpr const_iterator  cbegin()              const noexcept { return data(); }
    constexpr iterator        end()                       noexcept { return data() + m_size; }
    constexpr const_iterator  end()                 const noexcept { return data() + m_size; }
    constexpr const_iterator  cend()                const noexcept { return data() + m_size; }

    inline void               clear() noexcept;
    inline void               reserve(size_type n);
    inline void               resize(size_type n);
    inline void               resize(size_type n, const value_type& value);

    template<typename... Args>
    inline reference          emplace_back(Args&&... args);
    template<typename... Args>
    inline reference          emplace(const_iterator position, Args&&... args);

    inline iterator           erase(const_iterator position);
    inline iterator           erase(const_iterator first, const_iterator last);

    inline iterator           insert(const_iterator position, value_type&& value);
    inline iterator           insert(const_iterator position, const value_type& value);
    inline iterator           insert(const_iterator position, size_type n, const value_type& value);

    template<typename It>
    inline iterator           insert(const_iterator position, It begin, It end);

    inline reference          push_back(const value_type& value);
    inline reference          push_back(value_type&& value);
    inline void               pop_back();

private:
    using base_type::m_size;
    using base_type::has_capacity;
    using base_type::ensure_capacity;
    using base_type::ensure_capacity_insert;
    using base_type::resize_shrink;
};


namespace impl {
    
template<typename T, bool CanOverflow>
struct svec_base<T, 0, CanOverflow> {
protected:
    static ZEN_FORCEINLINE constexpr bool has_capacity(usize) noexcept { return true; }
    static ZEN_FORCEINLINE constexpr void resize_shrink(usize) {}
    static ZEN_FORCEINLINE constexpr void reset_small() {}
    static ZEN_FORCEINLINE constexpr void ensure_capacity(usize) {}
    static ZEN_FORCEINLINE constexpr void ensure_capacity_insert(const T*, usize) {}

public:
    ZEN_FORCEINLINE constexpr svec_base() {}
    static ZEN_FORCEINLINE constexpr bool         small()           noexcept { return true; }
    static ZEN_FORCEINLINE constexpr usize        capacity()        noexcept { return 0; }
    static ZEN_FORCEINLINE constexpr T*           data()            noexcept { return nullptr; }
    static ZEN_FORCEINLINE constexpr const T*     data()            noexcept { return nullptr; }
    static ZEN_FORCEINLINE constexpr void         shrink_to_fit() {}
    static ZEN_FORCEINLINE constexpr void         swap(svec_base& other) noexcept {}
};

template<typename T, usize N>
struct svec_base<T, N, false, std::enable_if_t<N != 0>> {
    using size_type = num::with::max_value<N>;
protected:
    mem::aligned_buffer<T, N * sizeof(T)> m_buf{};
    size_type m_size{};

    static ZEN_FORCEINLINE constexpr bool has_capacity(usize) noexcept { return true; }
    static ZEN_FORCEINLINE constexpr void resize_shrink(usize) {}
    static ZEN_FORCEINLINE constexpr void reset_small() {}
    
    ZEN_FORCEINLINE constexpr void ensure_capacity(usize n) {
        assertf(m_size + n <= N, "vector is full");
    }

    ZEN_FORCEINLINE constexpr void ensure_capacity_insert(const T* position, usize n) {
        ensure_capacity(n);
        if (ZEN_UNLIKELY(position != data() + m_size)) {
            const usize i = position - data();
            mem::move_backward(data() + i, data() + m_size, data() + i + n);
        }
    }

public:
    ZEN_FORCEINLINE constexpr              svec_base() : m_size{0} {}
    ZEN_FORCEINLINE static constexpr bool  small()           noexcept { return true; }
    ZEN_FORCEINLINE static constexpr usize capacity()        noexcept { return N; }
    ZEN_FORCEINLINE constexpr T*           data()            noexcept { return m_buf; }
    ZEN_FORCEINLINE constexpr const T*     data()      const noexcept { return m_buf; }
    ZEN_FORCEINLINE constexpr void         shrink_to_fit() {}
    ZEN_FORCEINLINE constexpr void         swap(svec_base& other) noexcept {
        mem::swap_ranges(data(), m_size, other.data(), other.m_size);
        std::swap(m_size, other.m_size);
    }
};


template<typename T, usize N>
struct svec_base<T, N, true, std::enable_if_t<N != 0>> {
    using size_type = usize;
protected:
    T* m_data{};
    usize m_size{};
    mem::aligned_buffer<T, N * sizeof(T)> m_buf{};
    usize m_cap{};
    alloc_t<> alloc{};

    ZEN_FORCEINLINE constexpr bool has_capacity(usize n) const noexcept { 
        return (m_size + n) <= m_cap; 
    }

    ZEN_FORCEINLINE constexpr void ensure_capacity(usize n) {
        if (ZEN_LIKELY(m_size + n <= m_cap)) 
            return;

        usize new_cap = m_cap << 1;
        while (new_cap < m_size + n) 
            new_cap <<= 1;

        auto* mem = static_cast<T*>(static_cast<void*>(alloc.allocate(new_cap * sizeof(T))));
        mem::move(m_data, m_data + m_size, mem);
        if (!small()) 
            deallocate();

        m_data = mem;
        m_cap = new_cap;
    }

    ZEN_FORCEINLINE void ensure_capacity_insert(const T* position, usize n) {
        if (ZEN_LIKELY(position == data() + m_size)) {
            ensure_capacity(n);
        } else {
            const usize i = position - data();
            ensure_capacity(n);
            mem::move_backward(data() + i, data() + m_size, data() + i + n);
        }
    }

    ZEN_FORCEINLINE void resize_shrink(usize n) {
        if (ZEN_LIKELY(!small() && n <= N)) {
            T* dst = m_buf;
            mem::move(data(), data() + m_size, dst);
            deallocate();
            m_cap = N;
            m_data = dst;
        }
    }
    
    ZEN_FORCEINLINE constexpr void reset_small() {
        if (!small()) deallocate();
        m_data = m_buf;
        m_size = 0;
        m_cap = N;
    }

    ZEN_FORCEINLINE void deallocate() {
        alloc.deallocate(static_cast<u8*>(static_cast<void*>(m_data)), m_cap * sizeof(T));
    }

public:
    ZEN_FORCEINLINE constexpr           svec_base(alloc_t<> alloc = std::pmr::get_default_resource()) : m_data{m_buf}, m_size{0}, m_cap{N}, alloc{alloc} {}
    ZEN_FORCEINLINE constexpr bool      small()     const noexcept { return m_cap == N; }
    ZEN_FORCEINLINE constexpr usize     capacity()  const noexcept { return m_cap; }
    ZEN_FORCEINLINE constexpr T*        data()            noexcept { return m_data; }
    ZEN_FORCEINLINE constexpr const T*  data()      const noexcept { return m_data; }
    
    ZEN_FORCEINLINE constexpr void      shrink_to_fit() {
        if (ZEN_LIKELY(small() || m_size > N))
            return;
        T* buf = m_buf;
        mem::move(m_data, m_data + m_size, buf);
        deallocate();
        m_cap = N;
        m_data = buf;
    }
    
    ZEN_FORCEINLINE constexpr void      swap(svec_base& other) noexcept {
        if (small() && other.small()) {
            mem::swap_ranges(data(), m_size, other.data(), other.m_size);
            std::swap(m_size, other.m_size);
        }
        else if (!small() && !other.small()) {
            std::swap(m_data, other.m_data);
            std::swap(m_cap, other.m_cap);
            std::swap(m_size, other.m_size);
        }
        // other heap, this small
        else if (small()) {
            T* obuf = other.m_buf;
            mem::move(m_data, m_data + m_size, obuf);
            m_data = other.m_data;
            other.m_data = obuf;
            std::swap(m_cap, other.m_cap);
            std::swap(m_size, other.m_size);
        }
        // other small, this heap
        else {
            T* buf = m_buf;
            mem::move(m_data, m_data + m_size, buf);
            other.m_data = m_data;
            m_data = buf;
            std::swap(m_cap, other.m_cap);
            std::swap(m_size, other.m_size);
        }
    }
};

}

template<typename T, usize N, bool Overflow>
template<typename... Args>
inline T& svector<T, N, Overflow>::emplace_back(Args&&... args) {
    if (ZEN_LIKELY(has_capacity(1))) {
        return *mem::construct_at(data() + m_size++, ZEN_FWD(args)...);
    } else {
        ensure_capacity(1);
        return *mem::construct_at(data() + m_size++, ZEN_FWD(args)...);
    }
}

template<typename T, usize N, bool Overflow>
inline T& svector<T, N, Overflow>::push_back(const T& value) {
    if (ZEN_LIKELY(has_capacity(1))) {
        return *mem::construct_at(data() + m_size++, value);
    } else {
        ensure_capacity(1);
        return *mem::construct_at(data() + m_size++, value);
    }
}

template<typename T, usize N, bool Overflow>
inline T& svector<T, N, Overflow>::push_back(T&& value) {
    if (ZEN_LIKELY(has_capacity(1))) {
        return *mem::construct_at(data() + m_size++, std::move(value));
    } else {
        ensure_capacity(1);
        return *mem::construct_at(data() + m_size++, std::move(value));
    }
}

template<typename T, usize N, bool Overflow>
inline void svector<T, N, Overflow>::pop_back() {
    if (ZEN_UNLIKELY(empty()))
        return;
    mem::destroy_at(data() + --m_size);
}

template<typename T, usize N, bool Overflow>
template<typename... Args>
inline T& svector<T, N, Overflow>::emplace(const_iterator position, Args&&... args) {
    const usize i = position - data();
    ensure_capacity_insert(position, 1);
    m_size++;
    return *mem::construct_at(data() + i, ZEN_FWD(args)...);
}

template<typename T, usize N, bool Overflow>
inline T* svector<T, N, Overflow>::insert(const_iterator position, T&& value) {
    const usize i = position - data();
    ensure_capacity_insert(position, 1);
    m_size++;
    return mem::construct_at(data() + i, ZEN_FWD(value));
}

template<typename T, usize N, bool Overflow>
inline T* svector<T, N, Overflow>::insert(const_iterator position, const T& value) {
    const usize i = position - data();
    ensure_capacity_insert(position, 1);
    m_size++;
    return mem::construct_at(data() + i, value);
}

template<typename T, usize N, bool Overflow>
inline T* svector<T, N, Overflow>::insert(const_iterator position, size_type n, const T& value) {
    const usize i = position - data();
    ensure_capacity_insert(position, n);
    m_size += n;
    mem::uninit_fill<T>(data() + i, data() + i + n, value);
    return data() + i;
}

template<typename T, usize N, bool Overflow>
template<typename It>
inline T* svector<T, N, Overflow>::insert(const_iterator position, It begin, It end) {
    const usize n = end - begin;
    const usize i = position - data();
    ensure_capacity_insert(position, n);
    m_size += size_type(n);
    mem::uninit_copy(begin, end, data() + i);
    return data() + i;
}

template<typename T, usize N, bool Overflow>
inline T* svector<T, N, Overflow>::erase(const_iterator position) {
    const usize i = position - data();
    mem::destroy_at(data() + i);
    mem::move(data() + i + 1, end(), data() + i);
    m_size--;
    return data() + i;
}

template<typename T, usize N, bool Overflow>
inline T* svector<T, N, Overflow>::erase(const_iterator first, const_iterator last) {
    const usize n = last - first;
    const usize i = first - data();
    mem::destroy_n(data() + i, n);
    mem::move(data() + i + n, end(), data() + i);
    m_size -= n;
    return data() + i;
}

template<typename T, usize N, bool Overflow>
inline void svector<T, N, Overflow>::clear() noexcept {
    mem::destroy_n(data(), m_size);
    m_size = 0;
}

template<typename T, usize N, bool Overflow>
inline void svector<T, N, Overflow>::reserve(size_type n) {
    ensure_capacity(n);
}

template<typename T, usize N, bool Overflow>
inline void svector<T, N, Overflow>::resize(size_type n) {
    if (n < m_size) {
        mem::destroy_n(data() + n, m_size - n);
        resize_shrink(n);
    } else {
        ensure_capacity(n - m_size);
        mem::uninit_fill(end(), end() + n - m_size);
    }
    m_size = n;
}

template<typename T, usize N, bool Overflow>
inline void svector<T, N, Overflow>::resize(size_type n, const T& value) {
    if (n < m_size) {
        mem::destroy_n(data() + n, m_size - n);
        resize_shrink(n);
    } else {
        ensure_capacity(n - m_size);
        mem::uninit_fill(end(), end() + n - m_size, value);
    }
    m_size = n;
}

template<typename T, usize N, bool Overflow>
svector<T, N, Overflow>::svector(const svector& v) {
    for(auto& a: v) push_back(a);
}

template<typename T, usize N, bool Overflow>
svector<T, N, Overflow>::svector(svector&& v) noexcept {
    for(auto& a: v) push_back(std::move(a));
    v.reset_small();
}

template<typename T, usize N, bool Overflow>
svector<T, N, Overflow>& svector<T, N, Overflow>::operator=(const svector& v) {
    if (&v == this) return *this;
    clear();
    ensure_capacity(v.size());
    for(auto& a: v) push_back(a);
    return *this;
}

template<typename T, usize N, bool Overflow>
svector<T, N, Overflow>& svector<T, N, Overflow>::operator=(svector&& v) noexcept {
    if (&v == this) return *this;
    clear();
    ensure_capacity(v.size());
    for(auto& a: v) push_back(std::move(a));
    v.reset_small();
    return *this;
}

}


#endif // ZEN_SVECTOR_H


#ifndef ZEN_TUPLE_H
#define ZEN_TUPLE_H


namespace zen::impl {

template <typename T> struct unwrap_reference { using type = T; };
template <typename U> struct unwrap_reference<std::reference_wrapper<U>> { using type = U&; };
template<typename T>
using unwrap_ref_decay_t = typename unwrap_reference<std::decay_t<T>>::type;

template<usize I, typename... T>
struct tuple_base;

}

namespace zen {

template<typename... T>
struct tuple : impl::tuple_base<0, T...> {
    static constexpr auto size = sizeof...(T);

    constexpr tuple() = default;
    constexpr tuple(const T&... values) : impl::tuple_base<0, T...>{ZEN_FWD(values)...} {}
    constexpr tuple(T&&... values) : impl::tuple_base<0, T...>{ZEN_FWD(values)...} {}
    
    template<typename Out>
    friend Out& operator<<(Out& out, const tuple& v) noexcept {
        out << '{';
        bool comma = false;
        v.each([&](const auto& x){ 
            if (comma) out << ", ";
            comma = true;
            out << x;
        });
        return out << '}';
    }
};


// Deduction guides
template<typename... T> tuple(T...) -> tuple<T...>;
template<typename... T> tuple(const T&...) -> tuple<T...>;
template<typename... T> tuple(T&&...) -> tuple<T...>;


// tuple size
template<typename Tuple>
static constexpr usize tuple_size_v = Tuple::size;


// tuple element
template<usize, typename>
struct tuple_element;

template<usize Index, typename First, typename... Other>
struct tuple_element<Index, tuple<First, Other...>> : tuple_element<Index - 1u, tuple<Other...>> {};

template<typename First, typename... Other>
struct tuple_element<0u, tuple<First, Other...>> { using type = First; };

template<usize Index, typename List>
using tuple_element_t = typename tuple_element<Index, std::remove_const_t<List>>::type;


// tuple get
template<usize I, typename... T>
constexpr decltype(auto) get(tuple<T...>& t) noexcept { return t.template get<I>(); }

template<usize I, typename... T>
constexpr decltype(auto) get(const tuple<T...>& t) noexcept { return t.template get<I>(); }

template<usize I, typename... T>
constexpr decltype(auto) get(tuple<T...>&& t) noexcept { return t.template get<I>(); }


// tuple factories
template<typename... Ts>
constexpr decltype(auto) make_tuple(Ts &&...ts) {
    return tuple<impl::unwrap_ref_decay_t<Ts>...>{ZEN_FWD(ts)...};
}

template<typename F, typename T>
constexpr decltype(auto) tuple_apply(F&& func, T&& t) noexcept { 
    return impl::tuple_apply(ZEN_FWD(func), ZEN_FWD(t), std::make_index_sequence<std::remove_reference_t<T>::size>());
}

template<typename F, typename T1, typename T2>
constexpr decltype(auto) tuple_combine(F&& func, T1&& t1, T2&& t2) noexcept { 
    return impl::tuple_combine(ZEN_FWD(func), ZEN_FWD(t1), ZEN_FWD(t2), std::make_index_sequence<std::remove_reference_t<T1>::size>());
}


namespace impl {

template<typename F, typename T, usize... I>
constexpr decltype(auto) tuple_apply(F&& func, T&& t, std::index_sequence<I...>) noexcept { return make_tuple(func(get<I>(t))...); }

template<typename F, typename T1, typename T2, usize... I>
constexpr decltype(auto) tuple_combine(F&& func, T1&& t1, T2&& t2, std::index_sequence<I...>) noexcept { return make_tuple(func(get<I>(t1), get<I>(t2))...); }
}

}

namespace zen::impl {

template<usize I>
struct tuple_base<I> {};


template<usize I, typename T>
struct holder {
    constexpr holder() = default;
    constexpr holder(const T& value) noexcept : value_{value} {}
    constexpr holder(T&& value) noexcept : value_{ZEN_FWD(value)} {}
    constexpr T& value() noexcept { return value_; }
    constexpr T value() const noexcept { return value_; }
private:
    T value_;
};


template<usize I, typename T, typename... Rest>
struct tuple_base<I, T, Rest...> : holder<I, T>, tuple_base<I + 1, Rest...>  {
    constexpr tuple_base() = default;
    constexpr tuple_base(const T& value, const Rest&... rest) : holder<I, T>{ZEN_FWD(value)}, tuple_base<I + 1, Rest...>{ZEN_FWD(rest)...} {}
    constexpr tuple_base(T&& value, Rest&&... rest) : holder<I, T>{ZEN_FWD(value)}, tuple_base<I + 1, Rest...>{ZEN_FWD(rest)...} {}

    template<usize J>
    constexpr auto& get() noexcept {
        if constexpr(I == J) return extract();
        else                 return tuple_base<I + 1, Rest...>::template get<J>();
    }

    template<usize J>
    constexpr auto get() const noexcept {
        if constexpr(I == J) return extract();
        else                 return tuple_base<I + 1, Rest...>::template get<J>();
    }

    template<typename F>
    constexpr void each(F&& func) noexcept {
        func(extract());
        if constexpr(sizeof...(Rest) > 0) {
            tuple_base<I + 1, Rest...>::each(std::forward<F>(func));
        }
    }

    template<typename F>
    constexpr void each(F&& func) const noexcept {
        func(extract());
        if constexpr(sizeof...(Rest) > 0) {
            tuple_base<I + 1, Rest...>::each(std::forward<F>(func));
        }
    }

private:
    auto& extract() noexcept { return static_cast<holder<I, T>&>(*this).value(); }
    auto  extract() const noexcept { return static_cast<const holder<I, T>&>(*this).value(); }
};

}

// std::tuple specialization
namespace std {
template<typename T>
struct tuple_size;
template<size_t I, typename T>
struct tuple_element;
}

template<typename... T>
struct std::tuple_size<zen::tuple<T...>>  : std::integral_constant<std::size_t, sizeof...(T)> {};

template<std::size_t I, typename... T>
struct std::tuple_element<I,zen::tuple<T...>> { using type = zen::tuple_element_t<I, zen::tuple<T...>>; };

template<typename... T>
struct std::tuple_size<const zen::tuple<T...>>  : std::integral_constant<std::size_t, sizeof...(T)> {};

template<std::size_t I, typename... T>
struct std::tuple_element<I,const zen::tuple<T...>> { using type = zen::tuple_element_t<I, zen::tuple<T...>>; };

#endif // ZEN_TUPLE_H


#ifndef ZEN_UNICODE_H
#define ZEN_UNICODE_H


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

