#include <gtest/gtest.h>
#include "../include/rate_limiter/IRateLimiter.hpp"
#include <chrono>
#include <memory>
#include <string>

using namespace std::chrono_literals;


class TestLimiter : public IRateLimiter {
public:
    using IRateLimiter::allow;

    bool allow(const std::string& key, TimePoint now) override {
        last_key = key;
        last_time = now;
        return true;
    }

    std::string name() const override {
        return "test_limiter";
    }

    std::string last_key;
    TimePoint last_time = TimePoint::min();
};


TEST(IRateLimiterTest, AllowWithoutTimeDelegatesToVirtual) {
    TestLimiter limiter;
    auto before = IRateLimiter::Clock::now();
    bool result = limiter.allow("user");
    auto after = IRateLimiter::Clock::now();

    EXPECT_TRUE(result);
    EXPECT_EQ(limiter.last_key, "user");

    EXPECT_GE(limiter.last_time, before);
    EXPECT_LE(limiter.last_time, after);
}

TEST(IRateLimiterTest, DefaultCleanupReturnsZero) {
    TestLimiter limiter;
    auto t = IRateLimiter::Clock::now();
    EXPECT_EQ(limiter.cleanup(t), 0);
}

TEST(IRateLimiterTest, CleanupNoArgsCallsOverload) {
    TestLimiter limiter;
    EXPECT_EQ(limiter.cleanup(), 0);
}

TEST(IRateLimiterTest, DefaultActiveKeysReturnsZero) {
    TestLimiter limiter;
    EXPECT_EQ(limiter.activeKeys(), 0);
}

TEST(IRateLimiterTest, DefaultEvictionCountReturnsZero) {
    TestLimiter limiter;
    EXPECT_EQ(limiter.evictionCount(), 0);
}

TEST(IRateLimiterTest, NameReturnsCorrectValue) {
    TestLimiter limiter;
    EXPECT_EQ(limiter.name(), "test_limiter");
}

TEST(IRateLimiterTest, PolymorphicDeleteWorks) {
    std::unique_ptr<IRateLimiter> ptr = std::make_unique<TestLimiter>();
    EXPECT_NO_THROW(ptr.reset());
}