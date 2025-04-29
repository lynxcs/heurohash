#pragma once

/*
 * Great algorithm with super low memory usage (slightly worse than ordered map)
 * Talk explaining it:
 *      https://www.youtube.com/watch?v=DLgM570cujU (start ~53 min mark)
 * Corresponding original source:
 *      https://github.com/intel/compile-time-init-build/blob/main/include/lookup/pseudo_pext_lookup.hpp
 *
 * The original source has dependency on boost,
 * as well as it only works with a kvp stored approach
 * so it was repurposed to be able to do split keyset/valueset
 * approach.
 * Also supports the convenient span-like interface
 */

#include <algorithm>
#include <array>
#include <bit>
#include <bitset>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <limits>
#include <span>
#include <tuple>
#include <type_traits>
#include <utility>

#include "comp_time_arg.hpp"

namespace lookup {

/* FIXME: Remove usage of this type */
// template <typename K, typename V> struct entry {
//     using key_type = K;
//     using value_type = V;
//     key_type key_{};
//     value_type value_{};
// };
// template <typename K, typename V> entry(K, V) -> entry<K, V>;

namespace detail {
constexpr auto as_raw_integral(auto v) {
    static_assert(sizeof(v) <= 8);
    static_assert(!std::is_pointer_v<decltype(v)>);

    if constexpr (sizeof(v) == 1) {
        return std::bit_cast<std::uint8_t>(v);
    } else if constexpr (sizeof(v) == 2) {
        return std::bit_cast<std::uint16_t>(v);
    } else if constexpr (sizeof(v) <= 4) {
        return std::bit_cast<std::uint32_t>(v);
    } else if constexpr (sizeof(v) <= 8) {
        return std::bit_cast<std::uint64_t>(v);
    }
}

template <typename T>
using raw_integral_t = decltype(as_raw_integral(std::declval<T>()));

template <uint64_t BiggestValue> auto uint_for_f() {
    if constexpr (BiggestValue <= std::numeric_limits<uint8_t>::max()) {
        return uint8_t{};

    } else if constexpr (BiggestValue <= std::numeric_limits<uint16_t>::max()) {
        return uint16_t{};

    } else if constexpr (BiggestValue <= std::numeric_limits<uint32_t>::max()) {
        return uint32_t{};

    } else {
        return uint64_t{};
    }
}

template <uint64_t BiggestValue>
using uint_for_ = decltype(uint_for_f<BiggestValue>());

/* FIXME: All this bitset stuff is constexpr only from C++23 */
/* Ideally we would have C++20 support by replicating manually */
template <typename T> constexpr auto bitset_to_type(const auto &bitset) {
    return static_cast<T>(bitset.to_ullong());
}

/// log n
template <typename T>
constexpr auto compute_pack_coefficient(std::size_t dst, T const mask) -> T {
    constexpr auto t_digits = std::numeric_limits<T>::digits;

    auto pack_coefficient = std::bitset<t_digits>{};
    auto const mask_bits = std::bitset<t_digits>{mask};

    bool prev_src_bit_set = false;
    for (auto src = std::size_t{}; src < t_digits; src++) {
        bool const curr_src_bit_set = mask_bits[src];
        bool const new_stretch = curr_src_bit_set and not prev_src_bit_set;

        if (new_stretch) {
            pack_coefficient.set(dst - src);
        }

        if (curr_src_bit_set) {
            dst += 1;
        }

        prev_src_bit_set = curr_src_bit_set;
    }

    // FIXME: Check for equivalence
    return bitset_to_type<T>(pack_coefficient);
    // return pack_coefficient.template to<T>();
}

template <typename T> struct mask_bits_t {
    static_assert(std::is_unsigned_v<T>,
                  "bit_mask must be used with unsigned types");

    constexpr auto operator()(std::size_t bit) const -> T {
        if (bit == std::numeric_limits<T>::digits) {
            return std::numeric_limits<T>::max();
        }
        return static_cast<T>(T{1} << bit) - T{1};
    }
};

template <typename T> struct bitmask_subtract {
    static_assert(std::is_unsigned_v<T>,
                  "bit_mask must be used with unsigned types");
    constexpr auto operator()(T x, T y) const -> T { return x ^ y; }
};

template <typename T>
[[nodiscard]] constexpr auto bit_mask(std::size_t Msb,
                                      std::size_t Lsb = 0) noexcept -> T {
    return detail::bitmask_subtract<T>{}(detail::mask_bits_t<T>{}(Msb + 1),
                                         detail::mask_bits_t<T>{}(Lsb));
}

template <typename T> struct pseudo_pext_t {
    T mask;
    T coefficient;
    T final_mask;
    // std::size_t gap_bits;
    raw_integral_t<T> gap_bits;

    constexpr explicit pseudo_pext_t(T mask_arg) : mask{mask_arg} {
        constexpr auto t_digits = std::numeric_limits<T>::digits;
        auto const num_bits_to_extract = std::popcount(mask);
        auto const left_padding = std::countl_zero(mask);
        gap_bits = static_cast<std::size_t>(t_digits - num_bits_to_extract -
                                            left_padding);
        coefficient = compute_pack_coefficient<T>(gap_bits, mask);
        auto const final_mask_msb =
            static_cast<std::size_t>(num_bits_to_extract - 1);

        final_mask = bit_mask<T>(final_mask_msb);
    }

    [[nodiscard]] constexpr __attribute__((always_inline)) auto
    operator()(T value) const -> T {
        auto const packed = (value & mask) * coefficient;
        return static_cast<T>(packed >> gap_bits) & final_mask;
    }
};

/// count the number of key duplicates (n log n)
template <typename T, std::size_t S>
constexpr auto count_duplicates(std::array<T, S> keys) -> std::size_t {
    std::sort(std::begin(keys), std::end(keys));
    auto dups = std::size_t{};
    for (auto i = std::adjacent_find(std::cbegin(keys), std::cend(keys));
         i != std::cend(keys); i = std::adjacent_find(++i, std::cend(keys))) {
        ++dups;
    }
    return dups;
}

/// count the length of the longest run of identical values (n)
template <typename T, std::size_t S>
constexpr auto count_longest_run(std::array<T, S> keys) -> std::size_t {
    std::sort(keys.begin(), keys.end());

    auto longest_run = std::size_t{};
    auto current_run = std::size_t{};

    if (S > 0) {
        T prev_value = keys[0];

        for (auto i = std::size_t{1}; i < S; i++) {
            T const curr_value = keys[i];

            if (curr_value == prev_value) {
                current_run++;
            }

            longest_run = std::max(longest_run, current_run);

            if (curr_value != prev_value) {
                current_run = 0;
            }

            prev_value = curr_value;
        }
    }

    return longest_run;
}

template <typename T, std::size_t S>
constexpr auto keys_are_unique(std::array<T, S> const &keys) -> bool {
    return count_duplicates(keys) == 0;
}

template <typename T, std::size_t S>
constexpr auto with_mask(T const mask, std::array<T, S> const &keys)
    -> std::array<T, S> {
    std::array<T, S> new_keys{};

    std::transform(keys.begin(), keys.end(), new_keys.begin(),
                   [&](T k) { return pseudo_pext_t(mask)(k); });

    return new_keys;
}

template <typename T, typename V, std::size_t S>
constexpr bool is_arr_kvp(std::array<std::pair<T, V>, S> const &entries) {
    return true;
}

template <typename T, std::size_t S>
constexpr auto is_arr_kvp(std::array<T, S> const &entries) {
    return false;
}

template <typename T, typename V, std::size_t S>
constexpr auto get_values(std::array<std::pair<T, V>, S> const &entries) {
    std::array<V, S> new_keys{};

    std::transform(entries.begin(), entries.end(), new_keys.begin(),
                   [&](std::pair<T, V> e) { return e.second; });

    return new_keys;
}

template <typename T, typename V, std::size_t S>
constexpr auto get_orig_keys(std::array<std::pair<T, V>, S> const &entries) {
    std::array<T, S> new_keys{};

    std::transform(entries.begin(), entries.end(), new_keys.begin(),
                   [&](std::pair<T, V> e) { return e.first; });

    return new_keys;
}

template <typename T, std::size_t S>
constexpr auto get_orig_keys(std::array<T, S> const &entries) {
    return std::array<T, S>{entries};
}

template <typename T, typename V, std::size_t S>
constexpr auto get_raw_keys(std::array<std::pair<T, V>, S> const &entries) {
    using raw_t = detail::raw_integral_t<T>;
    std::array<raw_t, S> new_keys{};

    std::transform(
        entries.begin(), entries.end(), new_keys.begin(),
        [&](std::pair<T, V> e) { return detail::as_raw_integral(e.first); });

    return new_keys;
}

template <typename T, std::size_t S>
constexpr auto get_raw_keys(std::array<T, S> const &entries) {
    using raw_t = detail::raw_integral_t<T>;
    std::array<raw_t, S> new_keys{};

    std::transform(entries.begin(), entries.end(), new_keys.begin(),
                   [&](auto e) { return detail::as_raw_integral(e); });

    return new_keys;
}

template <typename T, std::size_t S>
constexpr auto remove_cheapest_bit(detail::raw_integral_t<T> mask,
                                   std::array<T, S> keys)
    -> detail::raw_integral_t<T> {
    using raw_t = detail::raw_integral_t<T>;

    auto const t_digits = std::numeric_limits<raw_t>::digits;
    auto bmask = std::bitset<t_digits>{mask};

    auto cheapest_bit = std::size_t{};
    auto min_num_dups = std::numeric_limits<std::size_t>::max();

    for (size_t idx = 0; idx < t_digits; ++idx) {
        if (!bmask.test(idx)) {
            continue;
        }
        auto btry_mask = bmask;
        btry_mask.reset(idx);

        std::array<raw_t, S> try_keys =
            with_mask(bitset_to_type<raw_t>(btry_mask), keys);

        // return bitset_to_type<raw_t>(btry_mask);

        auto num_dups = count_duplicates(try_keys);
        if (num_dups < min_num_dups) {
            min_num_dups = num_dups;
            cheapest_bit = idx;
        }
    }
    // for_each(
    //     [&](auto i) {
    //     },
    //     bmask);

    bmask.reset(cheapest_bit);
    return bitset_to_type<raw_t>(bmask);
    // return bmask.template to<raw_t>();
}

template <typename T, std::size_t S>
constexpr auto calc_pseudo_pext_mask(std::array<T, S> const &input,
                                     std::size_t max_search_len) {
    auto keys = get_raw_keys(input);
    using raw_t = decltype(keys)::value_type;
    auto const t_digits = std::numeric_limits<raw_t>::digits;

    // try removing each bit from the mask one at a time.
    // then apply the pseudo_pext function to all the keys with the mask. if
    // the keys are all still unique, then we can remove the bit and move on
    // to the next one.
    raw_t mask = std::numeric_limits<raw_t>::max();
    for (auto x = std::size_t{}; x < t_digits; x++) {
        auto i = t_digits - 1 - x;
        raw_t const try_mask = mask & ~static_cast<raw_t>(raw_t{1} << i);
        std::array<raw_t, S> const try_keys = with_mask(try_mask, keys);
        if (keys_are_unique(try_keys)) {
            mask = try_mask;
        }
    }

    // we can remove more bits from the mask to achieve a smaller memory
    // footprint with a small runtime cost. each additional bit removed
    // from the mask cuts intermediate table size in half, but risks more
    // collisions. try to remove the most number of bits from the mask while
    // staying under the max search length.
    auto prev_longest_run = std::size_t{};
    while (max_search_len > 1 && std::popcount(mask) > 4) {
        auto try_mask = remove_cheapest_bit(mask, keys);
        auto current_longest_run = count_longest_run(with_mask(try_mask, keys));
        if (current_longest_run <= max_search_len) {
            mask = try_mask;
            prev_longest_run = current_longest_run;
        } else {
            return std::make_tuple(mask, prev_longest_run);
        }
    }

    return std::make_tuple(mask, prev_longest_run);
}

} // namespace detail

template <size_t len> struct empty_dyn_search {
    static constexpr size_t value = len;

    constexpr auto get() const noexcept { return value; }
};

struct dyn_search {
    size_t value;
    constexpr auto get() const noexcept { return value; }
};

template <typename StorageT, typename LookupTableT, size_t SearchLen = 0>
struct pseudo_next_indirect {
    using key_type = StorageT::value_type;
    using storage_t = std::remove_cv_t<StorageT>;
    using raw_key_type = detail::raw_integral_t<key_type>;

