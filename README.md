# Heurohash - Various C++20 constexpr/constinit maps

## Features
* Map keys _must_ be known at compile-time
* Map values can be compile-time, or run-time values (so either constexpr or constinit)
* Validates that keys are in fact unique (fails to compile if they aren't)
* Very similiar interface to that of std map's

## Types of supported maps
* Linear - Maps KVP of a linear range [0, 1, ..., n]. Minimal size (keys are discarded). Range can begin from any number. Key must be integral (can be enum).
* Ordered - Maps KVP into a sorted array. Supports passing custom Comparison operation (default std::less). Stored key type can be overwritten, if the keys don't use the whole spectrum of key type. Supports 'split' approach, where keys & actual map are stored separately (i.e. keys in RO data, values in RW data)
* Unordered - Maps KVP into a perfect-minimal-hash array. Still a W.I.P

## Span
In most cases, we want to provide 'clean' interfaces, where the caller doesn't need to know the size of the map (_ala span_, or as with any of the regular std map's). This library also supports those:
* linear_map_span - span over an linear map
* ordered_map_span - span over an ordered map (Still a W.I.P)
* unordered_map_span - span over an unordered map (Still a W.I.P)

They support the same begin()/end()/find()/etc. operations.

## Example usage

### Common api
#### Linear map
```cpp
#include <heurohash/linear_map.hpp>

enum class KeyType {
    /* Values can start not only at 0, as long as they are linear */
    A = -1, B = 0, C = 1, D = 2
};

/* Compile time map */
static constexpr auto example_map = heurohash::linear_map<KeyType, int, 3>{{{KeyType::A, 10},{KeyType::B, 20},{KeyType::C, 30}}};

static_assert(*example_map.find(KeyType::A) == 10);
static_assert(example_map[KeyType::B] == 20);
static_assert(example_map.find(KeyType::D) == example_map.end());

/* Initialize at compile time, but allow changing values */
static constinit auto example_map_runtime_static = heurohash::linear_map<KeyType, int, 3>{{<same as before>}};

/* Also supports initing at run-time. Even without constinit, guarantees that expensive constructor checks are not ran (due to constructor being consteval) */
auto example_map_runtime_nonstatic = heurohash::linear_map<KeyType, int, 3>{{<same as before>}};

/* For all variants can also specify only keys (doesn't make much sense as static constexpr though) */
auto example_map_runtime_no_vals = heurohash::linear_map<KeyType, int, 3>{{KeyType::A, KeyType::B, KeyType::C}};

/* Span interface (implicit conversion) */
heurohash::linear_map_span<KeyType, int> example_map_span = example_map_runtime_nonstatic;

/* Also supports the constexpr variants (value needs to be const though) */
heurohash::linear_map_span<KeyType, const int> example_map_span_const = example_map;
```
#### Ordered map
Ordered map supports very similiar API as linear_map, with it's own variant of span (ordered_map_span).

The one notable difference is that ordered_map has two extra template arguments:
- KeyStorageT - Type in internal storage. This is in most cases identical to KeyT, however, in case of enum, it's overwritten with the underlying type. This is to reduce template bloat, esp. considering most
enums will default to int.
- Compare - Comparison type. Defaults to std::less{} (same as std::map), however if needed, can pass other types. Note that compare has to match not Key type, but KeyStorageT (so take care when passing custom compare with enums)
```cpp
enum class KeyType {
    /* Values don't have to be linear */
    A = 50, B = 20, C = -10, D = 3
};

/* Compile time map */
static constexpr auto example_map = heurohash::ordered_map<KeyType, int, 3>{{{KeyType::A, 10},{KeyType::B, 20},{KeyType::C, 30}}};

static_assert(*example_map.find(KeyType::A) == 10);
static_assert(example_map[KeyType::B] == 20);
static_assert(example_map.find(KeyType::D) == example_map.end());

/* Initialize at compile time, but allow changing values */
static constinit auto example_map_runtime_static = heurohash::ordered_map<KeyType, int, 3>{{{KeyType::A, 10},{KeyType::B, 20},{KeyType::C, 30}}};

/* Also supports initing at run-time. Even without constinit, guarantees that expensive constructor checks are not ran (due to constructor being consteval) */
auto example_map_runtime_nonstatic = heurohash::ordered_map<KeyType, int, 3>{{{KeyType::A, 10},{KeyType::B, 20},{KeyType::C, 30}}};

/* For all variants can also specify only keys (doesn't make much sense as static constexpr though) */
auto example_map_runtime_no_vals = heurohash::ordered_map<KeyType, int, 3>{{KeyType::A, KeyType::B, KeyType::C}};
```
#### Minimal perfect hash map (W.I.P)
### Key/Value split API (W.I.P)
Alongside the standard API, this library also provides a keyset/valueset split for ordered map (and in future - MPH map).

The advantage of the split API, is that it allows to totally minimize the usage of ROM & RAM (useful for embedded) in cases where we want the values to be modifiable (constinit/regular construction).

This can be achieved by having the keyset be stored in ROM via static constexpr, and the valueset in RAM via constinit (or just as local variable). The valueset contains a reference to the keyset.

Note that keyset/valueset pairs are also compatible with their respective _span variants.

## Benchmarks
TODO: Compare with std::array (hand-made table), std::map, std::unordered_map
