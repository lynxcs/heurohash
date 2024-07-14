cmake_minimum_required(VERSION 3.18)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(heurohash)

add_library(heurohash INTERFACE)

target_sources(heurohash INTERFACE
    heurohash_direct.hpp
    heurohash_hashfunc.hpp
    heurohash_linear.hpp
    heurohash_map.hpp
    heurohash_properties.hpp
)

add_executable(test_client main.cpp)
target_link_libraries(test_client PRIVATE heurohash)

target_compile_features(heurohash INTERFACE cxx_std_17)
set_property(TARGET test_client PROPERTY CXX_STANDARD 17)