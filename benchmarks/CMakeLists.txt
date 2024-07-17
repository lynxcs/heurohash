# Disable the Google Benchmark requirement on Google Test
set(BENCHMARK_ENABLE_TESTING NO)
set(BENCHMARK_DOWNLOAD_DEPENDENCIES YES)

include(FetchContent)

FetchContent_Declare(
    googlebenchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG origin/main
)

FetchContent_MakeAvailable(googlebenchmark)

add_executable (benchmarks benchmarks.cpp)

# Link Google Benchmark to the project
target_link_libraries(benchmarks benchmark::benchmark heurohash)
