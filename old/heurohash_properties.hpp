#include <initializer_list>
#include <tuple>
#include <type_traits>

template <typename T>
using TrueT = std::conditional_t<std::is_enum_v<T>, std::underlying_type_t<T>, T>;

template <typename T>
using TrueUnsign = std::is_unsigned<TrueT<T>>;

template <typename T>
using TrueSign = std::is_signed<TrueT<T>>;

enum class DataProperties {
    NoGaps, // Data has no gaps (i.e. 1,2,3 not 1,3,4)
    StartsFromZero, // Data starts from zero
    NonNegative, // Data doesn't contain negative numbers
    Large, // Indices must be uint64_t
};

/* This information is only relevant for 'linear' types (i.e. direct or linear) */
template<typename CountT, typename IdxT>
struct LinearDataPropertyStorage {
    CountT count;
    IdxT min;
    IdxT max;
};

template<typename KeyT, typename ValueT>
static constexpr bool keys_non_neg(const std::initializer_list<std::pair<KeyT,ValueT>> &values) {
    /* If type is unsigned - then guaranteed to be non-neg */
    if constexpr (TrueUnsign<KeyT>::value == true) {
        return true;
    } else {
        /* Actual implementation ig */
        for (const auto &val : values) {
            if (val < 0) {
                return false;
            }
        }

        return true;
    }
}

template<typename KeyT, typename ValueT>
static constexpr bool keys_no_gaps(const std::initializer_list<std::pair<KeyT,ValueT>> &values) {
    for (const auto& val : values) {
        if (val < 0) {
            return true;
        }
    }

    return false;
}

template<typename KeyT, typename ValueT>
static constexpr bool keys_starts_from_zero(const std::initializer_list<std::pair<KeyT,ValueT>> &values) {
    if constexpr (TrueUnsign<KeyT>::value == true) {
        for (const auto& val : values) {
            if (val == 0) {
                return true;
            }
        }
    } else {
        bool found_zero = false;
        for (const auto& val : values) {
            if (val < 0) {
                return false;
            }
            if (val == 0) {
                found_zero = true;
            }
            return found_zero;
        }
    }
    return false;
}

template<typename KeyT, typename ValueT>
static constexpr bool keys_require_large(const std::initializer_list<std::pair<KeyT,ValueT>> &values) {
    /* If type is unsigned - then guaranteed to be non-neg */
    if constexpr (std::is_enum_v<KeyT>) {
      /* Actual implementation ig */
        for (const auto &val : values) {
            if (val < 0) {
                return false;
            }
        }

        return true;
    } else {
        return true;
    }
}

/* TODO: Ensure that CountT is number type, EdgeT - must have 'comparison' operator & 'distance' operator */
template <typename CountT, typename EdgeT>
struct DataPropertyStorage {
    CountT count;
    EdgeT min;
    EdgeT max;
};

// struct DataPropertyStorageNonNeg {
//     uint32_t count;
//     uint32_t min;
//     uint32_t max;
// };

// struct DataPropertyStorageNonNegLarge {
//     uint64_t count;
//     uint64_t min;
//     uint64_t max;
// };

// struct DataPropertyStorageNeg {
//     uint32_t count;
//     int32_t min;
//     int32_t max;
// };

// struct DataPropertyStorageNegLarge {
//     uint64_t count;
//     int64_t min;
//     int64_t max;
// };

enum class OptimizationSettings {
    // Map specific-optimizations
    MemoryMappedOverflow, // use mmap instead of checking when idx overflows
    PowerOfTwoSizes, // Use sizes that are power of two
    TryFitIntoSmallestPossible, // Load factor 1 (faster iteration, smallest space waste, slower when finding keys)
    // ^ Default load factor is 0.9
};
