#pragma once

#include <functional>

#include "detail/traits.hpp"
#include "detail/branchless_lower_bound.hpp"
#include "ordered_map_iterator.hpp"

namespace heurohash {

/* FWD declare ordered_map & ordered_map_valueset for span friend */
template <typename KeyT, typename ValueT, size_t Size, typename Compare>
class ordered_map;

template <typename KeyT, typename ValueT, size_t Size, typename Compare>
class ordered_map_valueset;

/* Span of linear map (aka desized, to allow better 'anonymous' interfaces) */
template <typename KeyT, typename ValueT, typename Compare = std::less<KeyT>>
class ordered_map_span {
    const KeyT *key_storage;
    ValueT *value_storage;
    size_t stor_size;
    [[no_unique_address]] Compare compare;

  public:
    /* Member types */
    using key_type = KeyT;
    using mapped_type = ValueT;
    using value_type = mapped_type;
    using size_type = size_t;
    using difference_type = std::ptrdiff_t;
    using reference = value_type &;
    using const_reference = const value_type &;
    using pointer = value_type *;
    using const_pointer = const value_type *;
    using iterator = ordered_map_iterator<KeyT, ValueT>;
    using const_iterator = ordered_map_iterator<KeyT, const ValueT>;

  protected:
    template <typename Key, typename Value, size_t Size, typename Comp>
    friend class ordered_map;

    template <typename Key, typename Value, size_t Size, typename Comp>
    friend class ordered_map_valueset;

    explicit constexpr ordered_map_span(
        const KeyT *keys, ValueT *values, size_t size,
        const Compare &comp = Compare{}) noexcept
        : key_storage{keys}, value_storage{values}, stor_size(size),
          compare(comp) {}

  public:
    constexpr ordered_map_span(const ordered_map_span &) noexcept = default;
    constexpr ordered_map_span(ordered_map_span &&) noexcept = default;
    constexpr ordered_map_span &
    operator=(const ordered_map_span &) noexcept = default;
    constexpr ordered_map_span &
    operator=(ordered_map_span &&) noexcept = default;

    constexpr operator ordered_map_span<KeyT, const ValueT, Compare>()
        const noexcept {
        return ordered_map_span<KeyT, const ValueT, Compare>(
            key_storage, value_storage, stor_size, compare);
    }

    /* Lookup */
    constexpr ValueT *find(const KeyT &key) const noexcept {
        return value_storage + find_impl(key);
    }

    constexpr reference operator[](const KeyT &key) const noexcept {
        return value_storage[find_impl(key)];
    }

    constexpr reference at(const KeyT &key) const noexcept {
        auto idx = find_impl(key);
        constexpr_assert(idx != stor_size, "Key not found");
        return value_storage[find_impl(key)];
    }

    constexpr size_type count(const KeyT &key) const noexcept {
        return contains() ? 1 : 0;
    }

    constexpr bool contains(const KeyT &key) const noexcept {
        return find_impl(key) != end();
    }

    constexpr bool empty() const noexcept { return stor_size == 0; }

    constexpr size_t size() const noexcept { return stor_size; }

    constexpr size_t max_size() const noexcept { return stor_size; }

    constexpr iterator begin() const noexcept { return iterator{key_storage, value_storage}; }

    constexpr iterator end() const noexcept {
        return iterator{key_storage + stor_size, value_storage + stor_size};
    }

    constexpr void clear() noexcept {
        std::fill(value_storage, value_storage + stor_size, ValueT{});
    }

  private:
    constexpr size_t find_impl(const KeyT &key) const noexcept {
        return detail::ordered_find_impl_cast(key_storage, stor_size, key,
                                              compare);
    }
};
}; // namespace heurohash