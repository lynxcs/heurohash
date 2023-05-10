# Heurohash - constexpr hash map with heuristics

## Types of heuristics
* Direct - fastest, all inputs must be from 0-X (just static array)
* Linear - 2nd fastest, inputs can be from x-y, with condition that x-y is linear
* Map - slowest, quadratic open addressing hash map

## Ways of using
### Enforcing specific type
You can pick specific type of heuristic hash, and if the input doesn't match, then a compile time error will be thrown
### Auto-pick
Compiler will pick best type, according to the input data

## Features / Caveats
* If values are forced const, then should be stored in flash (i.e. constant amount of flash taken, no dynamic allocation)
* If values are non-const (i.e. can be modified), then it's static (i.e. constant amount of RAM taken, no dynamic allocation)
* Values can be non-constexpr / can be modified
* Map can be used as a non-constexpr (but has to be marked as such). This approach (quadratic open addressing) should still be more efficient than std::unordered_map
* Can select wanted load-factor
* Can provide custom allocator/deallocator function
* Can disable bounds checking for extra performance (not recommended, unless you are 1000% sure, or are checking elsewhere)

## Advanced usage
### Non-linear linear
If your input has known hash that gives a **guaranteed** unique index slot, such that the result for all input is linear from x-y,
you can specific Linear with custom hash function.
If your input + hash function violates this assumption, then compile time error will be thrown