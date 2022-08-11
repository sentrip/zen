#ifndef ZEN_NUM_H
#define ZEN_NUM_H

#include "zen_config.h"

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
