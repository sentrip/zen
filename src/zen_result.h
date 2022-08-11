#ifndef ZEN_RESULT_H
#define ZEN_RESULT_H

#include "zen_config.h"

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
