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

execute_process(
    COMMAND git cat-file -e master^{commit};
    COMMAND_ECHO STDOUT
    OUTPUT_QUIET
    ERROR_QUIET
    RESULT_VARIABLE out_var
)

if(out_var)
    message("Encountered error")
else()
    message("All good")
    execute_process(
        COMMAND git checkout --force master
        COMMAND_ECHO STDOUT
        OUTPUT_QUIET
        ERROR_QUIET
        RESULT_VARIABLE out_var
    )
endif()


add_executable(test_client main.cpp)
target_link_libraries(test_client PRIVATE heurohash)

target_compile_features(heurohash INTERFACE cxx_std_17)
set_property(TARGET test_client PROPERTY CXX_STANDARD 17)