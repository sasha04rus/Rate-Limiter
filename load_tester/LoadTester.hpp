#pragma once

#include "TrafficPattern.hpp"
#include "IRateLimiter.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <vector>
#include <string>

struct LoadTestResult {
    std::string algorithm_name;
    std::string pattern_name;
    std::uint64_t total_requests = 0;
    std::uint64_t allowed_requests = 0;
    std::uint64_t denied_requests = 0;
    double allowed_ratio = 0.0;
    std::size_t active_keys = 0;
    std::size_t eviction_count = 0;
    double throughput_rps = 0.0;
    double p50_latency_ns = 0.0;
    double p99_latency_ns = 0.0;
};

class LoadTester {
public:
    LoadTestResult run(IRateLimiter& limiter, TrafficPattern& pattern) {
        using MeasureClock = std::chrono::steady_clock;

        LoadTestResult result;
        result.algorithm_name = limiter.name();
        result.pattern_name = pattern.name();

        std::vector<double> latencies;
        latencies.reserve(10000);

        auto started = MeasureClock::now();

        while (auto request = pattern.next()) {
            auto before = MeasureClock::now();
            bool allowed = limiter.allow(request->key, request->time);
            if (result.total_requests % 1000 == 0)
                limiter.cleanup(request->time);

            auto after = MeasureClock::now();
            auto latency = std::chrono::duration_cast<std::chrono::nanoseconds>(after - before).count();

            latencies.push_back(static_cast<double>(latency));

            result.total_requests++;

            if (allowed) result.allowed_requests++;
            else result.denied_requests++;
        }

        auto finished = MeasureClock::now();

        double seconds = std::chrono::duration<double>(finished - started).count();

        if (seconds > 0.0)
            result.throughput_rps = static_cast<double>(result.total_requests) / seconds;

        if (!latencies.empty()) {
            std::sort(latencies.begin(), latencies.end());
            result.p50_latency_ns = percentile(latencies, 0.50);
            result.p99_latency_ns = percentile(latencies, 0.99);
        }

        if (result.total_requests > 0) {
            result.allowed_ratio =
                static_cast<double>(result.allowed_requests) /
                static_cast<double>(result.total_requests);
        }

        result.active_keys = limiter.activeKeys();
        result.eviction_count = limiter.evictionCount();
        return result;
    }

private:
    static double percentile(const std::vector<double>& values, double p) {
        if (values.empty())
            return 0.0;

        std::size_t index = static_cast<std::size_t>(p * static_cast<double>(values.size() - 1));
        return values[index];
    }
};