    /* In the span-esque case, we don't know the exact search len */
    using dyn_search_t = std::conditional_t<SearchLen == 0, dyn_search,
                                            empty_dyn_search<SearchLen>>;

    using PextFunc = detail::pseudo_pext_t<raw_key_type>;

    static constexpr auto search_len_v = SearchLen;

    /* AoT-known case */
    constexpr pseudo_next_indirect(StorageT storage, LookupTableT lookup_table,
                                   PextFunc func) noexcept
        requires(SearchLen != 0)
        : key_storage(storage), lookup_table(lookup_table), pext_func(func),
          search_len{} {}

    /* Dynamic case */
    constexpr pseudo_next_indirect(StorageT storage, LookupTableT lookup_table,
                                   PextFunc func, size_t len) noexcept
        requires(SearchLen == 0)
        : key_storage(storage), lookup_table(lookup_table), pext_func(func),
          search_len{len} {}

    constexpr pseudo_next_indirect(const pseudo_next_indirect &copy) noexcept =
        default;
    constexpr pseudo_next_indirect &
    operator=(const pseudo_next_indirect &copy) noexcept = default;

    storage_t key_storage;
    LookupTableT lookup_table;
    PextFunc pext_func;
    [[no_unique_address]] dyn_search_t search_len;

    [[nodiscard]] constexpr __attribute__((always_inline)) size_t
    lookup(key_type key) const noexcept {
        auto const raw_key = detail::as_raw_integral(key);
        auto i = lookup_table[pext_func(raw_key)];
        if constexpr (SearchLen != 0) {
            for (auto search_count = std::size_t{0}; search_count < SearchLen;
                 ++search_count) {
                if (raw_key == detail::as_raw_integral(key_storage[i])) {
                    return i;
                }

                ++i;
            }
        } else {
            auto const max_len = search_len.get();
            for (auto search_count = std::size_t{0}; search_count < max_len;
                 ++search_count) {
                if (raw_key == detail::as_raw_integral(key_storage[i])) {
                    return i;
                }
                ++i;
            }
        }

        return key_storage.size();
    }

