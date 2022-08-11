#include <benchmark/benchmark.h>
#include "zen_fmt.h"
#include <charconv>

static void fmt__std_to_chars(benchmark::State& state) {
    char buffer[20];
    const u64 v = 123123123123123;
    for (auto _ : state) {
        std::to_chars(buffer, buffer + 20, v);
        benchmark::DoNotOptimize(buffer);
    }
}
BENCHMARK(fmt__std_to_chars);

static void fmt__fmt_int_to_chars(benchmark::State& state) {
    char buffer[20];
    const u64 v = 123123123123123;
    for (auto _ : state) {
        zen::fmt::int_to_chars(buffer, buffer + 20, v);
        benchmark::DoNotOptimize(buffer);
    }
}
BENCHMARK(fmt__fmt_int_to_chars);
