#ifndef ZEN_ARRAY_H
#define ZEN_ARRAY_H

#include "zen_config.h"

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
