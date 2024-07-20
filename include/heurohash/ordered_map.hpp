#pragma once

#include <algorithm>
#include <functional>
#include <utility>

#include "detail/traits.hpp"
#include "detail/branchless_lower_bound.hpp"

namespace heurohash {
namespace detail {
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
} // namespace detail

/* Split into two parts allows us to store key part completely in ROM, and the
 * value part in RAM. Storage internally is kept as the underlying type. This
 * includes the Compare, so when passing non-default ensure that it uses
 * underlying type. While this might appear strange the explanation is simple -
 * this allows us to reduce binary size by re-using the lower_bound
 * implementation for multiple enum. Assuming that they have the same underlying
 * type. Which (by default) is int.*/
template <typename KeyT, size_t Size,
          typename KeyStorT = detail::underlying_type<KeyT>,
          typename Compare = std::less<KeyStorT>>
class ordered_map_keyset {
    /* KeyStorageOverride allows to 'override' the storage with a smaller size.
     * Values are checked, and if any of passed Keys of KeyT don't fit into
     * KeyStorageOverride type - compile error */
    using KeyUnderlyingT = detail::underlying_type<KeyT>;
    using KeyValT = std::remove_cv_t<KeyStorT>;
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
        std::transform(first, last, keys.begin(), [](auto key) {
            return static_cast<const KeyValT>(key);
        });
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
        if constexpr (!std::is_same_v<KeyValT, detail::underlying_type<KeyT>>) {
            if (!std::in_range<KeyValT>(static_cast<KeyUnderlyingT>(key))) {
                return Size;
            }
        }
        return detail::ordered_find_impl<KeyValT, Compare>(
            keys.data(), keys.size(), static_cast<KeyValT>(key), compare);
    }
};

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
    using pointer           = value_type;
    using reference         = value_type;

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

    constexpr reference operator[](difference_type n) const noexcept {
        return {do_cast(key_ptr + n), *(value_ptr + n)};
    }
};

static_assert(std::random_access_iterator<ordered_map_iterator<int, void*>>,
                "ordered_map_iterator must be a random access iterator");


/* FWD declare ordered_map for span friend */
template <typename KeyT, typename ValueT, size_t Size, typename KeyStorT,
          typename Compare>
class ordered_map;

/* Span of linear map (aka desized, to allow better 'anonymous' interfaces) */
template <typename KeyT, typename ValueT,
          typename KeyStorT = detail::underlying_type<KeyT>,
          typename Compare = std::less<KeyStorT>>
class ordered_map_span {
    const KeyStorT *key_storage;
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
    template <typename Key, typename Value, size_t Size, typename KeyStor,
              typename Comp>
    friend class ordered_map;

