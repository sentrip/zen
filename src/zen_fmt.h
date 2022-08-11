#ifndef ZEN_FMT_H
#define ZEN_FMT_H

#include "zen_string.h"
#include <cstdio>

// Debugger trigger for zen::fmt::impl::assert_fail
#ifdef ZEN_COMPILER_MSVC
    extern "C" void DebugBreak();
    #ifdef _DEBUG
        #define ZEN_DEBUG
    #endif
#else
    #include <csignal>
    #ifndef NDEBUG
        #define ZEN_DEBUG
    #endif
#endif

// Todo macro for indicating unfinished implementations
#define todo(name)          assertf(false, "TODO: implement " name)

// Formatted assert with fmt library
#ifdef ZEN_DEBUG
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
    using size_type = typename sstring<N>::size_type;

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
    buffer& operator<<(const binary<T>& v)     noexcept { set_size(size_type(int_to_chars<2>(end(), begin() + max_size(), v.value) - begin())); return *this; }

    template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
    buffer& operator<<(const hex<T>& v)        noexcept { set_size(size_type(int_to_chars<16>(end(), begin() + max_size(), v.value) - begin())); return *this; }

    template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
    buffer& operator<<(const hexu<T>& v)       noexcept { set_size(size_type(int_to_chars<16 | impl::HEX_UPPER>(end(), begin() + max_size(), v.value) - begin())); return *this; }
    
    template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
    buffer& operator<<(const octal<T>& v)      noexcept { set_size(size_type(int_to_chars<8>(end(), begin() + max_size(), v.value) - begin())); return *this; }

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
        if constexpr(std::is_integral_v<std::remove_reference_t<T>>) {
            switch (style) {
                case 'b': out << "0b"; break;
                case 'x': out << "0x"; break;
                case 'X': out << "0x"; break;
                case 'o': out << "0o"; break;
                default: break;
            }
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
char* int_to_chars(char* begin, [[maybe_unused]] char* end, const T& value, num::index_t<Base>) noexcept
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
        // assert((end - begin) >= n && "Not enough space to format integer");
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
        // assert((end - begin) >= n && "Not enough space to format integer");
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
template<typename Type>
constexpr auto type_name() noexcept {
    // NOTE: In order for this trick to work, the only template allowed 
    // in the function signature is a user template. Therefore, to return
    // a string view (a template), we need the return type to be 'auto'
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
    return string_view{""};
    #else
    string_view pretty_function{ZEN_PRETTY_FUNCTION};
    auto first = pretty_function.find_first_not_of(' ', pretty_function.find_first_of(ZEN_PRETTY_FUNCTION_PREFIX) + 1);
    auto value = pretty_function.substr(first, pretty_function.find_last_of(ZEN_PRETTY_FUNCTION_SUFFIX) - first);
    return value;
    #endif

    #undef ZEN_PRETTY_FUNCTION
}

}

#endif // ZEN_FMT_H
