#pragma once

#include <concepts>
#include <functional>
#include <string>
#include <span>
#include <cassert>
#include <algorithm>

#define constexpr_assert(cond, msg) \
    assert(cond && msg)

namespace heurohash {
    template <typename KeyT, typename ValueT, size_t Size>
    /* Most space-efficient map implementation */
    /* Keys are discarded, since they are essentially just indices */
    class linear_map {
        static_assert(!std::is_reference_v<KeyT>, "Key type cannot be reference");
        /* Interal data*/
        using KeyValT = std::remove_cv_t<std::conditional_t<std::is_enum_v<KeyT>, std::underlying_type_t<KeyT>, KeyT>>;
        using StorageT = std::array<ValueT, Size>;
        static_assert(std::is_integral_v<KeyValT>, "Type must be integral (suitable for indexing array)");
        StorageT data{};
        /* Offset type */
        /* Used to support ranges which don't start from 0 (e.g.: [-1, 0, 1] or [5, 6, 7] etc.) */
        KeyValT offset_from_zero;

      public:
        /* Member types */
        using key_type = KeyT;
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

        consteval linear_map(std::initializer_list<std::pair<KeyT, ValueT>> kvp_items) : offset_from_zero(check_prereqs(kvp_items)) {
            for (const auto &kvp : kvp_items) {
                data[static_cast<KeyValT>(kvp.first) - offset_from_zero] = kvp.second;
            }
        }

        consteval linear_map(std::initializer_list<KeyT> key_items) : offset_from_zero(check_prereqs(key_items)) {}

        /* Lookup */

        constexpr iterator find(const KeyT &key) noexcept {
            return data.begin() + find_impl(key);
        }

        constexpr const_iterator find(const KeyT &key) const noexcept {
            return data.begin() + find_impl(key);
        }

        constexpr size_type count(const KeyT& key) const noexcept {
            return contains() ? 1 : 0;
        }

        constexpr bool contains(const KeyT& key) const noexcept {
            return find_impl(key) != end();
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
        /* Pre-requisites:
         * 1) Is being evaluated in constexpr (X)
         * 2) Input type is, either pair<key, X> (consteval) or key (constinit)
         * 3) Input size is equal to container size
         * 4) Input values are linearly increasing (can be unsorted) (i.e. 1, 2, 3, ..., N)
         */
        static constexpr auto check_prereqs(const auto &container) {
            constexpr_assert(container.size() == Size, "Invalid size");
            std::array<KeyValT, Size> prereq_arr{};
            constexpr auto extract_value = [](auto val) constexpr -> KeyValT {
                // static_assert(std::is_same_v<decltype(val.first), std::false_type>);
                if constexpr (requires {val.first;}) {
                    static_assert(std::is_same_v<decltype(val.first), KeyT>);
                    return static_cast<KeyValT>(val.first);
                } else {
                    static_assert(std::is_same_v<decltype(*val), KeyT>);
                    return static_cast<KeyValT>(*val);
                }
            };
            
            std::transform(container.begin(), container.end(), prereq_arr.begin(), extract_value);
            std::sort(prereq_arr.begin(), prereq_arr.end());

            auto adjacent_val = std::adjacent_find(prereq_arr.cbegin(), prereq_arr.cend());
            constexpr_assert(adjacent_val == prereq_arr.cend(), "Duplicate entries in keys");
            KeyValT old_val = prereq_arr.front();

            return prereq_arr.front();
        }

        constexpr size_t find_impl(const KeyT &key) const noexcept {
            if (static_cast<KeyValT>(key) < offset_from_zero || static_cast<KeyValT>(key) >= Size + offset_from_zero) {
                return size();
            }

            return (static_cast<KeyValT>(key) - offset_from_zero);
        }
    };
};

enum class TestEnum : int32_t {
    A = -1, B = 1, C = 0
};

static constexpr auto lin_map = heurohash::linear_map<TestEnum, int, 3>{{TestEnum::A, 1}, {TestEnum::B, 2}, {TestEnum::C, 3}};
