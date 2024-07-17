#pragma once

#include <type_traits>
#include <cassert>

#define constexpr_assert(cond, msg) assert(cond &&msg)

namespace heurohash::detail {
template <typename T>
using underlying_type = typename std::remove_cv_t<std::conditional_t<
    std::is_enum_v<T>, std::underlying_type<T>, std::type_identity<T>>>::type;
};