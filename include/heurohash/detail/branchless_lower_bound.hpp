#pragma once

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
};