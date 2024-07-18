#include <benchmark/benchmark.h>
#include <heurohash/ordered_map.hpp>
#include <heurohash/linear_map.hpp>

static constexpr auto arr = std::array<std::pair<int, int>, 8>{{{50, 20}, {10, 40}, {123, 435}, {53498, 423}, {1230, 1234}, {34598, 12390}, {123984, 92438}, {243098, 12309}}};
static constexpr auto arrlookup = std::array<int, 8>{{50, 10, 123, 53498, 1230, 34598, 123984, 243098}};

static constexpr auto created_map_find = heurohash::make_ordered_map<int, int, 8>(arr);

static_assert(created_map_find.find(50) != created_map_find.end());
static_assert(created_map_find.begin() != created_map_find.end());
static_assert(*created_map_find.begin()->first == 10);
static_assert(*created_map_find.begin()->second == 40);

static void MapCreation(benchmark::State& state) {
  // Code inside this loop is measured repeatedly
  for (auto _ : state) {
    std::map<int, int> created_map(arr.begin(), arr.end());
    // Make sure the variable is not optimized away by compiler
    benchmark::DoNotOptimize(created_map);
  }
}
// Register the function as a benchmark
BENCHMARK(MapCreation);

static void OrdMapCreation(benchmark::State& state) {
  // Code inside this loop is measured repeatedly
  for (auto _ : state) {
    auto created_map = heurohash::make_ordered_map<int, int, 8>(arr);
    // Make sure the variable is not optimized away by compiler
    benchmark::DoNotOptimize(created_map);
  }
}
// Register the function as a benchmark
BENCHMARK(OrdMapCreation);

static void MapCopy(benchmark::State& state) {
  // Code before the loop is not measured
   std::map<int, int> created_map(arr.begin(), arr.end());
  for (auto _ : state) {
    std::map<int,int> copy(created_map);
    benchmark::DoNotOptimize(copy);
  }
}
BENCHMARK(MapCopy);

static void OrdMapCopy(benchmark::State& state) {
  // Code before the loop is not measured
   auto created_map = heurohash::make_ordered_map<int, int, 8>(arr);
  for (auto _ : state) {
    auto copy(created_map);
    benchmark::DoNotOptimize(copy);
  }
}
BENCHMARK(OrdMapCopy);

static void MapLookup(benchmark::State& state) {
  std::map<int, int> created_map(arr.begin(), arr.end());
  size_t i = 0;
  for (auto _ : state) {
    int var = created_map[i];
    i = (i + 1) % arrlookup.size();
    benchmark::DoNotOptimize(var);
  }
}
BENCHMARK(MapLookup);

static void OrdMapLookup(benchmark::State& state) {
  // Code before the loop is not measured
  auto created_map = heurohash::make_ordered_map<int, int, 8>(arr);
  size_t i = 0;
  for (auto _ : state) {
    int var = created_map[i];
    i = (i + 1) % arrlookup.size();
    benchmark::DoNotOptimize(var);
  }
}
BENCHMARK(OrdMapLookup);

static void MapLookupConst(benchmark::State& state) {
  // Code before the loop is not measured
  std::map<int, int> created_map(arr.begin(), arr.end());
  for (auto _ : state) {
    int var = created_map[1230];
    benchmark::DoNotOptimize(var);
  }
}
BENCHMARK(MapLookupConst);

static void OrdMapLookupConst(benchmark::State& state) {
  // Code before the loop is not measured
  auto created_map = heurohash::make_ordered_map<int, int, 8>(arr);
  for (auto _ : state) {
    int var = created_map[1230];
    benchmark::DoNotOptimize(var);
  }
}
BENCHMARK(OrdMapLookupConst);

static void OrdMapLookupConstexpr(benchmark::State& state) {
  // Code before the loop is not measured
  static constexpr auto created_map = heurohash::make_ordered_map<int, int, 8>(arr);
  for (auto _ : state) {
    int var = created_map[1230];
    benchmark::DoNotOptimize(var);
  }
}
BENCHMARK(OrdMapLookupConstexpr);

BENCHMARK_MAIN();