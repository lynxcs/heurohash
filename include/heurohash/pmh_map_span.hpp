#pragma once

#include <algorithm>
#include <functional>
#include <memory>
#include <numeric>

#include "detail/traits.hpp"
#include "kvp_ptr_iterator.hpp"
#include "pmh_map_keyset.hpp"

namespace heurohash {

namespace detail {
/* FWD declare ordered_map & ordered_map_valueset for span friend */
template <typename KeySetT, typename ValueT, bool is_backing>
class hash_map_collection;

} // namespace detail

static consteval auto make_hash_span(comp_time auto builder) noexcept;

/* Span of linear map (aka desized, to allow better 'anonymous' interfaces) */
template <typename KeyT, typename ValueT> class hash_map_span {
    /* Need this extra layer of indirection */
    /* Because LUT element size depends on array size */
    union PseudoIndirFuncRes {
        size_t size;
        const KeyT *key_storage;
    };
    enum class PseudoIndirQueryOpt : uint8_t { GetSize = 0, GetStorage = 1 };
    using PseudoIndirQueryFunc =
        PseudoIndirFuncRes (*)(const void *ptr, PseudoIndirQueryOpt opt);
    using PseudoIndirLookupFunc = size_t (*)(const void *ptr, const KeyT &key);

    const void *pseudo_indirect_ptr;
    PseudoIndirQueryFunc pseudo_indirect_query_func;
    PseudoIndirLookupFunc pseudo_indirect_lookup_func;
    ValueT *value_storage;

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
    using iterator = kvp_ptr_iterator<KeyT, ValueT>;
    using const_iterator = kvp_ptr_iterator<KeyT, const ValueT>;

  protected:
    template <typename KeysetT, typename Value, bool is_backing>
    friend class detail::hash_map_collection;

    friend consteval auto make_hash_span(comp_time auto builder) noexcept;

    /* FIXME: This _might_ be not possible at constexpr-time */
    template <typename KeysetT, typename ValueStor>
    explicit constexpr hash_map_span(const KeysetT *keyset,
                                     ValueStor *stor_ptr) noexcept
        : pseudo_indirect_ptr{keyset},
          pseudo_indirect_query_func{
              [](const void *ptr,
                 PseudoIndirQueryOpt opt) constexpr -> PseudoIndirFuncRes {
                  const auto *set = reinterpret_cast<const KeysetT *>(ptr);
                  switch (opt) {
                  case PseudoIndirQueryOpt::GetSize: {
                      return PseudoIndirFuncRes{.size = set->size()};
                  } break;
                  case PseudoIndirQueryOpt::GetStorage: {
                      return PseudoIndirFuncRes{.key_storage = set->begin()};
                  } break;
                  default:
                      std::unreachable();
                      break;
                  }
              }},
          pseudo_indirect_lookup_func{
              [](const void *ptr, const KeyT &key) constexpr {
                  const auto *set = reinterpret_cast<const KeysetT *>(ptr);
                  return set->find(key);
              }},
          value_storage{stor_ptr} {}

  private:
    explicit constexpr hash_map_span(const void *ptr,
                                     PseudoIndirQueryFunc query_func,
                                     PseudoIndirLookupFunc lookup_func,
                                     ValueT *val_stor) noexcept
        : pseudo_indirect_ptr{ptr}, pseudo_indirect_query_func{query_func},
          pseudo_indirect_lookup_func{lookup_func}, value_storage{val_stor} {}

  public:
    constexpr hash_map_span(const hash_map_span &) noexcept = default;
    constexpr hash_map_span(hash_map_span &&) noexcept = default;
    constexpr hash_map_span &
    operator=(const hash_map_span &) noexcept = default;
    constexpr hash_map_span &operator=(hash_map_span &&) noexcept = default;

    constexpr operator hash_map_span<KeyT, const ValueT>() const noexcept {
        return hash_map_span<KeyT, const ValueT>{
            pseudo_indirect_ptr, pseudo_indirect_query_func,
            pseudo_indirect_lookup_func, value_storage};
    }

    /* Lookup */
    constexpr ValueT *find(const KeyT &key) const noexcept {
        return value_storage + find_impl(key);
    }

    /* Faster than going through process checking with end()? */
    constexpr bool find_check(const ValueT *loc) const noexcept {
        return loc != loc + size();
    }

    constexpr reference operator[](const KeyT &key) const noexcept {
        return value_storage[find_impl(key)];
    }

    constexpr reference at(const KeyT &key) const noexcept {
        auto idx = find_impl(key);
        constexpr_assert(idx != size(), "Key not found");
        return value_storage[find_impl(key)];
    }

    constexpr size_type count(const KeyT &key) const noexcept {
        return contains() ? 1 : 0;
    }

    constexpr bool contains(const KeyT &key) const noexcept {
        return find_impl(key) != size();
    }

    constexpr bool empty() const noexcept { return size() == 0; }

    constexpr size_t size() const noexcept {
        return pseudo_indirect_query_func(pseudo_indirect_ptr,
                                          PseudoIndirQueryOpt::GetSize)
            .size;
    }

    constexpr size_t max_size() const noexcept { return size(); }

    constexpr iterator begin() const noexcept {
        return iterator{get_key_stor(), value_storage};
    }

    constexpr iterator end() const noexcept {
        const auto sz = size();
        return iterator{get_key_stor() + sz, value_storage + sz};
    }

    constexpr void clear() noexcept {
        std::fill(value_storage, value_storage + size(), ValueT{});
    }

  private:
    constexpr auto get_key_stor() const noexcept {
        return pseudo_indirect_query_func(pseudo_indirect_ptr,
                                          PseudoIndirQueryOpt::GetStorage)
            .key_storage;
    }

    constexpr size_t find_impl(const KeyT &key) const noexcept {
        return pseudo_indirect_lookup_func(pseudo_indirect_ptr, key);
    }
};

/* Hash span generate this way is faster than span generated from
 * existing hash_map - this is because we can re-use the extra
 * type info to choose the compile-time known size variants of lookup */
static consteval auto make_hash_span(comp_time auto builder) noexcept {
    constexpr auto values = builder();
    static_assert(lookup::detail::is_arr_kvp(values));

    using builder_ret_t = decltype(lookup::detail::get_orig_keys(builder()));
    static constexpr auto static_stor_backing =
        lookup::pseudo_pext_lookup<detail::hash_map_pnext_depth>::make(builder);
    static constexpr auto static_value_backing =
        lookup::detail::get_values(values);

    return hash_map_span<
        typename builder_ret_t::value_type,
        const typename decltype(static_value_backing)::value_type>{
        &static_stor_backing, static_value_backing.data()};
}

}; // namespace heurohash
