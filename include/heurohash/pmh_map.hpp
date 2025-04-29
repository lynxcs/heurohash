#pragma once

#include "detail/traits.hpp"
#include "pmh_map_keyset.hpp"

#include "kvp_ptr_iterator.hpp"
#include "pmh_map_span.hpp"
#include <type_traits>
// #include "ordered_map_keyset.hpp" #include "ordered_map_span.hpp"

namespace heurohash {
namespace detail {
template <typename KeysetT, typename ValueT, bool is_backing>
class hash_map_collection {
    using KeyStorT =
        typename std::conditional_t<is_backing, KeysetT, const KeysetT &>;
    using ValueStorT = std::array<ValueT, KeysetT::keyset_size_v>;

    KeyStorT key_stor;
    ValueStorT value_stor;

    /* FIXME: Remove these redeclarations */
    static constexpr auto Size = KeysetT::keyset_size_v;
    using KeyT = KeysetT::key_type;

  public:
    using key_type = KeysetT::key_type;
    using mapped_type = ValueT;
    using value_type = ValueT;
    using pair_type = std::pair<key_type, value_type>;
    using size_type = size_t;
    using difference_type = std::ptrdiff_t;
    using reference = value_type &;
    using const_reference = const value_type &;
    using pointer = value_type *;
    using const_pointer = const value_type *;
    using iterator = kvp_ptr_iterator<key_type, ValueT>;
    using const_iterator = kvp_ptr_iterator<key_type, const ValueT>;

    /* Default initialize values */
    consteval hash_map_collection(KeyStorT &&key_storage) noexcept
        : key_stor(std::forward<KeyStorT>(key_storage)), value_stor{} {}

    template <typename IterT>
    consteval hash_map_collection(KeyStorT &&key_storage, IterT begin,
                                  IterT end) noexcept
        : key_stor(std::forward<KeyStorT>(key_storage)) {
        std::for_each(begin, end, [&](const auto &kvp) {
            auto idx = key_stor.find(kvp.first);
            constexpr_assert(idx != Size, "Key not found");
            value_stor[idx] = kvp.second;
        });
    }

    consteval hash_map_collection(
        KeyStorT &&keyset, std::initializer_list<pair_type> kvp_items) noexcept
        : hash_map_collection(std::forward<KeyStorT>(keyset), kvp_items.begin(),
                              kvp_items.end()) {}
    consteval hash_map_collection(
        KeyStorT &&keyset,
        const std::array<pair_type, Size> &kvp_items) noexcept
        : hash_map_collection(std::forward<KeyStorT>(keyset), kvp_items.begin(),
                              kvp_items.end()) {}
    consteval hash_map_collection(KeyStorT &&keyset,
                                  const pair_type (&kvp_items)[Size]) noexcept
        : hash_map_collection(std::forward<KeyStorT>(keyset),
                              std::begin(kvp_items), std::end(kvp_items)) {}

    constexpr hash_map_collection(const hash_map_collection &) noexcept =
        default;
    constexpr hash_map_collection &
    operator=(const hash_map_collection &) noexcept = default;

    constexpr hash_map_collection(hash_map_collection &&) noexcept = default;
    constexpr hash_map_collection &
    operator=(hash_map_collection &&) noexcept = default;

    constexpr ValueT *find(const key_type &key) noexcept {
        return value_stor.begin() + key_stor.find(key);
    }

    constexpr const ValueT *find(const key_type &key) const noexcept {
        return value_stor.cbegin() + key_stor.find(key);
    }

    constexpr ValueT &operator[](const KeyT &key) noexcept {
        return value_stor[key_stor.find(key)];
    }

    constexpr ValueT const &operator[](const KeyT &key) const noexcept {
        return value_stor[key_stor.find(key)];
    }

    constexpr ValueT &at(const KeyT &key) noexcept {
        auto idx = key_stor.find(key);
        constexpr_assert(idx != Size, "Key not found");
        return value_stor[idx];
    }

