#ifndef ZEN_ALLOC_H
#define ZEN_ALLOC_H

#include "zen_config.h"
#include <memory_resource>

namespace zen {

// The std:: names can get really long...
template<typename T = u8>
using alloc_t       = std::pmr::polymorphic_allocator<T>;

using mem_resource  = std::pmr::memory_resource;
using mem_buffer    = std::pmr::monotonic_buffer_resource;
using mem_pool      = std::pmr::unsynchronized_pool_resource;
using mem_pool_sync = std::pmr::synchronized_pool_resource;


namespace mem {

// Aligned buffer for container storage
template<typename T, usize NBytes = sizeof(T), usize Align = alignof(T)>
struct aligned_buffer {
    aligned_buffer() = default;
    operator T*      ()        noexcept  { return reinterpret_cast<T*>(&buf[0]); }
    operator const T*()  const noexcept  { return reinterpret_cast<const T*>(&buf[0]); }
private:
    alignas(Align) char buf[NBytes]{};
};


// I don't know why casting to void got so complicated in C++20...
#if __cplusplus >= 202002L
#define VOIDIFY(ptr) const_cast<void*>(static_cast<const volatile void*>(ptr))
#else
#define VOIDIFY(ptr) static_cast<void*>(ptr)
#endif


template<typename T, typename... Args>
ZEN_FORCEINLINE constexpr T* construct_at(T* p, Args&&... args) {
    return ::new (VOIDIFY(p)) T(ZEN_FWD(args)...);
}

template<typename T>
ZEN_FORCEINLINE constexpr void move(T* first, T* last, T* dst) {
    while (first != last)
        *dst++ = std::move(*first++);
}

template<typename T>
ZEN_FORCEINLINE constexpr void move_backward(T* first, T* last, T* dst) {
    while (first != last)
        *(dst++) = std::move(*(--last));
}

template<typename T>
ZEN_FORCEINLINE constexpr void uninit_fill(T* first, T* last) {
    for (; first != last; ++first)
        ::new (VOIDIFY(first)) T;
}

template<typename T>
ZEN_FORCEINLINE constexpr void uninit_fill(T* first, T* last, const T& value) {
    for (; first != last; ++first)
        ::new (VOIDIFY(first)) T(value);
}

template<typename It, typename T>
ZEN_FORCEINLINE constexpr void uninit_copy(It first, It last, T* dst) {
    for (; first != last; ++dst, (void)++first)
        ::new (VOIDIFY(dst)) T(*first);
}

template<typename T>
ZEN_FORCEINLINE constexpr void uninit_move(T* first, T* last, T* dst) {
    for (; first != last; ++dst, (void) ++first)
        ::new (VOIDIFY(dst)) T(std::move(*first));
}

template<typename T> 
ZEN_FORCEINLINE constexpr void destroy_at(T* p) { 
    if constexpr(!std::is_trivially_destructible_v<T>)
        p->~T(); 
}

template<typename T> 
ZEN_FORCEINLINE constexpr void destroy_n(T* p, usize n) { 
    if constexpr(!std::is_trivially_destructible_v<T>) {
        for (; n > 0; (void) ++p, --n)
            p->~T();
    }
}

template<typename T>
ZEN_FORCEINLINE constexpr void swap_ranges(T* left, usize left_size, T* right, usize right_size) {
    const usize n = left_size < right_size ? left_size : right_size;
    for (usize i = 0; i < n; ++i) std::swap(left[i], right[i]);
    const auto* dst = left_size < right_size ? left : right;
    const auto* src = left_size < right_size ? right : left;
    const usize mn = left_size < right_size ? right_size : left_size;
    move(src + n, src + mn, dst + n);
}

#undef VOIDIFY

}

}

#endif // ZEN_ALLOC_H
