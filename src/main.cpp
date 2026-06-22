#include "../load_tester/TrafficPattern.hpp"
#include "../load_tester/LoadTester.hpp"

#include "../include/rate_limiter/IRateLimiter.hpp"
#include "../include/rate_limiter/TokenBucket.hpp"
#include "../include/rate_limiter/RateLimiterManager.hpp"
#include "../include/rate_limiter/LeakingBucket.hpp"
#include "../include/rate_limiter/SlidingWindowLog.hpp"

#include <chrono>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

using namespace std::chrono_literals;

int main() {
    TrafficPatternConfig config;

    config.clients = 1000;
    config.ratePerSecond = 1000.0;
    config.duration = 10s;

    config.burstSize = 500;
    config.burstPeriod = 1000ms;

    config.seed = 42;

    LoadTester tester;

    std::cout
        << "algorithm,"
        << "pattern,"
        << "total_requests,"
        << "allowed_requests,"
        << "denied_requests,"
        << "throughput_rps,"
        << "p50_latency_ns,"
        << "p99_latency_ns,"
        << "key_count,"
        << "eviction_count"
        << '\n';

    std::vector<std::function<std::unique_ptr<TrafficPattern>()>> patternFactories;

    patternFactories.push_back([&]() {
        return makeUniformPattern(config);
    });

    patternFactories.push_back([&]() {
        return makeBurstPattern(config);
    });

    patternFactories.push_back([&]() {
        return makeDiurnalPattern(config);
    });

    patternFactories.push_back([&]() {
        return makeSparseBurstPattern(config);
    });

    patternFactories.push_back([&]() {
        return makeZombiePattern(config);
    });

    std::vector<std::function<std::unique_ptr<IRateLimiter>()>> limiterFactories;

    limiterFactories.push_back([]() {
        return std::make_unique<RateLimiterManager<TokenBucket>>(
            40s,            
            2,              
            60s,            
            "token_bucket"  
        );
    });

    limiterFactories.push_back([]() {
        return std::make_unique<RateLimiterManager<SlidingWindowLog>>(
            1s,                         
            100,                        
            60s,                        
            "sliding_window_log"        
        );
    });

    limiterFactories.push_back([]() {
        return std::make_unique<RateLimiterManager<LeakingBucket>>(
            1s,                         
            100,                        
            60s,                        
            "leaking_bucket"            
        );
    });

    for (auto& makeLimiter : limiterFactories) {
        for (auto& makePattern : patternFactories) {
            auto limiter = makeLimiter();
            auto pattern = makePattern();

            auto result = tester.run(*limiter, *pattern);

            std::cout
                << limiter->name() << ','
                << result.pattern_name << ','
                << result.total_requests << ','
                << result.allowed_requests << ','
                << result.denied_requests << ','
                << result.throughput_rps << ','
                << result.p50_latency_ns << ','
                << result.p99_latency_ns << ','
                << limiter->activeKeys() << ','
                << limiter->evictionCount()
                << '\n';
        }
    }

    return 0;
}