#include <gtest/gtest.h>

#include "SlidingWindowLog.hpp"
#include "SlidingWindowLogManager.hpp"

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

    EXPECT_TRUE(limiter.allow(t + 100ms));
    EXPECT_EQ(limiter.usedRequests(), 3);
}

TEST(SlidingWindowLogTest, NonMonotonicTimeIsSafe) {
    SlidingWindowLog limiter(100ms, 2);
    auto t = Clock::now();

    ASSERT_TRUE(limiter.allow(t + 100ms));
    ASSERT_TRUE(limiter.allow(t + 200ms));
    EXPECT_FALSE(limiter.allow(t + 50ms));

    EXPECT_TRUE(limiter.allow(t + 301ms));
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






using ClockManager = SlidingWindowLogManager::Clock;

class SlidingWindowLogManagerTest : public ::testing::Test {
protected:
    static constexpr auto window = 100ms;
    static constexpr size_t limit = 3;
    SlidingWindowLogManager manager{window, limit};
};

TEST_F(SlidingWindowLogManagerTest, DifferentKeysHaveIndependentLimits) {
    auto now = ClockManager::now();

    EXPECT_TRUE(manager.allow("A", now));
    EXPECT_TRUE(manager.allow("A", now + 1ms));
    EXPECT_TRUE(manager.allow("A", now + 2ms));
    EXPECT_FALSE(manager.allow("A", now + 3ms));

    EXPECT_TRUE(manager.allow("B", now));
    EXPECT_TRUE(manager.allow("B", now + 1ms));
    EXPECT_TRUE(manager.allow("B", now + 2ms));
    EXPECT_FALSE(manager.allow("B", now + 3ms));

    EXPECT_TRUE(manager.allow("C", now + 50ms));
    EXPECT_TRUE(manager.allow("C", now + 51ms));
}

TEST_F(SlidingWindowLogManagerTest, WindowSlidesPerKey) {
    auto t0 = ClockManager::now();

    ASSERT_TRUE(manager.allow("X", t0));
    ASSERT_TRUE(manager.allow("X", t0 + 10ms));
    ASSERT_TRUE(manager.allow("X", t0 + 20ms));
    ASSERT_FALSE(manager.allow("X", t0 + 30ms));
    
    EXPECT_TRUE(manager.allow("X", t0 + 101ms));
    EXPECT_FALSE(manager.allow("X", t0 + 102ms));

    EXPECT_TRUE(manager.allow("X", t0 + 111ms));

    EXPECT_TRUE(manager.allow("Y", t0 + 101ms));
}

TEST_F(SlidingWindowLogManagerTest, NewKeyStartsFresh) {
    auto now = ClockManager::now();

    EXPECT_TRUE(manager.allow("late", now + 500ms));
    EXPECT_TRUE(manager.allow("late", now + 501ms));
    EXPECT_TRUE(manager.allow("late", now + 502ms));
    EXPECT_FALSE(manager.allow("late", now + 503ms));
}

TEST_F(SlidingWindowLogManagerTest, ProductionAllowUsesSystemTime) {
    bool result = manager.allow("real_time_key");
    EXPECT_TRUE(result);
}

TEST_F(SlidingWindowLogManagerTest, ManyKeysStress) {
    auto now = ClockManager::now();
    constexpr int numKeys = 5000;
    for (int i = 0; i < numKeys; ++i) {
        std::string key = "k" + std::to_string(i);

        ASSERT_TRUE(manager.allow(key, now));
        ASSERT_TRUE(manager.allow(key, now + 10ms));
        ASSERT_TRUE(manager.allow(key, now + 20ms));
        ASSERT_FALSE(manager.allow(key, now + 30ms));
    }
    EXPECT_TRUE(manager.allow("k0", now + 101ms));
}

TEST_F(SlidingWindowLogManagerTest, SpecialKeyNames) {
    auto now = ClockManager::now();

    EXPECT_TRUE(manager.allow("", now));
    EXPECT_TRUE(manager.allow(std::string(1000, 'a'), now));
    EXPECT_TRUE(manager.allow("key with spaces", now));
    EXPECT_TRUE(manager.allow("ключ_на_русском", now));
}

TEST_F(SlidingWindowLogManagerTest, ThreadSafetyMultiKey) {
    constexpr int kThreads = 8;
    constexpr int kKeysPerThread = 100;
    std::atomic<int> totalAllowed{0};
    auto now = ClockManager::now();

    auto worker = [&](int threadId) {
        for (int i = 0; i < kKeysPerThread; ++i) {
            std::string key = "t" + std::to_string(threadId) + "_" + std::to_string(i);
        
            if (manager.allow(key, now)) ++totalAllowed;
            if (manager.allow(key, now + 1ms)) ++totalAllowed;
            if (manager.allow(key, now + 2ms)) ++totalAllowed;
            if (manager.allow(key, now + 3ms)) ++totalAllowed;
        }
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back(worker, i);
    }
    for (auto& t : threads) t.join();

    int expected = kThreads * kKeysPerThread * 3;
    EXPECT_EQ(totalAllowed, expected);
}

TEST_F(SlidingWindowLogManagerTest, KeyCleanupAfterLongIdle) {
    auto t = ClockManager::now();
    ASSERT_TRUE(manager.allow("idle", t));
    ASSERT_TRUE(manager.allow("idle", t + 1ms));
    ASSERT_TRUE(manager.allow("idle", t + 2ms));
    ASSERT_FALSE(manager.allow("idle", t + 3ms));

    EXPECT_TRUE(manager.allow("idle", t + 200ms));
    EXPECT_TRUE(manager.allow("idle", t + 201ms));
    EXPECT_TRUE(manager.allow("idle", t + 202ms));
    EXPECT_FALSE(manager.allow("idle", t + 203ms));
}