#pragma once

#include "detail/traits.hpp"

#include "ordered_map_iterator.hpp"
#include "ordered_map_keyset.hpp"
#include "ordered_map_span.hpp"

namespace heurohash {
template <typename KeyT, typename ValueT, size_t Size,
          typename Compare = std::less<KeyT>>
class ordered_map_valueset {
    using StorageT = std::array<ValueT, Size>;
    using KeysetT = ordered_map_keyset<KeyT, Size, Compare>;
    /* FIXME: Move ordered_map & ordered_map_valueset into a common class where
     * KeysetT can be controlled */
    const KeysetT &keyset;
    StorageT values{};

  public:
    using key_type = KeyT;
    using mapped_type = ValueT;
    using value_type = ValueT;
    using pair_type = std::pair<key_type, value_type>;
    using size_type = size_t;
    using difference_type = std::ptrdiff_t;
    using reference = value_type &;
    using const_reference = const value_type &;
    using pointer = value_type *;
    using const_pointer = const value_type *;
    using iterator = ordered_map_iterator<KeyT, ValueT>;
    using const_iterator = ordered_map_iterator<KeyT, const ValueT>;

    template <typename InputIt>
    static constexpr std::array<KeyT, Size> extract_keys(InputIt first,
                                                         InputIt last) {
        std::array<KeyT, Size> keys{};
        std::transform(first, last, keys.begin(), [](const auto &kvp) {
            if constexpr (requires { (*first).second; }) {
                return kvp.first;
            } else {
                return kvp;
            }
        });
        return keys;
    }

    consteval ordered_map_valueset(const KeysetT &keyset) noexcept
        : keyset(keyset) {}

    template <typename InputIt>
    consteval ordered_map_valueset(const KeysetT &keyset, InputIt first,
                                   InputIt last) noexcept
        : keyset(keyset) {
        std::for_each(first, last, [&](const auto &kvp) {
            auto idx = keyset.find(kvp.first);
            values[idx] = kvp.second;
        });
    }

    consteval ordered_map_valueset(
        const KeysetT &keyset,
        std::initializer_list<pair_type> kvp_items) noexcept
        : ordered_map_valueset(keyset, kvp_items.begin(), kvp_items.end()) {}
    consteval ordered_map_valueset(
        const KeysetT &keyset,
        const std::array<pair_type, Size> &kvp_items) noexcept
        : ordered_map_valueset(keyset, kvp_items.begin(), kvp_items.end()) {}
    consteval ordered_map_valueset(const KeysetT &keyset,
                                   const pair_type (&kvp_items)[Size]) noexcept
        : ordered_map_valueset(keyset, std::begin(kvp_items),
                               std::end(kvp_items)) {}

    constexpr ordered_map_valueset(const ordered_map_valueset &) noexcept =
        default;
    constexpr ordered_map_valueset &
    operator=(const ordered_map_valueset &) noexcept = default;

    constexpr ordered_map_valueset(ordered_map_valueset &&) noexcept = default;
    constexpr ordered_map_valueset &
    operator=(ordered_map_valueset &&) noexcept = default;

    constexpr ValueT *find(const key_type &key) noexcept {
        return values.begin() + keyset.find(key);
    }

    constexpr const ValueT *find(const key_type &key) const noexcept {
        return values.cbegin() + keyset.find(key);
    }

    constexpr ValueT &operator[](const KeyT &key) noexcept {
        return values[keyset.find(key)];
    }

    constexpr ValueT const &operator[](const KeyT &key) const noexcept {
        return values[keyset.find(key)];
    }

    constexpr ValueT &at(const KeyT &key) noexcept {
        auto idx = keyset.find(key);
        constexpr_assert(idx != Size, "Key not found");
        return values[idx];
    }

    constexpr ValueT const &at(const KeyT &key) const noexcept {
        auto idx = keyset.find(key);
        constexpr_assert(idx != Size, "Key not found");
        return values[idx];
    }

    constexpr size_type count(const key_type &key) const noexcept {
        return contains() ? 1 : 0;
    }

    constexpr bool contains(const key_type &key) const noexcept {
        return keyset.contains(key);
    }

    /* Capacity */
    constexpr bool empty() const noexcept { return Size == 0; }

    constexpr size_t size() const noexcept { return Size; }

    constexpr size_t max_size() const noexcept { return Size; }

    /* Iterators */
    constexpr iterator begin() noexcept {
        return iterator{keyset.begin(), values.begin()};
    }

    constexpr const_iterator begin() const noexcept {
        return const_iterator{keyset.begin(), values.cbegin()};
    }

    constexpr iterator end() noexcept {
        return iterator{keyset.end(), values.end()};
    }

    constexpr const_iterator end() const noexcept {
        return iterator{keyset.end(), values.cend()};
    }

    constexpr void clear() noexcept { values.fill(ValueT{}); }

    constexpr operator ordered_map_span<KeyT, ValueT, Compare>() noexcept {
        return ordered_map_span<KeyT, ValueT, Compare>(
            keyset.begin(), values.data(), Size, keyset.key_comp());
    }

    constexpr
    operator ordered_map_span<KeyT, const ValueT, Compare>() const noexcept {
        return ordered_map_span<KeyT, const ValueT, Compare>(
            keyset.begin(), values.data(), Size, keyset.key_comp());
    }

    constexpr auto to_span() noexcept {
        return ordered_map_span<KeyT, ValueT, Compare>(
            keyset.begin(), values.data(), Size, keyset.key_comp());
    }

    constexpr auto to_span() const noexcept {
        return ordered_map_span<KeyT, const ValueT, Compare>(
            keyset.begin(), values.data(), Size, keyset.key_comp());
    }
};

template <typename T, typename U, std::size_t N>
static consteval auto
make_ordered_map_valueset(const ordered_map_keyset<T, N> &keyset) {
    return ordered_map_valueset<T, U, N>{keyset};
}

template <typename T, typename U, std::size_t N>
static consteval auto
make_ordered_map_valueset(const ordered_map_keyset<T, N> &keyset,
                          std::array<std::pair<T, U>, N> const &items) {
    return ordered_map_valueset<T, U, N>{keyset, items};
}

template <typename T, typename U, std::size_t N>
static consteval auto
make_ordered_map_valueset(std::pair<T, U> const (&items)[N]) {
    return ordered_map_valueset<T, U, N>{items};
}

template <typename T, typename U, std::size_t N>
static consteval auto
make_ordered_map_valueset(std::array<std::pair<T, U>, N> const &items) {
    return ordered_map_valueset<T, U, N>{items};
}

template <typename T, typename U, typename Compare, std::size_t N>
static consteval auto
make_ordered_map_valueset(std::pair<T, U> const (&items)[N],
                          Compare const &compare = Compare{}) {
    return ordered_map_valueset<T, U, N, Compare>{items, compare};
}

template <typename T, typename U, typename Compare, std::size_t N>
static consteval auto
make_ordered_map_valueset(std::array<std::pair<T, U>, N> const &items,
                          Compare const &compare = Compare{}) {
    return ordered_map_valueset<T, U, N, Compare>{items, compare};
}

}; // namespace heurohash