    constexpr size_t find(key_type key) const noexcept { return lookup(key); }

    constexpr size_t size() const noexcept { return key_storage.size(); }
    constexpr size_t lut_size() const noexcept { return lookup_table.size(); }
    constexpr size_t depth() const noexcept { return search_len.get(); }

    constexpr const key_type *begin() const noexcept {
        return key_storage.data();
    }

    /* Allow conversion to dyn size if AoT type */
    [[nodiscard]] constexpr pseudo_next_indirect<StorageT, LookupTableT, 0>
    to_dyn() const noexcept
        requires(SearchLen != 0)
    {
        return pseudo_next_indirect{
            std::span{key_storage.data(), key_storage.size()},
            std::span{lookup_table.data(), key_storage.size()}, pext_func,
            search_len.get()};
    }

    [[nodiscard]] constexpr auto to_dyn_lut() const noexcept
        requires(SearchLen != 0)
    {
        auto lookup_table_span =
            std::span{lookup_table.data(), lookup_table.size()};
        return pseudo_next_indirect<decltype(key_storage),
                                    decltype(lookup_table_span), 0>{
            key_storage, lookup_table_span, pext_func, search_len.get()};
    }
};

template <typename KeyT>
using dyn_pseudo_next_indirect =
    pseudo_next_indirect<std::span<KeyT>, std::span<KeyT>, 0>;

template <size_t MaxSize> using lookup_idx_exp_t = detail::uint_for_<MaxSize>;

template <std::size_t MaxSearchLen = 4> struct pseudo_pext_lookup {
  private:
    static_assert(MaxSearchLen >= 1);

  public:
    [[nodiscard]] constexpr static auto make(comp_time auto comp_time_builder) {
        constexpr auto input = comp_time_builder();
        // using key_type = typename decltype(input)::value_type::key_type;
        // using raw_key_type = detail::raw_integral_t<key_type>;
        // using value_type = typename decltype(input)::value_type::value_type;

        constexpr auto raw_keys = detail::get_raw_keys(input);
        static_assert(detail::keys_are_unique(raw_keys),
                      "Lookup keys must be unique.");

        constexpr auto mask_and_search =
            detail::calc_pseudo_pext_mask(input, MaxSearchLen);

        constexpr auto mask = std::get<0>(mask_and_search);
        constexpr auto search_len = std::get<1>(mask_and_search) + 1;

        constexpr auto p = detail::pseudo_pext_t(mask);
        constexpr auto lookup_table_size = 1 << std::popcount(mask);

        constexpr auto storage = [&]() {
            auto s = detail::get_orig_keys(input);

            // sort by the hashed key to group all the buckets together
            std::sort(s.begin(), s.end(), [&](auto left, auto right) {
                return p(detail::as_raw_integral(left)) <
                       p(detail::as_raw_integral(right));
            });

            // find end of the longest bucket
            auto const end_of_longest_bucket = [&]() {
                auto e = s.begin();

                auto curr_bucket_length = 1U;
                auto prev_idx = p(detail::as_raw_integral(*e));
                ++e;
                while (e != s.end()) {
                    auto const curr_idx = p(detail::as_raw_integral(*e));

                    if (curr_idx == prev_idx) {
                        curr_bucket_length++;

                    } else if (curr_bucket_length >= search_len) {
                        return e;

                    } else {
                        curr_bucket_length = 1;
                    }

                    prev_idx = curr_idx;
                    ++e;
                }

                return e;
            }();

            // place the longest bucket at the end
            std::rotate(s.begin(), end_of_longest_bucket, s.end());

            return s;
        }();

        constexpr auto lookup_table = [&]() {
            using lookup_idx_t = detail::uint_for_<storage.size()>;
            std::array<lookup_idx_t, lookup_table_size> t{};

            t.fill(0);

            // iterate backwards so the index of the first entry of a bucket
            // remains in the lookup table
            for (auto entry_idx = static_cast<lookup_idx_t>(storage.size() - 1);
                 entry_idx < storage.size(); entry_idx--) {
                auto const e = storage[entry_idx];
                auto const raw_key = detail::as_raw_integral(e);
                auto const lookup_table_idx = p(raw_key);
                t[lookup_table_idx] = entry_idx;
            }

            return t;
        }();

        return pseudo_next_indirect<std::remove_cv_t<decltype(storage)>,
                                    std::remove_cv_t<decltype(lookup_table)>,
                                    search_len>{storage, lookup_table, p};
    }
};
} // namespace lookup

static constexpr auto table = lookup::pseudo_pext_lookup<4>::make([]() {
    return std::array{
        std::make_pair(static_cast<uint16_t>(1), static_cast<uint16_t>(1))};
});

static_assert(sizeof(table) == 12);
static_assert(table.lookup(1) == 0);
static_assert(table.lookup(2) == 1);
static_assert(table.lookup(929) == 1);

static constexpr auto table2 = lookup::pseudo_pext_lookup<16>::make([]() {
    return std::array{
        std::make_pair(static_cast<uint32_t>(1), static_cast<uint16_t>(1)),
        std::make_pair(static_cast<uint32_t>(2), static_cast<uint16_t>(1)),
    };
});

static_assert(sizeof(table2) == 28);

static constexpr auto table3 = lookup::pseudo_pext_lookup<16>::make([]() {
    return std::array{static_cast<uint32_t>(1), static_cast<uint32_t>(2)};
});

static_assert(sizeof(table3) == 28);

// static_assert(table[2] == 0);
