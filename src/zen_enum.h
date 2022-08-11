#ifndef ZEN_ENUM_H
#define ZEN_ENUM_H

#include "zen_macros.h"

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
