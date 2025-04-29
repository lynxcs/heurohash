#pragma once

#include <concepts>

template <typename T>
concept comp_time = requires {
    []() constexpr {
        T t{};
        if constexpr (requires { t(); }) {
            t();
        } else if constexpr (requires { T::operator()(); }) {
            T::operator()();
        }
    }();
};

template <typename T, typename Ret>
concept comp_time_with_arg = requires {
    []() constexpr -> Ret {
        T t{};
        if constexpr (requires {
                          { t() } -> std::convertible_to<Ret>;
                      }) {
            return t();
        } else if constexpr (requires {
                                 {
                                     T::operator()()
                                 } -> std::convertible_to<Ret>;
                             }) {
            return T::operator()();
        } else {
            static_assert(false, "Type does not have a callable operator() "
                                 "returning the required type.");
        }
    }();
};
