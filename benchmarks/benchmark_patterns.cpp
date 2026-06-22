// benchmarks/benchmark_patterns.cpp
#include <benchmark/benchmark.h>

#include "../load_tester/TrafficPattern.hpp"

#include <chrono>
#include <cstdint>
#include <memory>

using namespace std::chrono_literals;

using PatternFactory = std::unique_ptr<TrafficPattern> (*)(TrafficPatternConfig);

static TrafficPatternConfig make_config(benchmark::State& state) {
    TrafficPatternConfig config;

    config.clients = static_cast<std::size_t>(state.range(0));
    config.ratePerSecond = static_cast<double>(state.range(1));
    config.duration = 1s;

    config.burstSize = 1000;
    config.burstPeriod = 100ms;

    config.seed = 42;

    return config;
}

static void BM_TrafficPattern_Generation(
    benchmark::State& state,
    PatternFactory factory
) {
    std::uint64_t generated_requests = 0;

    for (auto _ : state) {
        auto config = make_config(state);
        auto pattern = factory(config);

        while (auto request = pattern->next()) {
            benchmark::DoNotOptimize(request->time);
            benchmark::DoNotOptimize(request->key.data());

            ++generated_requests;
        }
    }

    state.SetItemsProcessed(static_cast<std::int64_t>(generated_requests));
}

BENCHMARK_CAPTURE(
    BM_TrafficPattern_Generation,
    Uniform,
    makeUniformPattern
)
    ->Args({100, 1000})
    ->Args({1000, 1000})
    ->Args({1000, 10000})
    ->Args({10000, 10000});

BENCHMARK_CAPTURE(
    BM_TrafficPattern_Generation,
    Burst,
    makeBurstPattern
)
    ->Args({100, 1000})
    ->Args({1000, 1000})
    ->Args({1000, 10000})
    ->Args({10000, 10000});

BENCHMARK_CAPTURE(
    BM_TrafficPattern_Generation,
    Diurnal,
    makeDiurnalPattern
)
    ->Args({100, 1000})
    ->Args({1000, 1000})
    ->Args({1000, 10000})
    ->Args({10000, 10000});

BENCHMARK_CAPTURE(
    BM_TrafficPattern_Generation,
    SparseBurst,
    makeSparseBurstPattern
)
    ->Args({100, 1000})
    ->Args({1000, 1000})
    ->Args({1000, 10000})
    ->Args({10000, 10000});

BENCHMARK_CAPTURE(
    BM_TrafficPattern_Generation,
    Zombie,
    makeZombiePattern
)
    ->Args({100, 1000})
    ->Args({1000, 1000})
    ->Args({1000, 10000})
    ->Args({10000, 10000});

BENCHMARK_MAIN();