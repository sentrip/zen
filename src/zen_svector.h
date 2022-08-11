#ifndef ZEN_SVECTOR_H
#define ZEN_SVECTOR_H

#include "zen_alloc.h"
#include "zen_num.h"
#include "zen_fmt.h"

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
    
    ZEN_FORCEINLINE constexpr void ensure_capacity([[maybe_unused]] usize n) {
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
svector<T, N, Overflow>::svector(const svector& v) : svector{} {
    for(auto& a: v) push_back(a);
}

template<typename T, usize N, bool Overflow>
svector<T, N, Overflow>::svector(svector&& v) noexcept : svector{} {
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
