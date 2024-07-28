#pragma once

#include <algorithm>
#include <functional>
#include <utility>

#include "detail/traits.hpp"

#include "ordered_map_keyset.hpp"
#include "ordered_map_iterator.hpp"
#include "ordered_map_span.hpp"

namespace heurohash {

template <typename KeyT, typename ValueT, size_t Size,
          typename Compare = std::less<KeyT>>
class ordered_map {
    using StorageT = std::array<ValueT, Size>;
    ordered_map_keyset<KeyT, Size, Compare> keyset;
    StorageT values{};

  public:
    using key_type = typename decltype(keyset)::key_type;
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

    template <typename InputIt>
    consteval ordered_map(InputIt first, InputIt last,
                          const Compare &comp = Compare{}) noexcept
        : keyset(extract_keys(first, last), comp) {
        /* Copy over data if iterator is KVP */
        if constexpr (requires { (*first).second; }) {
            std::for_each(first, last, [this](const auto &kvp) {
                auto idx = keyset.find(kvp.first);
                values[idx] = kvp.second;
            });
        }
    }

    consteval ordered_map(std::initializer_list<pair_type> kvp_items,
                          const Compare &comp = Compare{}) noexcept
        : ordered_map(kvp_items.begin(), kvp_items.end(), comp) {}
    consteval ordered_map(std::initializer_list<key_type> key_items,
                          const Compare &comp = Compare{}) noexcept
        : ordered_map(key_items.begin(), key_items.end(), comp) {}

    consteval ordered_map(const std::array<pair_type, Size> &kvp_items,
                          const Compare &comp = Compare{}) noexcept
        : ordered_map(kvp_items.begin(), kvp_items.end(), comp) {}
    consteval ordered_map(const std::array<key_type, Size> &key_items,
                          const Compare &comp = Compare{}) noexcept
        : ordered_map(key_items.begin(), key_items.end(), comp) {}

    consteval ordered_map(const pair_type (&kvp_items)[Size],
                          const Compare &comp = Compare{}) noexcept
        : ordered_map(std::begin(kvp_items), std::end(kvp_items), comp) {}
    consteval ordered_map(const key_type (&key_items)[Size],
                          const Compare &comp = Compare{}) noexcept
        : ordered_map(std::begin(key_items), std::end(key_items), comp) {}

    constexpr ordered_map(const ordered_map &) noexcept = default;
    constexpr ordered_map &operator=(const ordered_map &) noexcept = default;

    constexpr ordered_map(ordered_map &&) noexcept = default;
    constexpr ordered_map &operator=(ordered_map &&) noexcept = default;

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
        return const_iterator{keyset.end(), values.cend()};
    }

    constexpr void clear() noexcept { values.fill(ValueT{}); }

    constexpr
    operator ordered_map_span<KeyT, ValueT, Compare>() noexcept {
        return ordered_map_span<KeyT, ValueT, Compare>(
            keyset.begin(), values.data(), Size, keyset.key_comp());
    }

    constexpr operator ordered_map_span<KeyT, const ValueT, Compare>()
        const noexcept {
        return ordered_map_span<KeyT, const ValueT, Compare>(
            keyset.begin(), values.data(), Size, keyset.key_comp());
    }
};

template <typename T, typename U, std::size_t N>
static consteval auto make_ordered_map(std::pair<T, U> const (&items)[N]) {
    return ordered_map<T, U, N>{items};
}

template <typename T, typename U, std::size_t N>
static consteval auto
make_ordered_map(std::array<std::pair<T, U>, N> const &items) {
    return ordered_map<T, U, N>{items};
}

template <typename T, typename U, typename Compare, std::size_t N>
static consteval auto make_ordered_map(std::pair<T, U> const (&items)[N],
                                       Compare const &compare = Compare{}) {
    return ordered_map<T, U, N, Compare>{items, compare};
}

template <typename T, typename U, typename Compare, std::size_t N>
static consteval auto
make_ordered_map(std::array<std::pair<T, U>, N> const &items,
                 Compare const &compare = Compare{}) {
    return ordered_map<T, U, N, Compare>{items, compare};
}

}; // namespace heurohash