    explicit constexpr ordered_map_span(
        const KeyStorT *keys, ValueT *values, size_t size,
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

    constexpr operator ordered_map_span<KeyT, const ValueT, KeyStorT, Compare>()
        const noexcept {
        return ordered_map_span<KeyT, const ValueT, KeyStorT, Compare>(
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
        return detail::ordered_find_impl<KeyStorT, Compare>(
            key_storage, stor_size, static_cast<KeyStorT>(key), compare);
    }
};

template <typename KeyT, typename ValueT, size_t Size,
          typename KeyStorT = detail::underlying_type<KeyT>,
          typename Compare = std::less<KeyStorT>>
class ordered_map {
    using StorageT = std::array<ValueT, Size>;
    ordered_map_keyset<KeyT, Size, KeyStorT, Compare> keyset;
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
    operator ordered_map_span<KeyT, ValueT, KeyStorT, Compare>() noexcept {
        return ordered_map_span<KeyT, ValueT, KeyStorT, Compare>(
            keyset.begin(), values.data(), Size, keyset.key_comp());
    }

    constexpr operator ordered_map_span<KeyT, const ValueT, KeyStorT, Compare>()
        const noexcept {
        return ordered_map_span<KeyT, const ValueT, KeyStorT, Compare>(
            keyset.begin(), values.data(), Size, keyset.key_comp());
    }
};

template <typename KeyT, typename ValueT, size_t Size,
          typename KeyStorT = detail::underlying_type<KeyT>,
          typename Compare = std::less<KeyStorT>>
class ordered_map_valueset {
    using StorageT = std::array<ValueT, Size>;
    using KeysetT = ordered_map_keyset<KeyT, Size, KeyStorT, Compare>;
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
    consteval ordered_map_valueset(const KeysetT &keyset, InputIt first, InputIt last) noexcept
        : keyset(keyset) {
        /* Copy over data if iterator is KVP */
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

    constexpr ordered_map_valueset(const ordered_map_valueset &) noexcept = default;
    constexpr ordered_map_valueset &operator=(const ordered_map_valueset &) noexcept = default;

    constexpr ordered_map_valueset(ordered_map_valueset &&) noexcept = default;
    constexpr ordered_map_valueset &operator=(ordered_map_valueset &&) noexcept = default;

    constexpr ValueT* find(const key_type &key) noexcept {
        return values.begin() + keyset.find(key);
    }

    constexpr const ValueT* find(const key_type &key) const noexcept {
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

    constexpr iterator end() noexcept {return iterator{keyset.end(), values.end()}; }

    constexpr const_iterator end() const noexcept {return iterator{keyset.end(), values.cend()}; }

    constexpr void clear() noexcept { values.fill(ValueT{}); }

    constexpr
    operator ordered_map_span<KeyT, ValueT, KeyStorT, Compare>() noexcept {
        return ordered_map_span<KeyT, ValueT, KeyStorT, Compare>(
            keyset.begin(), values.data(), Size, keyset.key_comp());
    }

    constexpr operator ordered_map_span<KeyT, const ValueT, KeyStorT, Compare>()
        const noexcept {
        return ordered_map_span<KeyT, const ValueT, KeyStorT, Compare>(
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
    return ordered_map<T, U, N, detail::underlying_type<T>, Compare>{items,
                                                                     compare};
}

template <typename T, typename U, typename Compare, std::size_t N>
static consteval auto
make_ordered_map(std::array<std::pair<T, U>, N> const &items,
                 Compare const &compare = Compare{}) {
    return ordered_map<T, U, N, detail::underlying_type<T>, Compare>{items,
                                                                     compare};
}

template <typename T, typename U,
          typename OverrideT = detail::underlying_type<T>, typename Compare,
          std::size_t N>
static consteval auto make_ordered_map(std::pair<T, U> const (&items)[N],
                                       Compare const &compare = Compare{}) {
    return ordered_map<T, U, N, Compare>{items, compare};
}

template <typename T, typename U,
          typename OverrideT = detail::underlying_type<T>, typename Compare,
          std::size_t N>
static consteval auto
make_ordered_map(std::array<std::pair<T, U>, N> const &items,
                 Compare const &compare = Compare{}) {
    return ordered_map<T, U, N, Compare>{items, compare};
}

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
    return ordered_map_keyset<T, N, detail::underlying_type<T>, Compare>{
        items, compare};
}

template <typename T, typename Compare, std::size_t N>
static consteval auto make_ordered_keyset(std::array<T, N> const &items,
                                          Compare const &compare = Compare{}) {
    return ordered_map_keyset<T, N, detail::underlying_type<T>, Compare>{
        items, compare};
}

template <typename T, typename OverrideT = detail::underlying_type<T>,
          typename Compare, std::size_t N>
static consteval auto make_ordered_keyset(T const (&items)[N],
                                          Compare const &compare = Compare{}) {
    return ordered_map_keyset<T, N, Compare>{items, compare};
}

template <typename T, typename OverrideT = detail::underlying_type<T>,
          typename Compare, std::size_t N>
static consteval auto make_ordered_keyset(std::array<T, N> const &items,
                                          Compare const &compare = Compare{}) {
    return ordered_map_keyset<T, N, Compare>{items, compare};
}

template <typename T, typename U, std::size_t N>
static consteval auto make_ordered_map_valueset(const ordered_map_keyset<T, N> &keyset) {
    return ordered_map_valueset<T, U, N>{keyset};
}

template <typename T, typename U, std::size_t N>
static consteval auto make_ordered_map_valueset(std::pair<T, U> const (&items)[N]) {
    return ordered_map_valueset<T, U, N>{items};
}

template <typename T, typename U, std::size_t N>
static consteval auto
make_ordered_map_valueset(std::array<std::pair<T, U>, N> const &items) {
    return ordered_map_valueset<T, U, N>{items};
}

template <typename T, typename U, typename Compare, std::size_t N>
static consteval auto make_ordered_map_valueset(std::pair<T, U> const (&items)[N],
                                       Compare const &compare = Compare{}) {
    return ordered_map_valueset<T, U, N, detail::underlying_type<T>, Compare>{items,
                                                                     compare};
}

template <typename T, typename U, typename Compare, std::size_t N>
static consteval auto
make_ordered_map_valueset(std::array<std::pair<T, U>, N> const &items,
                 Compare const &compare = Compare{}) {
    return ordered_map_valueset<T, U, N, detail::underlying_type<T>, Compare>{items,
                                                                     compare};
}

template <typename T, typename U,
          typename OverrideT = detail::underlying_type<T>, typename Compare,
          std::size_t N>
static consteval auto make_ordered_map_valueset(std::pair<T, U> const (&items)[N],
                                       Compare const &compare = Compare{}) {
    return ordered_map_valueset<T, U, N, Compare>{items, compare};
}

template <typename T, typename U,
          typename OverrideT = detail::underlying_type<T>, typename Compare,
          std::size_t N>
static consteval auto
make_ordered_map_valueset(std::array<std::pair<T, U>, N> const &items,
                 Compare const &compare = Compare{}) {
    return ordered_map_valueset<T, U, N, Compare>{items, compare};
}

}; // namespace heurohash
