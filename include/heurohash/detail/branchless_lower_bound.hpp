#pragma once

#include <cstddef>
#include <iterator>
namespace heurohash::detail {
template <class ForwardIt, class T, class Compare>
static constexpr ForwardIt branchless_lower_bound(ForwardIt first, ForwardIt last, const T& value, Compare comp) {
    auto length = last - first;
    while (length > 0) {
        auto half = length / 2;
        if (comp(first[half], value)) {
            first += length - half;
        }
        length = half;
    }
    return first;
}

template <typename KeyT, typename Compare>
static constexpr size_t ordered_find_impl(const KeyT *keys, size_t size,
                                          const KeyT &key,
                                          const Compare &compare) noexcept {
    /* branchless ~3x faster on 5900x */
    auto it = branchless_lower_bound(keys, keys + size, key, compare);
    // auto it = std::lower_bound(keys, keys + size, key, compare);
    if ((it != (keys + size)) && (static_cast<KeyT>(*it) == key)) {
        return std::distance(keys, it);
    }
    return size;
}
};