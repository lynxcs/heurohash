#pragma once

#include <cstddef>
#include <functional>
#include <utility>

#include "detail/branchless_lower_bound.hpp"
#include "detail/traits.hpp"

namespace heurohash {
template <typename KeyT, size_t Size,
          typename Compare = std::less<KeyT>>
class ordered_map_keyset {
    using KeyUnderlyingT = detail::underlying_type<KeyT>;
    using KeyValT = std::remove_cv_t<KeyT>;
    using KeyStorageT = std::array<KeyValT, Size>;

    KeyStorageT keys{};
    [[no_unique_address]] Compare compare;

  public:
    static constexpr size_t keyset_size = Size;

    /* Member types */
    using key_type = KeyT;
    using value_type = size_t;
    using size_type = size_t;
    using difference_type = std::ptrdiff_t;
    using reference = key_type &;
    using const_reference = const value_type &;
    using pointer = value_type *;
    using const_pointer = const value_type *;
    using const_iterator = typename KeyStorageT::const_iterator;

    using compare_type = Compare;
    using storage_type = KeyValT;

    template <typename InputIt>
    consteval ordered_map_keyset(InputIt first, InputIt last,
                                 const Compare &comp = Compare{}) noexcept
        : compare(comp) {
        constexpr_assert(std::distance(first, last) == Size,
                         "Passed array size doesn't match");
        std::copy(first, last, keys.begin());
        sort_keys();
    }

    consteval ordered_map_keyset(std::initializer_list<key_type> lst,
                                 const Compare &comp = Compare{}) noexcept
        : ordered_map_keyset(lst.begin(), lst.end(), comp) {}

    consteval ordered_map_keyset(const std::array<key_type, Size> &arr,
                                 const Compare &comp = Compare{}) noexcept
        : ordered_map_keyset(arr.begin(), arr.end(), comp) {}

    consteval ordered_map_keyset(const key_type (&arr)[Size],
                                 const Compare &comp = Compare{}) noexcept
        : ordered_map_keyset(std::begin(arr), std::end(arr), comp) {}

    constexpr ordered_map_keyset(const ordered_map_keyset &) noexcept = default;
    constexpr ordered_map_keyset &
    operator=(const ordered_map_keyset &) noexcept = default;

    constexpr ordered_map_keyset(ordered_map_keyset &&) noexcept = default;
    constexpr ordered_map_keyset &
    operator=(ordered_map_keyset &&) noexcept = default;

    constexpr value_type find(const key_type &key) noexcept {
        return find_impl(key);
    }

    constexpr value_type find(const key_type &key) const noexcept {
        return find_impl(key);
    }

    constexpr size_type count(const key_type &key) const noexcept {
        return contains() ? 1 : 0;
    }

    constexpr bool contains(const key_type &key) const noexcept {
        return find_impl(key) != Size;
    }

    /* Capacity */
    constexpr bool empty() const noexcept { return Size == 0; }

    constexpr size_t size() const noexcept { return Size; }

    constexpr size_t max_size() const noexcept { return Size; }

    constexpr compare_type key_comp() const noexcept { return compare; }

    constexpr const_iterator begin() const noexcept { return keys.cbegin(); }

    constexpr const_iterator end() const noexcept { return keys.cend(); }

  private:
    constexpr void sort_keys() noexcept {
        std::sort(keys.begin(), keys.end(), compare);
        auto adjacent_val = std::adjacent_find(keys.cbegin(), keys.cend());
        constexpr_assert(adjacent_val == keys.cend(),
                         "Duplicate entries in keys");
    }

    constexpr size_t find_impl(const KeyT &key) const noexcept {
        return detail::ordered_find_impl_cast(keys.data(), keys.size(), key,
                                              compare);
    }
};

template <typename T, std::size_t N>
static consteval auto make_ordered_keyset(T const (&items)[N]) {
    return ordered_map_keyset<T, N>{items};
}

template <typename T, std::size_t N>
static consteval auto make_ordered_keyset(std::array<T, N> const &items) {
    return ordered_map_keyset<T, N>{items};
}

template <typename T, typename Compare, std::size_t N>
static consteval auto make_ordered_keyset(T const (&items)[N],
                                          Compare const &compare = Compare{}) {
    return ordered_map_keyset<T, N, Compare>{
        items, compare};
}

template <typename T, typename Compare, std::size_t N>
static consteval auto make_ordered_keyset(std::array<T, N> const &items,
                                          Compare const &compare = Compare{}) {
    return ordered_map_keyset<T, N, Compare>{
        items, compare};
}

};