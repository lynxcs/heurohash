#include <tuple>
#include <initializer_list>
#include <stdio.h>

#include <type_traits>
#include <limits>
#include <array>
#include <cstdint>

template <typename T>
using TrueT = std::conditional_t<std::is_enum_v<T>, std::underlying_type_t<T>, T>;

template <typename T>
using TrueUnsign = std::is_unsigned<TrueT<T>>;

template <typename T>
using TrueSign = std::is_signed<TrueT<T>>;

template <typename Key, typename Value, size_t N>
using KvpInit = std::pair<Key,Value>[N];

template<typename KeyT, typename ValueT, size_t N>
static constexpr inline bool keys_no_gaps(const KvpInit<KeyT,ValueT, N> &values) {
    using TT = TrueT<KeyT>;
    TT min = std::numeric_limits<TT>::max();
    TT max = std::numeric_limits<TT>::min();
    for (const auto& val : values) {
        if (static_cast<TT>(val.first) < min) {
            min = (TrueT<KeyT>)val.first;
        }
        if (static_cast<TT>(val.first) > max) {
            max = (TrueT<KeyT>)val.first;
        }
    }

    return static_cast<size_t>(max - min) == N - 1;
}

template<typename KeyT, typename ValueT, size_t N>
static constexpr inline bool keys_starts_from_zero(const KvpInit<KeyT,ValueT, N> &values) {
    using TT = TrueT<KeyT>;
    if constexpr (TrueUnsign<KeyT>::value) {
        for (const auto& val : values) {
            if (static_cast<TT>(val.first) == 0) {
                return true;
            }
        }
    } else {
        bool found_zero = false;
        for (const auto& val : values) {
            if (val.first < 0) {
                return false;
            }
            if (val.first == 0) {
                found_zero = true;
            }
            return found_zero;
        }
    }
    return false;
}


template <typename Key, typename Value, size_t N>
constexpr auto generate_direct_heuro_(const KvpInit<Key,Value, N>& init);

template <typename Key, typename Value, typename Size, Size lst_size>
struct HeuroDirect {
    public:
        [[nodiscard]] constexpr inline auto begin() const noexcept {
            return direct_data.begin();
        }

        [[nodiscard]] constexpr inline auto end() const noexcept {
            return direct_data.end();
        }

        [[nodiscard]] constexpr inline Value* data() const noexcept {
            return direct_data.data();
        }

        /* Taking by value is faster when it's a primitive type */
        /* I.e.: uint32_t. Since direct is meant to work with number keys */
        /* then it makes sense to use them here. */
        constexpr inline const Value& operator[](Key idx) const noexcept {
            return direct_data[idx];
        }
        
        constexpr inline Value& operator[](Key idx) noexcept {
            return direct_data[idx];
        }

        [[nodiscard]] constexpr inline bool contains(Key idx) const noexcept {
            return idx < lst_size;
        }

        [[nodiscard]] constexpr inline size_t size() const noexcept {
            return lst_size;
        }

    private:
        /* This function assumes output is already filtered! Allows us to mark as noexcept */
        constexpr HeuroDirect(const KvpInit<Key, Value, lst_size> &lst) noexcept : direct_data() {
            for (const auto &kvp : lst) {
                direct_data[kvp.first] = kvp.second;
            }
        }

      friend constexpr auto generate_direct_heuro_<Key, Value>(const KvpInit<Key,Value, lst_size>& init);
      /* When compiling w/ optimizations theres no difference between this & std::array */
      /* But on debug builds this results in less instructions*/
      std::array<Value, lst_size> direct_data;
};

template <typename Key, typename Value, size_t N>
constexpr auto generate_direct_heuro_(const KvpInit<Key, Value, N> &init) {
      if (!keys_starts_from_zero(init)) {
            throw "Keys must start from zero!";
      }

      if (!keys_no_gaps(init)) {
            throw "Keys must not have gaps!";
      }

      return HeuroDirect<Key, Value, std::size_t, N>(init);
}

template <typename Key, typename Value, size_t N>
static constexpr auto thing(const KvpInit<Key,Value, N> &init) {
    return generate_direct_heuro_<Key, Value, N>(init);
}

enum TestEnum {
    A,
    B,
    C
};

static const char *test_arr_thing[3] = {
    "A",
    "B",
    "C"
};

static constexpr auto test_arr = thing<TestEnum, const char*>({{TestEnum::A, "A"}, {TestEnum::B, "B"}, {TestEnum::C, "C"}});

