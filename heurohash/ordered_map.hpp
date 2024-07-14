#pragma once

#include <concepts>
#include <functional>
#include <string>
#include <span>
#include <cassert>
#include <algorithm>
#include <utility>
#include <bit>
#include <cstdint>
#include <span>

#define constexpr_assert(cond, msg) \
    assert(cond && msg)

namespace heurohash {
    template <typename KeyT, typename Compare>
    static constexpr size_t find_impl_r(const KeyT *keys, size_t size, const KeyT &key) noexcept {
        auto it = std::lower_bound(keys, keys + size, key, Compare{});
        if ((it != (keys + size)) && (static_cast<KeyT>(*it) == key)) {
            return std::distance(keys, it);
        }
        return size;
    }

    template<typename T>
    using UnderlyingType = std::remove_cv_t<std::conditional_t<std::is_enum_v<T>, std::underlying_type<T>, std::type_identity<T>>>::type;

    /* Split into two parts allows us to store key part completely in ROM, and the value part in RAM */
    /* Storage internally is kept as the underlying type. This includes the Compare, so when passing non-default ensure that it uses underlying type. */
    /* ^ While this might appear strange the explanation is simple - this allows us to reduce binary size by re-using the lower_bound implementation for multiple enums */
    /* Assuming that they have the same underlying type. Which (by default) is int. */
    template <typename KeyT, size_t Size, typename KeyStorageOverrideT = UnderlyingType<KeyT>, typename Compare = std::less<KeyStorageOverrideT>>
    class ordered_map_keyset {
        /* KeyStorageOverride allows to 'override' the storage with a smaller size. */
        /* Values are checked, and if any of passed Keys of KeyT don't fit into KeyStorageOverride type - compile error */
        static_assert(!std::is_reference_v<KeyT>, "Key type cannot be reference");

        static constexpr void check_override() {
            if constexpr(!std::is_same_v<UnderlyingType<KeyT>, KeyStorageOverrideT>) {
                static_assert(std::is_integral_v<KeyStorageOverrideT>, "Override type must be integral");
                static_assert(sizeof(KeyStorageOverrideT) < sizeof(KeyT), "Size of override must be smaller");
            }
        }

        using KeyUnderlyingT = UnderlyingType<KeyT>;
        using KeyValT = std::remove_cv_t<KeyStorageOverrideT>;
        using KeyStorageT = std::array<KeyValT, Size>;

        KeyStorageT keys{};

      public:
        static constexpr size_t keyset_size = Size;

        /* Member types */
        using key_type = KeyT;
        using value_type = size_t;
        using size_type = size_t;
        using difference_type = std::ptrdiff_t;
        using reference = key_type&;
        using const_reference = const value_type&;
        using pointer = value_type*;
        using const_pointer = const value_type*;

        template <size_t ArraySize>
        consteval ordered_map_keyset(const KeyT (&arr)[ArraySize]) {
            static_assert(ArraySize == Size, "Passed array size doesn't match");
            check_override();
            std::transform(std::begin(arr), std::end(arr), keys.begin(), [](auto key){
                if constexpr (!std::is_same_v<KeyValT, KeyT>) {
                    constexpr_assert(std::in_range<KeyValT>(static_cast<KeyUnderlyingT>(key)), "Key doesn't fit into override");
                }
                return static_cast<const KeyValT>(key);
            });
            sort_keys();
        }

        constexpr value_type find(const KeyT &key) noexcept {
            return find_impl(key);
        }

        constexpr value_type find(const KeyT &key) const noexcept {
            return find_impl(key);
        }

        constexpr size_type count(const KeyT& key) const noexcept {
            return contains() ? 1 : 0;
        }

        constexpr bool contains(const KeyT& key) const noexcept {
            return find_impl(key) != Size;
        }

        /* Capacity */
        constexpr bool empty() const noexcept {
            return Size == 0;
        }

        constexpr size_t size() const noexcept {
            return Size;
        }

        constexpr size_t max_size() const noexcept {
            return Size;
        }
      private:
        template <typename FromT, typename ToT>
        struct CompareWrapper {
            constexpr bool operator()(const FromT& lhs, const FromT& rhs) const 
            {
                return Compare{}(static_cast<ToT>(lhs),static_cast<ToT>(rhs));
            }

