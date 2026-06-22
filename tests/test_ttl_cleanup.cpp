#include <gtest/gtest.h>

#include "../include/rate_limiter/IRateLimiter.hpp"
#include "../include/rate_limiter/RateLimiterManager.hpp"
#include "../include/rate_limiter/SlidingWindowLog.hpp"

#include <chrono>

using namespace std::chrono_literals;

TEST(TtlCleanupTest, RemovesInactiveKeysAfterTTL) {
    RateLimiterManager<SlidingWindowLog> limiter(
        100ms,  
        2,      
        50ms    
    );

    auto t = IRateLimiter::Clock::now();

    EXPECT_TRUE(limiter.allow("user1", t));
    EXPECT_TRUE(limiter.allow("user2", t));

    EXPECT_EQ(limiter.activeKeys(), 2);

    std::size_t removed = limiter.cleanup(t + 51ms);

    EXPECT_EQ(removed, 2);
    EXPECT_EQ(limiter.activeKeys(), 0);
}

TEST(TtlCleanupTest, DoesNotRemoveActiveKeysBeforeTTL) {
    RateLimiterManager<SlidingWindowLog> limiter(
        100ms,
        2,
        50ms
    );

    auto t = IRateLimiter::Clock::now();

    EXPECT_TRUE(limiter.allow("user1", t));
    EXPECT_TRUE(limiter.allow("user2", t));

    std::size_t removed = limiter.cleanup(t + 49ms);

    EXPECT_EQ(removed, 0);
    EXPECT_EQ(limiter.activeKeys(), 2);
}

TEST(TtlCleanupTest, RecentlyUsedKeySurvivesCleanup) {
    RateLimiterManager<SlidingWindowLog> limiter(
        100ms,
        2,
        50ms
    );

    auto t = IRateLimiter::Clock::now();

    EXPECT_TRUE(limiter.allow("old_user", t));
    EXPECT_TRUE(limiter.allow("active_user", t));

    EXPECT_TRUE(limiter.allow("active_user", t + 40ms));

    std::size_t removed = limiter.cleanup(t + 60ms);

    EXPECT_EQ(removed, 1);
    EXPECT_EQ(limiter.activeKeys(), 1);
}

TEST(TtlCleanupTest, ZeroTTLDisablesCleanup) {
    RateLimiterManager<SlidingWindowLog> limiter(
        100ms,
        2,
        IRateLimiter::Duration::zero()
    );

    auto t = IRateLimiter::Clock::now();

    EXPECT_TRUE(limiter.allow("user1", t));
    EXPECT_TRUE(limiter.allow("user2", t));

    std::size_t removed = limiter.cleanup(t + 1h);

    EXPECT_EQ(removed, 0);
    EXPECT_EQ(limiter.activeKeys(), 2);
}

TEST(TtlCleanupTest, RemovedKeyStartsFreshWhenUsedAgain) {
    RateLimiterManager<SlidingWindowLog> limiter(
        100ms,
        1,
        50ms
    );

    auto t = IRateLimiter::Clock::now();

    EXPECT_TRUE(limiter.allow("user1", t));
    EXPECT_FALSE(limiter.allow("user1", t + 1ms));

    EXPECT_EQ(limiter.cleanup(t + 100ms), 1);
    EXPECT_EQ(limiter.activeKeys(), 0);

    EXPECT_TRUE(limiter.allow("user1", t + 101ms));
    EXPECT_EQ(limiter.activeKeys(), 1);
}

TEST(TtlCleanupTest, NameDefaultAndCustom) {
    RateLimiterManager<SlidingWindowLog> limiter1(100ms, 2, 0s);
    EXPECT_EQ(limiter1.name(), "rate_limiter_manager");

    RateLimiterManager<SlidingWindowLog> limiter2(100ms, 2, 0s, "my_limiter");
    EXPECT_EQ(limiter2.name(), "my_limiter");
}

TEST(TtlCleanupTest, EvictionCountAfterCleanup) {
    RateLimiterManager<SlidingWindowLog> limiter(100ms, 2, 50ms);
    auto t = IRateLimiter::Clock::now();

    ASSERT_TRUE(limiter.allow("a", t));
    ASSERT_TRUE(limiter.allow("b", t));

    size_t removed = limiter.cleanup(t + 60ms);
    EXPECT_EQ(removed, 2);
    EXPECT_EQ(limiter.evictionCount(), 2);

    ASSERT_TRUE(limiter.allow("c", t + 100ms));
    removed = limiter.cleanup(t + 200ms);
    EXPECT_EQ(removed, 1);
    EXPECT_EQ(limiter.evictionCount(), 3);
}

TEST(TtlCleanupTest, CleanupNoArgs) {
    RateLimiterManager<SlidingWindowLog> limiter(100ms, 2, 50ms);
    limiter.allow("user", IRateLimiter::Clock::now());
    EXPECT_NO_THROW(limiter.cleanup());
}

TEST(TtlCleanupTest, CleanupEmptyState) {
    RateLimiterManager<SlidingWindowLog> limiter(100ms, 2, 50ms);
    auto t = IRateLimiter::Clock::now();

    size_t removed = limiter.cleanup(t);
    EXPECT_EQ(removed, 0);
    EXPECT_EQ(limiter.activeKeys(), 0);
}

TEST(TtlCleanupTest, ExistingKeySuccessUpdatesLastAccess) {
    RateLimiterManager<SlidingWindowLog> limiter(100ms, 3, 50ms);
    auto t = IRateLimiter::Clock::now();

    ASSERT_TRUE(limiter.allow("user", t));
    EXPECT_TRUE(limiter.allow("user", t + 10ms));
    
    size_t removed = limiter.cleanup(t + 55ms);
    EXPECT_EQ(removed, 0);
    EXPECT_EQ(limiter.activeKeys(), 1);

    removed = limiter.cleanup(t + 75ms);
    EXPECT_EQ(removed, 1);
    EXPECT_EQ(limiter.activeKeys(), 0);
}