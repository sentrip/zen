#ifndef ZEN_BITSET_H
#define ZEN_BITSET_H

#include "zen_num.h"

namespace zen {

template<typename W = u64, typename I = u32>
struct bit_view;


template<typename W = u64, typename I = u32>
using cbit_view = bit_view<const W, I>;


#define nth_bit(T, i) T(1) << ((i) & ((sizeof(T) * 8) - 1))


// Bit view with storage
template<usize N, typename W = u64>
struct bitset {
    using word_type = W;
    static constexpr W     word_max   = num::limits<W>::max();
    static constexpr usize word_nbits = 8 * sizeof(W);
    static constexpr usize n_words    = 1 + ((N - 1) / word_nbits);
    
    ZEN_FORCEINLINE constexpr bitset() = default;

    // Bit manipulation
           ZEN_FORCEINLINE constexpr void reset     ()                       noexcept { for (auto& w: words) w = 0; }
           ZEN_FORCEINLINE constexpr void set       (usize i)                noexcept { words[i / word_nbits] |= nth_bit(W, i); }
           ZEN_FORCEINLINE constexpr void clear     (usize i)                noexcept { words[i / word_nbits] &= ~nth_bit(W, i); }
    ZEN_ND ZEN_FORCEINLINE constexpr bool test      (usize i)          const noexcept { return (words[i / word_nbits] & nth_bit(W, i)) != 0; }
    ZEN_ND ZEN_FORCEINLINE constexpr bool operator[](usize i)          const noexcept { return (words[i / word_nbits] & nth_bit(W, i)) != 0; }
    ZEN_ND ZEN_FORCEINLINE constexpr bool any       ()                 const noexcept { 
        for(const auto w: words) { 
            if (w != 0) 
                return true; 
        }
        return false; 
    }
    ZEN_ND ZEN_FORCEINLINE constexpr bool all       ()                 const noexcept { 
        for(const auto w: words) { 
            if (w != word_max) 
                return false; 
        }
        return true; 
    }
    
    // Word manipulation
    ZEN_ND ZEN_FORCEINLINE constexpr       W* data()                       noexcept { return words; }
    ZEN_ND ZEN_FORCEINLINE constexpr const W* data()                 const noexcept { return words; }
    ZEN_ND ZEN_FORCEINLINE constexpr       W* begin()                      noexcept { return words; }
    ZEN_ND ZEN_FORCEINLINE constexpr const W* begin()                const noexcept { return words; }
    ZEN_ND ZEN_FORCEINLINE constexpr       W* end()                        noexcept { return words + n_words; }
    ZEN_ND ZEN_FORCEINLINE constexpr const W* end()                  const noexcept { return words + n_words; }

    template<typename Out>
    friend Out& operator<<(Out& o, const bitset& v) noexcept { for (usize i = 0; i < N; ++i) o << u32(v[i]); return o; }

private:
    W words[n_words]{};
};


// Bit view with no storage
template<typename W, typename I>
struct bit_view {
    // NOTE: WORK IN PROGRESS
    using word_type = W;
    static constexpr W     word_max   = num::limits<W>::max();
    static constexpr I     word_nbits = 8 * sizeof(W);
    
    ZEN_FORCEINLINE constexpr bit_view() = default;

    ZEN_FORCEINLINE constexpr bit_view(W* words, I count) : 
        words{words}, begin{0}, end{count} {}

    ZEN_FORCEINLINE constexpr bit_view(W* begin, I offset, I count) : 
        words{begin + offset / word_nbits}, begin{offset & (word_nbits - 1)}, end{begin + count} {}

    ZEN_FORCEINLINE constexpr bit_view(W* begin, I begin_offset, W* end, I end_offset) : 
        words{begin + begin_offset / word_nbits}, begin{begin_offset & (word_nbits - 1)}, end{(end - begin) * word_nbits + end_offset} {}

    // Bit manipulation
    ZEN_ND ZEN_FORCEINLINE constexpr W    prefix()                     const noexcept { return words[0] & bit_shift_right_safe(word_max, word_nbits - begin); }
    ZEN_ND ZEN_FORCEINLINE constexpr W    suffix()                     const noexcept { return words[end / word_nbits] & (word_max << (end & (word_nbits - 1))); }
    ZEN_ND ZEN_FORCEINLINE constexpr bool test      (usize i)          const noexcept { I x = i + begin; return (words[x / word_nbits] & nth_bit(W, x)) != 0; }
    ZEN_ND ZEN_FORCEINLINE constexpr bool operator[](usize i)          const noexcept { I x = i + begin; return (words[x / word_nbits] & nth_bit(W, x)) != 0; }
    ZEN_ND ZEN_FORCEINLINE constexpr bool any       ()                 const noexcept { 
        if (prefix() != 0)
            return true;
        for(const auto w: *this) { 
            if (w != 0) 
                return true; 
        }
        return suffix() != 0; 
    }
    ZEN_ND ZEN_FORCEINLINE constexpr bool all       ()                 const noexcept { 
        if (prefix() != word_max)
            return true;
        for(const auto w: *this) { 
            if (w != word_max) 
                return false; 
        }
        return suffix() == word_max;
    }