            constexpr bool operator()(const FromT& lhs, const ToT& rhs) const 
            {
                return Compare{}(static_cast<ToT>(lhs),rhs);
            }

            constexpr bool operator()(const ToT& lhs, const FromT& rhs) const 
            {
                return Compare{}(lhs,static_cast<ToT>(rhs));
            }

            constexpr bool operator()(const ToT& lhs, const ToT& rhs) const 
            {
                return Compare{}(lhs,rhs);
            }
        };

        using RealCompare = std::conditional_t<std::is_same_v<KeyValT, UnderlyingType<KeyT>>, Compare, CompareWrapper<KeyValT, KeyT>>;
      
        constexpr void sort_keys() noexcept {
            std::sort(keys.begin(), keys.end(), Compare{});
            auto adjacent_val = std::adjacent_find(keys.cbegin(), keys.cend());
            constexpr_assert(adjacent_val == keys.cend(), "Duplicate entries in keys");
        }

        constexpr size_t find_impl(const KeyT &key) const noexcept {
            if constexpr (!std::is_same_v<KeyValT, UnderlyingType<KeyT>>) {
                if (!std::in_range<KeyValT>(static_cast<KeyUnderlyingT>(key))) {
                    return Size;
                }
            }
            return find_impl_r<KeyValT, Compare>(keys.data(), keys.size(), static_cast<KeyValT>(key));
        }
    };

    template<typename ValueT,  typename KeysetT>
    class ordered_map {
        const KeysetT &keyset;

        using StorageT = std::array<ValueT, KeysetT::keyset_size>;
        StorageT data{};
        
      public:
        using key_type = KeysetT::key_type;
        using mapped_type = ValueT;
        using value_type = mapped_type;
        using size_type = size_t;
        using difference_type = std::ptrdiff_t;
        using reference = value_type&;
        using const_reference = const value_type&;
        using pointer = value_type*;
        using const_pointer = const value_type*;
        using iterator = typename StorageT::iterator;
        using const_iterator = typename StorageT::const_iterator;
        
        template <size_t ArraySize>
        consteval ordered_map(const KeysetT &keyset, const std::pair<key_type, value_type> (&arr)[ArraySize]) noexcept : keyset(keyset) {
            static_assert(ArraySize == KeysetT::keyset_size, "Passed array size doesn't match");
            check_values(arr);
        }

        constexpr iterator find(const key_type &key) noexcept {
            return data.begin() + find_impl(key);
        }

        constexpr const_iterator find(const key_type &key) const noexcept {
            return data.begin() + find_impl(key);
        }

        constexpr size_type count(const key_type& key) const noexcept {
            return contains() ? 1 : 0;
        }

        constexpr bool contains(const key_type& key) const noexcept {
            return find_impl(key) != end();
        }

        /* Capacity */
        constexpr bool empty() const noexcept {
            return KeysetT::keyset_size == 0;
        }

        constexpr size_t size() const noexcept {
            return KeysetT::keyset_size;
        }

        constexpr size_t max_size() const noexcept {
            return KeysetT::keyset_size;
        }
        
        /* Iterators */
        constexpr iterator begin() noexcept {
            return data.begin();
        }

        constexpr const_iterator begin() const noexcept {
            return data.cbegin();
        }

        constexpr iterator end() noexcept {
            return data.end();
        }

        constexpr const_iterator end() const noexcept {
            return data.cend();
        }

        constexpr const_iterator cbegin() const noexcept {
            return data.cbegin();
        }

        constexpr const_iterator cend() const noexcept {
            return data.cend();
        }

      private:

        constexpr void check_values(std::span<const std::pair<key_type, value_type>> values) noexcept {
            std::array<bool, KeysetT::keyset_size> occurance{false};
            for(auto &kvp : values) {
                auto idx = keyset.find(kvp.first);
                constexpr_assert(idx < keyset.size(), "KVP outside range");
                constexpr_assert(occurance[idx] == false, "Duplicate KVP in range");
                occurance[idx] = true;
                data[idx] = kvp.second;
            }
        }

        constexpr size_t find_impl(const key_type &key) const noexcept {
            return keyset.find(key);
        }
    };
};
