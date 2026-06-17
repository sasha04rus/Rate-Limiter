#include <gtest/gtest.h>
#include "SlidingWindowLog.hpp"

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
    EXPECT_TRUE(limiter.allow(t + 105ms));
    EXPECT_FALSE(limiter.allow(t + 102ms));
}

TEST(SlidingWindowLogTest, CleanupOnIdle) {
    SlidingWindowLog limiter(50ms, 1);
    auto t = Clock::now();

    ASSERT_TRUE(limiter.allow(t));
    ASSERT_FALSE(limiter.allow(t + 1ms));

    EXPECT_TRUE(limiter.allow(t + 100ms));
    EXPECT_EQ(limiter.used_requests(), 1);
}