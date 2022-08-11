#ifndef ZEN_BSWAP_H
#define ZEN_BSWAP_H

#include "zen_config.h"
#include <cstring>

namespace zen::bytes {

// Basic bswap functions using GCC/clang/MSVC intrinsics.
#ifdef ZEN_COMPILER_MSVC
#include <cstdlib>
constexpr u8  bswap(u8 v)  noexcept { return v; }
static    u16 bswap(u16 v) noexcept { return _byteswap_ushort(v); }
static    u32 bswap(u32 v) noexcept { return _byteswap_ulong(v); }
static    u64 bswap(u64 v) noexcept { return _byteswap_uint64(v); }
#else
constexpr u8  bswap(u8 v)  noexcept { return v; }
static    u16 bswap(u16 v) noexcept { return __builtin_bswap16(v); }
static    u32 bswap(u32 v) noexcept { return __builtin_bswap32(v); }
static    u64 bswap(u64 v) noexcept { return __builtin_bswap64(v); }
#endif

// Load an integer from memory
template <class T>
static T load(const void* Ptr) noexcept {
    static_assert(std::is_integral<T>::value, "T must be an integer!");
    T Ret;
    memcpy(&Ret, Ptr, sizeof(T));
    return Ret;
}

// Store an integer to memory
template <class T>
static void store(void* Ptr, const T V) noexcept {
    static_assert(std::is_integral<T>::value, "T must be an integer!");
    memcpy(Ptr, &V, sizeof(V));
}


// Decodes little-endian input (u8) into any-endian output (T). Assumes len is a multiple of sizeof(T).
template<typename T>
constexpr void decode(T* output, const u8* input, usize len) noexcept
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
constexpr void encode(u8* output, const T* input, usize len) noexcept
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