    constexpr ValueT const &at(const KeyT &key) const noexcept {
        auto idx = key_stor.find(key);
        constexpr_assert(idx != Size, "Key not found");
        return value_stor[idx];
    }

    constexpr size_type count(const key_type &key) const noexcept {
        return contains() ? 1 : 0;
    }

    constexpr bool contains(const key_type &key) const noexcept {
        return key_stor.contains(key);
    }

    /* Capacity */
    constexpr bool empty() const noexcept { return Size == 0; }

    constexpr size_t size() const noexcept { return Size; }

    constexpr size_t max_size() const noexcept { return Size; }

    /* Iterators */
    constexpr iterator begin() noexcept {
        return iterator{key_stor.begin(), value_stor.begin()};
    }

    constexpr const_iterator begin() const noexcept {
        return const_iterator{key_stor.begin(), value_stor.cbegin()};
    }

    constexpr iterator end() noexcept {
        return iterator{key_stor.end(), value_stor.end()};
    }

    constexpr const_iterator end() const noexcept {
        return const_iterator{key_stor.end(), value_stor.end()};
    }

    constexpr void clear() noexcept { value_stor.fill(ValueT{}); }

    constexpr operator hash_map_span<KeyT, ValueT>() noexcept {
        return to_span();
    }

    constexpr operator hash_map_span<KeyT, const ValueT>() const noexcept {
        return to_span();
    }

    constexpr hash_map_span<KeyT, ValueT> to_span() noexcept {
        return hash_map_span<KeyT, ValueT>{&key_stor, value_stor.data()};
    }

    constexpr hash_map_span<KeyT, const ValueT> to_span() const noexcept {
        return hash_map_span<KeyT, const ValueT>{&key_stor, value_stor.data()};
    }
};
} // namespace detail

template <typename KeysetT, typename ValueT>
using hash_map = detail::hash_map_collection<KeysetT, ValueT, true>;

static consteval auto make_hash_map(comp_time auto builder) noexcept {
    constexpr auto values = builder();
    static_assert(lookup::detail::is_arr_kvp(values));
    using ValueStorT = decltype(lookup::detail::get_values(values));
    using ValueT = std::remove_cv_t<typename ValueStorT::value_type>;
    using KeysetT = decltype(make_hash_keyset(builder));
    return hash_map<KeysetT, ValueT>{make_hash_keyset(builder), values.begin(),
                                     values.end()};
}

template <typename KeysetT, typename ValueT>
static consteval auto
make_hash_map(KeysetT keyset,
              std::span<const std::pair<typename KeysetT::key_type, ValueT>,
                        KeysetT::keyset_size_v>
                  values) noexcept {
    return hash_map{keyset, values.begin(), values.end()};
}

template <typename KeysetT>
static consteval auto make_hash_map(KeysetT keyset) noexcept {
    return hash_map{keyset};
}

template <typename KeyT, typename ValueT>
using hash_map_valueset =
    detail::hash_map_collection<std::remove_cvref_t<KeyT>, ValueT, false>;

static consteval auto make_hash_valueset(comp_time auto builder) noexcept {
    constexpr auto values = builder();
    static constexpr auto keyset = make_hash_keyset(builder);
    using ValueStorT = decltype(lookup::detail::get_values(values));
    using ValueT = std::remove_cv_t<typename ValueStorT::value_type>;
    return hash_map_valueset<decltype(keyset), ValueT>{keyset, values.begin(),
                                                       values.end()};
}

template <typename KeysetT, typename ValueT>
static consteval auto make_hash_valueset(
    const KeysetT &keyset,
    std::span<const std::pair<typename KeysetT::key_type, ValueT>,
              KeysetT::keyset_size_v>
        values) noexcept {
    return hash_map_valueset{keyset, values.begin(), values.end()};
}

template <typename KeysetT, typename ValueT>
static consteval auto make_hash_valueset(const KeysetT &keyset) noexcept {
    return hash_map_valueset<KeysetT, ValueT>{keyset};
}

} // namespace heurohash