template<typename KeyT, typename OffsetT, OffsetT Offset>
struct HeuroLinearOffset {
    static constexpr KeyT hash(KeyT key) noexcept {
        return key - Offset;
    }
};

template <typename KeyT, typename OffsetT, size_t N, OffsetT Offset>
struct HeuroLinearP2 {
    static constexpr KeyT hash(KeyT key) noexcept { return xs_ILogPow2(key); }

  private:
    // only works for powers of 2 inputs
    static inline uint32_t xs_ILogPow2(uint32_t v) {
            static constexpr std::array<uint8_t, 32> xs_KotayBits = {
                0,  1,  2, 16, 3,  6, 17, 21, 14, 4,  7,  9,  18, 11, 22, 26,
                31, 15, 5, 20, 13, 8, 10, 25, 30, 19, 12, 24, 29, 23, 28, 27};
            return xs_KotayBits[(uint32_t(v) * uint32_t(0x04ad19df)) >> 27];
    }
};

template<typename KeyT, typename Func, typename... Funcs>
struct HeuroLinearCombine {
    static constexpr KeyT hash(KeyT key) noexcept {
        return HeuroLinearCombine<KeyT, Funcs...>::hash(Func(key));
    }
};

template <typename Key, typename Value, size_t N>
constexpr auto generate_linear_heuro_(const KvpInit<Key, Value, N> &init);

template <typename KeyT, typename ValueT, typename Linear, size_t N>
struct HeuroLinear {
        [[nodiscard]] constexpr inline auto begin() const noexcept {
            return direct.begin();
        }

        [[nodiscard]] constexpr inline auto end() const noexcept {
            return direct.end();
        }

        [[nodiscard]] constexpr inline ValueT* data() const noexcept {
            return direct.data();
        }

        constexpr inline const ValueT& operator[](KeyT idx) const noexcept {
            return direct[Linear::hash(idx)];
        }
        
        constexpr inline ValueT& operator[](KeyT idx) noexcept {
            return direct[Linear::hash(idx)];
        }

        [[nodiscard]] constexpr inline bool contains(KeyT idx) const noexcept {
            return Linear::hash(idx) < N;
        }

        [[nodiscard]] constexpr inline size_t size() const noexcept {
            return N;
        }

    private:
    /*
      constexpr std::array<std::pair<KeyT, ValueT>, N>
      apply_func(const KvpInit<KeyT, ValueT, N> &lst) {
        std::array<std::pair<KeyT, ValueT>, N> vals;
        size_t i = 0;
        for (const auto& [key, val] : lst) {
            vals[i] = std::make_pair(func(key), val);
            ++i;
        }
        return vals;
      }
    */

      /* This function assumes output is already filtered! Allows us to mark as
       * noexcept */
      constexpr HeuroLinear(const KvpInit<KeyT, ValueT, N> &lst) noexcept : direct() {
            for (const auto &[key, val] : lst) {
                direct[Linear::hash(key)] = val;
            }
        }

        friend constexpr auto generate_linear_heuro_<KeyT, ValueT>(const KvpInit<KeyT,ValueT, N>& init);
        HeuroDirect<KeyT, ValueT, size_t, N> direct;
};

template <typename Key, typename Value, size_t N>
constexpr auto generate_linear_heuro_(const KvpInit<Key, Value, N> &init) {
      if (!keys_starts_from_zero(init)) {
            throw "Keys must start from zero!";
      }

      if (!keys_no_gaps(init)) {
            throw "Keys must not have gaps!";
      }

      return HeuroLinear<Key, Value, std::size_t, N>(init);
}

template <typename Key, typename Value, size_t N>
constexpr auto pick_heuro(const KvpInit<Key, Value, N> &init) {
    if (!keys_starts_from_zero(init)) {
        throw "Keys must start from zero!";
    }

    if (!keys_no_gaps(init)) {
        throw "Keys must not have gaps!";
    }

    return HeuroLinear<Key, Value, std::size_t, N>(init);
}

int main() {
    
    return 0;
}

void print_thing() {
    for (size_t i = 0; i < test_arr.size(); i++) {
        ::printf("%s\n", test_arr[(TestEnum)i]);
    }
}

void print_thing_primative() {
    for (size_t i = 0; i < (sizeof(test_arr_thing)/sizeof(test_arr_thing[0])); i++) {
        ::printf("%s\n", test_arr_thing[(TestEnum)i]);
    }
}