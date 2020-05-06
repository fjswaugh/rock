#include <benchmark/benchmark.h>
#include <rock/algorithms.h>
#include <rock/starting_position.h>

static auto bm_count_moves(benchmark::State& state)
{
    for (auto _ : state)
    {
        auto const count = rock::count_moves(rock::starting_position, 5);
        benchmark::DoNotOptimize(count);
    }
}
BENCHMARK(bm_count_moves);

BENCHMARK_MAIN();
