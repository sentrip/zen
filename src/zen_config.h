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
