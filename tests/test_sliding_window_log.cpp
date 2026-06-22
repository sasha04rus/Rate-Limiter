#include <gtest/gtest.h>

#include "../include/rate_limiter/SlidingWindowLog.hpp"
#include "../include/rate_limiter/RateLimiterManager.hpp"

#include <thread>
#include <atomic>
#include <vector>

using namespace std::chrono_literals;
using Clock = SlidingWindowLog::Clock;


TEST(SlidingWindowLogTest, AllowsUpToLimit) {
    SlidingWindowLog limiter(10s, 3);
    auto base = Clock::now();

    EXPECT_TRUE(limiter.allow(base));
    EXPECT_TRUE(limiter.allow(base + 1ms));
    EXPECT_TRUE(limiter.allow(base + 2ms));
    EXPECT_FALSE(limiter.allow(base + 3ms));
}

TEST(SlidingWindowLogTest, WindowSlides) {
    SlidingWindowLog limiter(100ms, 2);
    auto t = Clock::now();

    ASSERT_TRUE(limiter.allow(t));
    ASSERT_TRUE(limiter.allow(t + 10ms));
    ASSERT_FALSE(limiter.allow(t + 20ms));

    ASSERT_TRUE(limiter.allow(t + 101ms));
    EXPECT_FALSE(limiter.allow(t + 102ms));

    EXPECT_TRUE(limiter.allow(t + 111ms));
}

TEST(SlidingWindowLogTest, CleanupOnIdle) {
    SlidingWindowLog limiter(50ms, 1);
    auto t = Clock::now();

    ASSERT_TRUE(limiter.allow(t));
    ASSERT_FALSE(limiter.allow(t + 1ms));

    EXPECT_TRUE(limiter.allow(t + 100ms));
    EXPECT_EQ(limiter.usedRequests(), 1);
}

TEST(SlidingWindowLogTest, ZeroLimitAlwaysDenies) {
    SlidingWindowLog limiter(100ms, 0);
    auto now = Clock::now();

    EXPECT_FALSE(limiter.allow(now));
    EXPECT_FALSE(limiter.allow(now + 1s));
    EXPECT_EQ(limiter.usedRequests(), 0);
}

TEST(SlidingWindowLogTest, ZeroWindow) {
    SlidingWindowLog limiter(0s, 2);
    auto now = Clock::now();

    EXPECT_TRUE(limiter.allow(now));
    EXPECT_TRUE(limiter.allow(now));
    EXPECT_TRUE(limiter.allow(now));
}

TEST(SlidingWindowLogTest, LargeWindowAndLimit) {
    SlidingWindowLog limiter(1h, 1000);
    auto t = Clock::now();

    for (int i = 0; i < 1000; ++i) {
        EXPECT_TRUE(limiter.allow(t + i * 1ms));
    }
    EXPECT_FALSE(limiter.allow(t + 1000ms));
}

TEST(SlidingWindowLogTest, ChangingParameters) {
    SlidingWindowLog limiter(100ms, 2);
    auto t = Clock::now();

    ASSERT_TRUE(limiter.allow(t));
    ASSERT_TRUE(limiter.allow(t + 10ms));
    ASSERT_FALSE(limiter.allow(t + 20ms));

    limiter.setMaxRequests(5);

    EXPECT_TRUE(limiter.allow(t + 30ms));
    EXPECT_TRUE(limiter.allow(t + 40ms));

    limiter.setDuration(50ms);

    ASSERT_TRUE(limiter.allow(t + 100ms));
    EXPECT_EQ(limiter.usedRequests(), 1);
}

TEST(SlidingWindowLogTest, NonMonotonicTimeIsSafe) {
    SlidingWindowLog limiter(100ms, 2);
    auto t = Clock::now();

    ASSERT_TRUE(limiter.allow(t + 100ms));
    ASSERT_TRUE(limiter.allow(t + 200ms));

    EXPECT_FALSE(limiter.allow(t + 50ms));

    ASSERT_TRUE(limiter.allow(t + 400ms));
    EXPECT_TRUE(limiter.allow(t + 410ms));
}

TEST(SlidingWindowLogTest, ThreadSafety) {
    SlidingWindowLog limiter(200ms, 100);
    constexpr int kThreads = 10;
    constexpr int kCallsPerThread = 30;
    auto now = Clock::now();

    std::atomic<int> allowed{0};
    auto worker = [&]() {
        for (int i = 0; i < kCallsPerThread; ++i) {
            if (limiter.allow()) {
                ++allowed;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    };
    std::vector<std::thread> threads;
    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back(worker);
    }
    for (auto& t : threads) t.join();

    EXPECT_GT(allowed, 0);
    EXPECT_LE(allowed, kThreads * kCallsPerThread);
}