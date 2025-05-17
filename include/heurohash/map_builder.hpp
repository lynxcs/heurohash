#pragma once

#include "ordered_map.hpp"
#include "pmh_map.hpp"

namespace heurohash {
static constexpr auto gen_mixed_map(comp_time auto builder) {
    constexpr auto data = builder();
    if constexpr (std::size(data) <= 16) {
        return heurohash::make_hash_map(builder);
    } else {
        return heurohash::make_ordered_map(data);
    }
}
} // namespace heurohash
