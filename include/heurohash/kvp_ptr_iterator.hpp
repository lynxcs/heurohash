#pragma once

#include "detail/traits.hpp"
#include <iterator>

namespace heurohash {
template <typename KeyT, typename ValueT> class kvp_ptr_iterator {
    const KeyT *key_ptr;
    ValueT *value_ptr;

  public:
    using iterator_category = std::random_access_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = std::pair<const KeyT &, ValueT &>;
    using reference = value_type;

    /* Inspiration for arrow proxy taken from:
     * https://github.com/WG21-SG14/SG14/blob/master/SG14/flat_map.h */
    struct arrow_proxy {
        reference *operator->() { return std::addressof(data_); }
        reference data_;
    };
    using pointer = arrow_proxy;

    constexpr kvp_ptr_iterator(const KeyT *key_ptr, ValueT *value_ptr) noexcept
        : key_ptr(key_ptr), value_ptr(value_ptr) {}

    constexpr kvp_ptr_iterator() noexcept
        : key_ptr(nullptr), value_ptr(nullptr) {}

    constexpr kvp_ptr_iterator(const kvp_ptr_iterator &other) noexcept =
        default;
    constexpr kvp_ptr_iterator &
    operator=(const kvp_ptr_iterator &other) noexcept = default;

    constexpr kvp_ptr_iterator(kvp_ptr_iterator &&other) noexcept = default;
    constexpr kvp_ptr_iterator &
    operator=(kvp_ptr_iterator &&other) noexcept = default;

    constexpr operator kvp_ptr_iterator<KeyT, const ValueT>() const noexcept {
        return kvp_ptr_iterator<KeyT, const ValueT>{key_ptr, value_ptr};
    }

    constexpr kvp_ptr_iterator &operator++() noexcept {
        ++key_ptr;
        ++value_ptr;
        return *this;
    }

    constexpr kvp_ptr_iterator operator++(int) noexcept {
        kvp_ptr_iterator tmp = *this;
        ++(*this);
        return tmp;
    }

    constexpr kvp_ptr_iterator &operator--() noexcept {
        --key_ptr;
        --value_ptr;
        return *this;
    }

    constexpr kvp_ptr_iterator operator--(int) noexcept {
        kvp_ptr_iterator tmp = *this;
        --(*this);
        return tmp;
    }

    constexpr kvp_ptr_iterator &operator+=(difference_type n) noexcept {
        key_ptr += n;
        value_ptr += n;
        return *this;
    }

    constexpr kvp_ptr_iterator &operator-=(difference_type n) noexcept {
        key_ptr -= n;
        value_ptr -= n;
        return *this;
    }

    constexpr kvp_ptr_iterator operator+(difference_type n) noexcept {
        return kvp_ptr_iterator(key_ptr + n, value_ptr + n);
    }

    friend constexpr kvp_ptr_iterator
    operator+(difference_type n, const kvp_ptr_iterator &it) noexcept {
        return kvp_ptr_iterator(it.key_ptr + n, it.value_ptr + n);
    }

    constexpr kvp_ptr_iterator<KeyT, std::remove_const_t<ValueT>>
    operator+(difference_type n) const noexcept {
        return kvp_ptr_iterator<KeyT, std::remove_const_t<ValueT>>(
            key_ptr + n, value_ptr + n);
    }

    constexpr kvp_ptr_iterator operator-(difference_type n) const noexcept {
        return kvp_ptr_iterator(key_ptr - n, value_ptr - n);
    }

    friend constexpr kvp_ptr_iterator
    operator-(difference_type n, const kvp_ptr_iterator &it) noexcept {
        return kvp_ptr_iterator(it.key_ptr - n, it.value_ptr - n);
    }

    constexpr difference_type
    operator-(const kvp_ptr_iterator &other) const noexcept {
        return key_ptr - other.key_ptr;
    }

    constexpr auto operator<=>(const kvp_ptr_iterator &other) const noexcept {
        return key_ptr <=> other.key_ptr;
    }

    constexpr bool operator==(const kvp_ptr_iterator &other) const noexcept {
        return key_ptr == other.key_ptr && value_ptr == other.value_ptr;
    }

    constexpr bool operator==(const ValueT *other_val_ptr) const noexcept {
        return value_ptr == other_val_ptr;
    }

    constexpr reference operator*() noexcept { return {*key_ptr, *value_ptr}; }

    constexpr reference operator*() const noexcept {
        return {*key_ptr, *value_ptr};
    }

    constexpr pointer operator->() noexcept {
        return arrow_proxy{{*key_ptr, *value_ptr}};
    }

    constexpr pointer operator->() const noexcept {
        return arrow_proxy{{*key_ptr, *value_ptr}};
    }

    constexpr reference operator[](difference_type n) const noexcept {
        return {*(key_ptr + n), *(value_ptr + n)};
    }
};
}; // namespace heurohash
