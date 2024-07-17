cmake_minimum_required(VERSION 3.18)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(heurohash)

option(HEUROHASH_ENABLE_TESTING "Enable tests" OFF)
option(HEUROHASH_ENABLE_BENCHES "Enable benchmarks" OFF)

add_library(heurohash INTERFACE)

target_include_directories(heurohash INTERFACE include/)
target_compile_features(heurohash INTERFACE cxx_std_20)

if (HEUROHASH_ENABLE_BENCHES)
    add_subdirectory(benchmarks)
endif()
