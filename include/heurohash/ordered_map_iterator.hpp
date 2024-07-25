#pragma once

#include "detail/traits.hpp"
#include <iterator>

namespace heurohash {
template <typename KeyT, typename ValueT>
class ordered_map_iterator {
    using KeyStorT = detail::underlying_type<KeyT>;
    const KeyStorT *key_ptr;
    ValueT *value_ptr;

    using RetKeyT = std::conditional_t<std::is_same_v<KeyT, KeyStorT>, const KeyT&, const KeyT>;

    constexpr RetKeyT do_cast(const KeyStorT *x) const noexcept {
        return static_cast<RetKeyT>(*x);
    }

  public:
    using iterator_category = std::random_access_iterator_tag;
    using difference_type   = std::ptrdiff_t;
    using value_type        = std::pair<RetKeyT, ValueT&>;
    using reference         = value_type;

    /* Inspiration for arrow proxy taken from:
     * https://github.com/WG21-SG14/SG14/blob/master/SG14/flat_map.h */
    struct arrow_proxy {
        reference *operator->() { return std::addressof(data_); }
        reference data_;
    };
    using pointer           = arrow_proxy;

    constexpr ordered_map_iterator(const KeyStorT *key_ptr,
                                   ValueT *value_ptr) noexcept
        : key_ptr(key_ptr), value_ptr(value_ptr) {}

    constexpr ordered_map_iterator() noexcept
        : key_ptr(nullptr), value_ptr(nullptr) {}

    constexpr ordered_map_iterator(const ordered_map_iterator& other) noexcept = default;
    constexpr ordered_map_iterator& operator=(const ordered_map_iterator& other) noexcept = default;

    constexpr ordered_map_iterator(ordered_map_iterator&& other) noexcept = default;
    constexpr ordered_map_iterator& operator=(ordered_map_iterator&& other) noexcept = default;

    constexpr operator ordered_map_iterator<KeyT, const ValueT>() const noexcept {
        return ordered_map_iterator<KeyT, const ValueT>{key_ptr, value_ptr};
    }

    constexpr ordered_map_iterator &operator++() noexcept {
        ++key_ptr;
        ++value_ptr;
        return *this;
    }

    constexpr ordered_map_iterator operator++(int) noexcept {
        ordered_map_iterator tmp = *this;
        ++(*this);
        return tmp;
    }

    constexpr ordered_map_iterator& operator--() noexcept {
        --key_ptr;
        --value_ptr;
        return *this;
    }

    constexpr ordered_map_iterator operator--(int) noexcept {
        ordered_map_iterator tmp = *this;
        --(*this);
        return tmp;
    }

    constexpr ordered_map_iterator& operator+=(difference_type n) noexcept {
        key_ptr += n;
        value_ptr += n;
        return *this;
    }

    constexpr ordered_map_iterator& operator-=(difference_type n) noexcept {
        key_ptr -= n;
        value_ptr -= n;
        return *this;
    }

    constexpr ordered_map_iterator operator+(difference_type n) noexcept {
        return ordered_map_iterator(key_ptr + n, value_ptr + n);
    }

    friend constexpr ordered_map_iterator operator+(difference_type n, const ordered_map_iterator &it) noexcept {
        return ordered_map_iterator(it.key_ptr + n, it.value_ptr + n);
    }

    constexpr ordered_map_iterator<KeyT, std::remove_const_t<ValueT>> operator+(difference_type n) const noexcept {
        return ordered_map_iterator<KeyT, std::remove_const_t<ValueT>>(key_ptr + n, value_ptr + n);
    }

    constexpr ordered_map_iterator operator-(difference_type n) const noexcept {
        return ordered_map_iterator(key_ptr - n, value_ptr - n);
    }

    friend constexpr ordered_map_iterator operator-(difference_type n, const ordered_map_iterator &it) noexcept {
        return ordered_map_iterator(it.key_ptr - n, it.value_ptr - n);
    }

    constexpr difference_type operator-(const ordered_map_iterator& other) const noexcept {
        return key_ptr - other.key_ptr;
    }

    constexpr auto operator<=>(const ordered_map_iterator& other) const noexcept {
        return key_ptr <=> other.key_ptr;
    }

    constexpr bool
    operator==(const ordered_map_iterator &other) const noexcept {
        return key_ptr == other.key_ptr && value_ptr == other.value_ptr;
    }

    constexpr bool
    operator==(const ValueT *other_val_ptr) const noexcept {
        return value_ptr == other_val_ptr;
    }

    constexpr reference operator*() noexcept {
        return {do_cast(key_ptr), *value_ptr};
    }

    constexpr reference operator*() const noexcept {
        return {do_cast(key_ptr), *value_ptr};
    }

    constexpr pointer operator->() noexcept {
        return arrow_proxy{{do_cast(key_ptr), *value_ptr}};
    }

    constexpr pointer operator->() const noexcept {
        return arrow_proxy{{do_cast(key_ptr), *value_ptr}};
    }

    constexpr reference operator[](difference_type n) const noexcept {
        return {do_cast(key_ptr + n), *(value_ptr + n)};
    }
};
}; // namespace heurohash