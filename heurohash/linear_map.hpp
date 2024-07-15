#pragma once

#include <algorithm>
#include <span>

#include "detail/traits.hpp"

namespace heurohash {
namespace detail {
template <typename T>
static constexpr size_t linear_find_impl(const T &key, size_t size,
                                         T offset_from_zero) noexcept {
    if (key < offset_from_zero ||
        key >= static_cast<T>(size + offset_from_zero)) {
        return size;
    }

    return key - offset_from_zero;
}
}; // namespace detail

/* FWD declare linear_map for span friend */
template <typename KeyT, typename ValueT, size_t Size> class linear_map;

/* Span of linear map (aka desized, to allow better 'anonymous' interfaces) */
template <typename KeyT, typename ValueT> class linear_map_span {
    using KeyValT = detail::underlying_type<KeyT>;
    using StorageT = std::span<ValueT>;

    StorageT data;
    KeyValT offset_from_zero;

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
    using iterator = typename StorageT::iterator;

  protected:
    template <typename Key, typename Value, size_t Size>
    friend class linear_map;

    explicit constexpr linear_map_span(std::span<ValueT> data,
                                       KeyValT offset) noexcept
        : data(data), offset_from_zero(offset) {}

  public:
    constexpr linear_map_span(const linear_map_span &) noexcept = default;
    constexpr linear_map_span(linear_map_span &&) noexcept = default;
    constexpr linear_map_span &
    operator=(const linear_map_span &) noexcept = default;
    constexpr linear_map_span &operator=(linear_map_span &&) noexcept = default;

    /* Lookup */
    constexpr iterator find(const KeyT &key) const noexcept {
        return data.begin() + find_impl(key);
    }

    constexpr reference operator[](const KeyT &key) const noexcept {
        return data[find_impl(key)];
    }

    constexpr reference at(const KeyT &key) const noexcept {
        auto idx = find_impl(key);
        constexpr_assert(idx != data.size(), "Key not found");
        return data[find_impl(key)];
    }

    constexpr size_type count(const KeyT &key) const noexcept {
        return contains() ? 1 : 0;
    }

    constexpr bool contains(const KeyT &key) const noexcept {
        return find_impl(key) != end();
    }

    constexpr bool empty() const noexcept { return data.empty(); }

    constexpr size_t size() const noexcept { return data.size(); }

    constexpr size_t max_size() const noexcept { return data.size(); }

    constexpr iterator begin() const noexcept { return data.begin(); }

    constexpr iterator end() const noexcept { return data.end(); }

    constexpr void clear() noexcept {
        std::fill(data.begin(), data.end(), ValueT{});
    }

  private:
    constexpr size_t find_impl(const KeyT &key) const noexcept {
        return detail::linear_find_impl<KeyValT>(static_cast<KeyValT>(key),
                                                 data.size(), offset_from_zero);
    }
};

/* Most space-efficient map implementation. Keys are discarded, since they are
 * essentially just indices */
template <typename KeyT, typename ValueT, size_t Size> class linear_map {
    using KeyValT = detail::underlying_type<KeyT>;
    using StorageT = std::array<ValueT, Size>;
    static_assert(std::is_integral_v<KeyValT>,
                  "Type must be integral (suitable for indexing array)");
    StorageT data{};
    /* Offset type. Used to support ranges which don't start from 0 (e.g.: [-1,
     * 0, 1] or [5, 6, 7] etc.) */
    KeyValT offset_from_zero;

  public:
    /* Member types */
    using key_type = KeyT;
    using mapped_type = ValueT;
    using value_type = mapped_type;
    using pair_type = std::pair<key_type, value_type>;
    using size_type = size_t;
    using difference_type = std::ptrdiff_t;
    using reference = value_type &;
    using const_reference = const value_type &;
    using pointer = value_type *;
    using const_pointer = const value_type *;
    using iterator = typename StorageT::iterator;
    using const_iterator = typename StorageT::const_iterator;

    template <typename InputIt>
    consteval linear_map(InputIt first, InputIt last) noexcept
        : offset_from_zero(check_prereqs(std::span(first, last))) {
        /* Copy over data if iterator is KVP */
        if constexpr (requires { (*first).second; }) {
            std::for_each(first, last, [this](const auto &kvp) {
                data[static_cast<KeyValT>(kvp.first) - offset_from_zero] =
                    kvp.second;
            });
        }
    }

    consteval linear_map(std::initializer_list<pair_type> kvp_items) noexcept
        : linear_map(kvp_items.begin(), kvp_items.end()) {}
    consteval linear_map(std::initializer_list<key_type> key_items) noexcept
        : linear_map(key_items.begin(), key_items.end()) {}

    consteval linear_map(const std::array<pair_type, Size> &kvp_items) noexcept
        : linear_map(kvp_items.begin(), kvp_items.end()) {}
    consteval linear_map(const std::array<key_type, Size> &key_items) noexcept
        : linear_map(key_items.begin(), key_items.end()) {}

    consteval linear_map(const pair_type (&kvp_items)[Size]) noexcept
        : linear_map(kvp_items.begin(), kvp_items.end()) {}
    consteval linear_map(const key_type (&key_items)[Size]) noexcept
        : linear_map(key_items.begin(), key_items.end()) {}

    /* Copying can take place at run-time*/
    constexpr linear_map(const linear_map &) noexcept = default;
    constexpr linear_map &operator=(const linear_map &) noexcept = default;

    /* Moving can take place at run-time*/
    constexpr linear_map(linear_map &&) noexcept = default;
    constexpr linear_map &operator=(linear_map &&) noexcept = default;

    /* Lookup */
    constexpr iterator find(const KeyT &key) noexcept {
        return data.begin() + find_impl(key);
    }

    constexpr const_iterator find(const KeyT &key) const noexcept {
        return data.begin() + find_impl(key);
    }

    constexpr ValueT &operator[](const KeyT &key) noexcept {
        return data[find_impl(key)];
    }

    constexpr ValueT const &operator[](const KeyT &key) const noexcept {
        return data[find_impl(key)];
    }

    constexpr ValueT const &at(const KeyT &key) const noexcept {
        auto idx = find_impl(key);
        constexpr_assert(idx != data.size(), "Key not found");
        return data[find_impl(key)];
    }

    constexpr ValueT &at(const KeyT &key) noexcept {
        auto idx = find_impl(key);
        constexpr_assert(idx != data.size(), "Key not found");
        return data[find_impl(key)];
    }

    constexpr size_type count(const KeyT &key) const noexcept {
        return contains() ? 1 : 0;
    }

    constexpr bool contains(const KeyT &key) const noexcept {
        return find_impl(key) != end();
    }

    /* Capacity */
    constexpr bool empty() const noexcept { return Size == 0; }

    constexpr size_t size() const noexcept { return Size; }

    constexpr size_t max_size() const noexcept { return Size; }

    /* Iterators */
    constexpr iterator begin() noexcept { return data.begin(); }

    constexpr const_iterator begin() const noexcept { return data.cbegin(); }

    constexpr iterator end() noexcept { return data.end(); }

    constexpr const_iterator end() const noexcept { return data.cend(); }

    constexpr const_iterator cbegin() const noexcept { return data.cbegin(); }

    constexpr const_iterator cend() const noexcept { return data.cend(); }

    constexpr void clear() noexcept { data.fill(ValueT{}); }

    constexpr operator linear_map_span<KeyT, ValueT>() noexcept {
        return linear_map_span<KeyT, ValueT>(data, offset_from_zero);
    }

    constexpr operator linear_map_span<KeyT, const ValueT>() const noexcept {
        return linear_map_span<KeyT, const ValueT>(data, offset_from_zero);
    }

  private:
    static constexpr auto check_prereqs(const auto &container) {
        constexpr_assert(container.size() == Size, "Invalid size");
        std::array<KeyValT, Size> prereq_arr{};
        constexpr auto extract_value = [](auto val) constexpr -> KeyValT {
            if constexpr (requires { val.first; }) {
                static_assert(std::is_same_v<decltype(val.first), KeyT>);
                return static_cast<KeyValT>(val.first);
            } else {
                static_assert(std::is_same_v<decltype(val), KeyT>);
                return static_cast<KeyValT>(val);
            }
        };

        std::transform(container.begin(), container.end(), prereq_arr.begin(),
                       extract_value);
        std::sort(prereq_arr.begin(), prereq_arr.end());

        auto adjacent_val =
            std::adjacent_find(prereq_arr.cbegin(), prereq_arr.cend());
        constexpr_assert(adjacent_val == prereq_arr.cend(),
                         "Duplicate entries in keys");
        return prereq_arr.front();
    }

    constexpr size_t find_impl(const KeyT &key) const noexcept {
        return detail::linear_find_impl<KeyValT>(static_cast<KeyValT>(key),
                                                 Size, offset_from_zero);
    }
};
}; // namespace heurohash
