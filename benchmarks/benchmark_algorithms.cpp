// benchmarks/benchmark_algorithms.cpp
#include <benchmark/benchmark.h>

#include "../include/rate_limiter/SlidingWindowLog.hpp"
#include "../include/rate_limiter/LeakingBucket.hpp"
#include "../include/rate_limiter/RateLimiterManager.hpp"
#include "../include/rate_limiter/IRateLimiter.hpp"

#include <chrono>
#include <memory>
#include <string>
#include <vector>

using namespace std::chrono_literals;

constexpr auto   WINDOW          = 1s;
constexpr size_t LIMIT           = 1000;
constexpr double BUCKET_CAPACITY = static_cast<double>(LIMIT);
constexpr double BUCKET_REFILL   = BUCKET_CAPACITY / 1.0; // полный слив за 1 секунду

std::unique_ptr<RateLimiter> make_token_bucket() {
    // TTL = 0 (никогда не удаляем ключи автоматически)
    // cleanInterval = 1 час (фоновый поток практически не мешает)
    return std::make_unique<RateLimiter>(
        std::chrono::seconds(0),
        std::chrono::seconds(3600),
        BUCKET_CAPACITY,
        BUCKET_REFILL
    );
}


static void BM_SlidingWindow_Allow(benchmark::State& state) {
    SlidingWindowLog limiter(WINDOW, LIMIT);
    auto now = SlidingWindowLog::Clock::now();
    for (auto _ : state) {
        bool ok = limiter.allow(now);
        benchmark::DoNotOptimize(ok);
        now += 1us; // чтобы лимит не исчерпался
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_SlidingWindow_Allow);

static void BM_LeakingBucket_Allow(benchmark::State& state) {
    LeakingBucket limiter(WINDOW, LIMIT);
    auto now = LeakingBucket::Clock::now();
    for (auto _ : state) {
        bool ok = limiter.allow(now);
        benchmark::DoNotOptimize(ok);
        now += 1us;
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LeakingBucket_Allow);


static void BM_Manager_SlidingWindow_SingleKey(benchmark::State& state) {
    RateLimiterManager<SlidingWindowLog> mgr(WINDOW, LIMIT, 0s);
    auto now = IRateLimiter::Clock::now();
    const std::string key = "single";
    for (auto _ : state) {
        bool ok = mgr.allow(key, now);
        benchmark::DoNotOptimize(ok);
        now += 1us;
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Manager_SlidingWindow_SingleKey);

static void BM_Manager_LeakingBucket_SingleKey(benchmark::State& state) {
    RateLimiterManager<LeakingBucket> mgr(WINDOW, LIMIT, 0s);
    auto now = IRateLimiter::Clock::now();
    const std::string key = "single";
    for (auto _ : state) {
        bool ok = mgr.allow(key, now);
        benchmark::DoNotOptimize(ok);
        now += 1us;
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Manager_LeakingBucket_SingleKey);

static void BM_TokenBucket_SingleKey(benchmark::State& state) {
    auto mgr = make_token_bucket();
    auto now = IRateLimiter::Clock::now();
    const std::string key = "single";
    for (auto _ : state) {
        bool ok = mgr->allow(key, now);
        benchmark::DoNotOptimize(ok);
        now += 1us;
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_TokenBucket_SingleKey);


template <typename ManagerMaker>
static void BM_ManyKeys(benchmark::State& state, ManagerMaker maker) {
    auto mgr = maker();
    const int num_keys = state.range(0);
    std::vector<std::string> keys(num_keys);
    for (int i = 0; i < num_keys; ++i) {
        keys[i] = "key_" + std::to_string(i);
    }
    auto now = IRateLimiter::Clock::now();
    int idx = 0;
    for (auto _ : state) {
        bool ok = mgr->allow(keys[idx % num_keys], now);
        benchmark::DoNotOptimize(ok);
        now += 1us;
        ++idx;
    }
    state.SetItemsProcessed(state.iterations());
    state.SetComplexityN(state.range(0));
}


auto make_sliding_manager = []() {
    return std::make_unique<RateLimiterManager<SlidingWindowLog>>(WINDOW, LIMIT, 0s);
};
auto make_leaking_manager = []() {
    return std::make_unique<RateLimiterManager<LeakingBucket>>(WINDOW, LIMIT, 0s);
};
auto make_token_manager = []() {
    return make_token_bucket();
};

BENCHMARK_CAPTURE(BM_ManyKeys, SlidingWindow, make_sliding_manager)
    ->Arg(1)->Arg(10)->Arg(100)->Arg(1000)->Arg(10000)
    ->Complexity();
BENCHMARK_CAPTURE(BM_ManyKeys, LeakingBucket, make_leaking_manager)
    ->Arg(1)->Arg(10)->Arg(100)->Arg(1000)->Arg(10000)
    ->Complexity();
BENCHMARK_CAPTURE(BM_ManyKeys, TokenBucket, make_token_manager)
    ->Arg(1)->Arg(10)->Arg(100)->Arg(1000)->Arg(10000)
    ->Complexity();


BENCHMARK_MAIN();
