
#ifndef ZEN_TUPLE_H
#define ZEN_TUPLE_H

#include "zen_config.h"

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
