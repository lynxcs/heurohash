#pragma once

#include <algorithm>
#include <cstddef>
#include <functional>
#include <utility>

#include "detail/pseudo_pext_lookup.hpp"
#include "detail/traits.hpp"

/* FIXME: Allow passing custom hashers? */

namespace heurohash {
namespace detail {
static constexpr inline size_t hash_map_pnext_depth = 2;
template <typename KeyT, size_t Size, size_t LutSize, size_t Depth>
using pseudo_next_t = lookup::pseudo_next_indirect<
    std::array<KeyT, Size>, std::array<lookup::lookup_idx_exp_t<Size>, LutSize>,
    Depth>;
} // namespace detail
template <typename KeyT, size_t Size, size_t LutSize, size_t Depth>
class hash_map_keyset {
    using KeyUnderlyingT = detail::underlying_type<KeyT>;
    using KeyValT = std::remove_cv_t<KeyT>;
    using KeyStorageT = std::remove_cv_t<std::array<KeyValT, Size>>;
    using LookupT = detail::pseudo_next_t<KeyT, Size, LutSize, Depth>;

    LookupT storage;

  public:
    static constexpr size_t keyset_size_v = Size;
    static constexpr size_t keyset_lut_size_v = LutSize;
    static constexpr size_t keyset_depth_v = Depth;

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

    using storage_type = KeyValT;

    consteval hash_map_keyset(LookupT stor) noexcept : storage(stor) {}

    constexpr hash_map_keyset(const hash_map_keyset &) noexcept = default;
    constexpr hash_map_keyset &
    operator=(const hash_map_keyset &) noexcept = default;

    constexpr hash_map_keyset(hash_map_keyset &&) noexcept = default;
    constexpr hash_map_keyset &operator=(hash_map_keyset &&) noexcept = default;

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

    constexpr const_iterator begin() const noexcept {
        return storage.key_storage.cbegin();
    }

    constexpr const_iterator end() const noexcept {
        return storage.key_storage.cend();
    }

  private:
    constexpr size_t find_impl(const KeyT &key) const noexcept {
        return storage.lookup(key);
    }
};

// template <typename KeyT, size_t Size, size_t LutSize, size_t Depth>
// class hash_map_keyset<KeyT, Size, LutSize, Depth>->hash_map_keysetjk
//
template <typename KeyT, size_t Size, size_t LutSize, size_t Depth>
hash_map_keyset(detail::pseudo_next_t<KeyT, Size, LutSize, Depth>)
    -> hash_map_keyset<KeyT, Size, LutSize, Depth>;

static consteval auto make_hash_keyset(comp_time auto builder) noexcept {
    using builder_ret_t = decltype(lookup::detail::get_orig_keys(builder()));

    /* FIXME: This forces us back to C++23. Also full struct might be stored in
     * binary?  */
    // static constexpr auto static_stor_backing =
    //     lookup::pseudo_pext_lookup<detail::hash_map_pnext_depth>::make(builder);
    constexpr auto data =
        lookup::pseudo_pext_lookup<detail::hash_map_pnext_depth>::make(builder);
    using data_t = decltype(data);
    return hash_map_keyset<typename data_t::key_type, data.size(),
                           data.lut_size(), data.depth()>{data};

    // return hash_map_keyset<typename builder_ret_t::value_type,
    //                        std::tuple_size_v<builder_ret_t>>{
    //     static_stor_backing.to_dyn_lut()};
}

static constexpr auto kst =
    make_hash_keyset([]() consteval { return std::array{1, 2, 3}; });
static_assert(kst.size() == 3);
static_assert(kst.find(1) == 2);
static_assert(kst.find(2) == 0);
static_assert(kst.find(3) == 1);
static_assert(kst.find(5) == 3);
static_assert(kst.find(8221) == 3);

}; // namespace heurohash
