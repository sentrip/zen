#ifndef ZEN_HANDLE_H
#define ZEN_HANDLE_H

#include "zen_config.h"

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
