#pragma once

#include "traits.hpp"
#include <cstddef>
#include <iterator>

namespace heurohash::detail {
template <class ForwardIt, class T, typename Func>
static constexpr ForwardIt
branchless_lower_bound(ForwardIt first, ForwardIt last, const T &value,
                       const Func &comp_func) {
    auto length = last - first;
    while (length > 0) {
        auto half = length / 2;
        if (comp_func(first[half], value)) {
            first += length - half;
        }
        length = half;
    }
    return first;
}

template <typename KeyT, typename Func>
static constexpr size_t ordered_find_impl(const KeyT *keys, size_t size,
                                          const KeyT &key,
                                          const Func &comp_func) noexcept {
    /* branchless ~3x faster on 5900x */
    /* on embedded platforms - performance is the same */
    auto it = branchless_lower_bound(keys, keys + size, key, comp_func);
    if ((it != (keys + size)) && (static_cast<KeyT>(*it) == key)) {
        return std::distance(keys, it);
    }
    return size;
}

/* The reason we use a callable instead of a comparison type, is because it
 * offers more opportunities for identical code folding without specifying icf
 * in the linker. Take for example find of enum A & B. By using Compare we have
 * to pass types A & B, which would mean two different template instanciations,
 * however, passing underlying type (assuming it's the same), with a lambda that
 * calls the actual types comparison function, the compiler will very likely
 * fold the two different lambdas into the same instanciation (assuming that
 * they are otherwise identical, apart from the 'real' type) */
template <typename KeyT, typename Compare>
static constexpr size_t
ordered_find_impl_cast(const KeyT *keys, size_t size, const KeyT &key,
                       const Compare &compare) noexcept {
    using KeyUnderlyingT = detail::underlying_type<KeyT>;
    if constexpr (std::is_same_v<KeyT, KeyUnderlyingT>) {
        /* Can just pass comparison since underlying & real type is same */
        return detail::ordered_find_impl(keys, size, key, compare);
    } else if (std::is_constant_evaluated()) {
        /* constant eval won't allow reinterpret_cast */
        return detail::ordered_find_impl(keys, size, key, compare);
    } else if constexpr (std::is_empty_v<Compare>) {
        /* If empty, we can capture nothing in the lambda */
        return detail::ordered_find_impl<KeyUnderlyingT>(
            reinterpret_cast<const KeyUnderlyingT *>(keys), size,
            static_cast<KeyUnderlyingT>(key), [](const auto &a, const auto &b) {
                return Compare{}(static_cast<KeyT>(a), static_cast<KeyT>(b));
            });
    } else {
        /* Compare is not empty so we can't have 'empty' lambda */
        return detail::ordered_find_impl(
            reinterpret_cast<const KeyUnderlyingT *>(keys), size,
            static_cast<KeyUnderlyingT>(key),
            [&](const auto &a, const auto &b) {
                return compare(static_cast<KeyT>(a), static_cast<KeyT>(b));
            });
    }
}
}; // namespace heurohash::detail