    // Word manipulation
    ZEN_ND ZEN_FORCEINLINE constexpr       W* begin()                      noexcept { return words; }
    ZEN_ND ZEN_FORCEINLINE constexpr const W* begin()                const noexcept { return words; }
    ZEN_ND ZEN_FORCEINLINE constexpr       W* end()                        noexcept { return words; }
    ZEN_ND ZEN_FORCEINLINE constexpr const W* end()                  const noexcept { return words; }

private:
    W* words{};
    I  begin{}, end{};
};

#undef nth_bit

// Safely shift bits for widths >= width of type
template<typename T>
ZEN_FORCEINLINE constexpr T bit_shift_left_safe(T word, usize shift) noexcept {
    const auto sh = shift / 2;
    return (word << sh) << (shift - sh); 
}

// Safely shift bits for widths >= width of type
template<typename T>
ZEN_FORCEINLINE constexpr T bit_shift_right_safe(T word, usize shift) noexcept {
    const auto sh = shift / 2;
    return (word >> sh) >> (shift - sh); 
}

// Set range of bits with no offset
template<bool On = true, typename T>
constexpr void bit_range_set(T* data, usize bit_end) noexcept {
    constexpr usize NBits  = sizeof(T) * 8;
    constexpr T MAX        = num::limits<T>::max();
    const auto aligned_end = align_down(bit_end, NBits);
    const auto word_end    = aligned_end / NBits;
    const auto mask        = bit_shift_right_safe(MAX, (NBits - (bit_end & (NBits - 1))));
    if constexpr(On) {
        memset(data, 0xff, word_end * sizeof(T));
        data[word_end] |= mask;
    } else {
        memset(data, 0, word_end * sizeof(T));
        data[word_end] &= ~mask;
    }
}

// Set range of bits
template<bool On = true, usize MaxSize = SIZE_MAX, typename T>
constexpr void bit_range_set(T* data, usize bit_begin, usize bit_end) noexcept {
    constexpr usize NBits  = sizeof(T) * 8;
    constexpr T MAX        = num::limits<T>::max();
    const auto n           = bit_end - bit_begin;
    const auto word_begin  = bit_begin / NBits;
    const auto word_end    = bit_end / NBits;
    const auto shift_begin = bit_begin & (NBits - 1);
    const auto shift_end   = NBits - (bit_end & (NBits - 1));
    const auto mask_begin  = MAX << shift_begin;
    const auto mask_end    = bit_shift_right_safe(MAX, shift_end);
    // If bit_begin is aligned to a word boundary, we can use the fast version
    if (ZEN_UNLIKELY(shift_begin == 0)) {
        const auto offset  = bit_begin / NBits;
        return bit_range_set<On>(data + offset, bit_end - (offset * NBits));
    }

    // For ranges that are fully contained in a single word, we only need to write data in a single word and return
    if (ZEN_LIKELY(word_begin == word_end)) {
        if constexpr(On) {
            data[word_begin] |= (mask_begin & mask_end);
        } else {
            data[word_begin] &= ~(mask_begin & mask_end);
        }
        return;
    }

    // Otherwise we need to write two or more words
    if constexpr(On) {
        data[word_begin] |= mask_begin;
        data[word_end]   |= mask_end;
    } else {
        data[word_begin] &= ~mask_begin;
        data[word_end]   &= ~mask_end;
    }
    
    // If the data overflows a single word, then we need to loop and fill the middle
    if constexpr(MaxSize <= NBits) { return; }
    if (ZEN_LIKELY(n <= NBits)) { return; }

    const auto aligned_begin = align_up(bit_begin, NBits);
    const auto aligned_end   = align_down(bit_end, NBits);
    const auto middle        = (aligned_end - aligned_begin) / NBits;
    auto* begin              = data + (aligned_begin / NBits);
    memset(begin, On ? 0xff : 0, middle);
}

// Copy range of bits from src to dst
template<bool ClearBeforeWrite = true, usize MaxSize = SIZE_MAX, typename T>
constexpr void bit_range_copy(T* dst, usize dst_begin, const T* src, usize src_begin, usize src_end) noexcept {
    constexpr usize NBits      = sizeof(T) * 8;
    constexpr T MAX            = num::limits<T>::max();
    const auto bit_dst         = dst_begin & (NBits - 1);
    const auto bit_src         = src_begin & (NBits - 1);
    const auto n               = src_end - src_begin;
    const auto dst_end         = dst_begin + n;
    const auto dst_word_begin  = dst_begin / NBits;
    const auto dst_word_end    = dst_end / NBits;
    const auto src_word_begin  = src_begin / NBits;
    const auto src_word_end    = src_end / NBits;
    const auto src_shift_begin = src_begin & (NBits - 1);
    const auto src_shift_end   = NBits - (src_end & (NBits - 1));
    const auto src_mask_begin  = MAX << src_shift_begin;
    const auto src_mask_end    = bit_shift_right_safe(MAX, src_shift_end);
    const auto src_w_begin     = src[src_word_begin] & src_mask_begin;
    const auto src_w_end       = src[src_word_end] & src_mask_end;
        
    // If the local bit index is the same for src and dst, we only need to mask 
    // the first and last word, all other words in the middle can be byte-copied
    if (bit_dst == bit_src) {
        if constexpr(ClearBeforeWrite) {
            dst[dst_word_begin] &= ~src_mask_begin;
            dst[dst_word_end] &= ~src_mask_end;
        }
        dst[dst_word_begin]     |= src_w_begin;
        dst[dst_word_end]       |= src_w_end;
    }
    // Every single word will overflow, so we need a slower, more generic loop
    else {
        const auto dst_shift_begin = dst_begin & (NBits - 1);
        const auto dst_shift_end   = NBits - (dst_end & (NBits - 1));
        const auto dst_mask_begin  = MAX << dst_shift_begin;
        const auto dst_mask_end    = bit_shift_right_safe(MAX, dst_shift_end);
        if constexpr(ClearBeforeWrite) {
            dst[dst_word_begin] &= ~dst_mask_begin;
            dst[dst_word_end] &= ~dst_mask_end;
        }

        if (bit_dst > bit_src) {
            const auto delta         = bit_dst - bit_src;
            dst[dst_word_begin]     |= src_w_begin << delta;
            dst[dst_word_begin + 1] |= bit_shift_right_safe(src_w_begin, NBits - delta);
            dst[dst_word_end]       |= src_w_end << delta;
            dst[dst_word_end - 1]   |= bit_shift_right_safe(src_w_end, NBits - delta);
        } else {
            const auto delta       = bit_src - bit_dst;
            dst[dst_word_begin]     |= src_w_begin >> delta;
            dst[dst_word_begin + 1] |= bit_shift_left_safe(src_w_begin, NBits - delta);
            dst[dst_word_end]       |= src_w_end >> delta;
            dst[dst_word_end - 1]   |= bit_shift_left_safe(src_w_end, NBits - delta);
        }
    }

    // If the data overflows a single word, then we need to loop and fill the middle        
    if constexpr(MaxSize <= NBits) { return; }
    if (ZEN_LIKELY(n <= NBits)) { return; }
       
    const auto dst_aligned_begin    = align_up(dst_begin, NBits);
    const auto src_aligned_begin    = align_up(src_begin, NBits);
    const auto src_aligned_end      = align_down(src_end, NBits);
    auto* dst_data_begin            = dst + (dst_aligned_begin / NBits);
    const auto* src_data_begin      = src + (src_aligned_begin / NBits);

    // The middle can just be byte copied
    if (bit_dst == bit_src) {
        memcpy(dst_data_begin, src_data_begin, (src_aligned_end - src_aligned_begin) / 8);
    }
    // Otherwise we must handle overflow for every single word
    else {
        const auto middle               = (src_aligned_end - src_aligned_begin) / NBits;
        const auto* src_data_end        = src_data_begin + middle;        
        if (bit_dst > bit_src) {
            const auto delta            = bit_dst - bit_src;
            for (; src_data_begin != src_data_end; ++src_data_begin, (void) ++dst_data_begin) {
                if constexpr(ClearBeforeWrite) { *dst_data_begin = 0; }
                *dst_data_begin        |= *src_data_begin << delta;
                *(dst_data_begin + 1)  |= bit_shift_right_safe(*src_data_begin, NBits - delta);
            }
        }
        else {
            const auto delta            = bit_src - bit_dst;
            for (; src_data_begin != src_data_end; ++src_data_begin, (void) ++dst_data_begin) {
                if constexpr(ClearBeforeWrite) { *dst_data_begin = 0; }
                *dst_data_begin        |= *src_data_begin >> delta;
                *(dst_data_begin - 1)  |= bit_shift_left_safe(*src_data_begin, NBits - delta);
            }
        }
    }
}

}

#endif //ZEN_BITSET_H
