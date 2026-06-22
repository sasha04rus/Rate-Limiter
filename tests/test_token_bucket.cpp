#include <gtest/gtest.h>

#include <chrono>
#include <thread>
#include <vector>
#include <atomic>

#include "../include/rate_limiter/TokenBucket.hpp"

using Clock = TokenBucket::Clock;

TEST(TokenBucketTest, RateAccuracy) {
    TokenBucket bucket(std::chrono::seconds(40), 2);

    auto now = Clock::now();

    EXPECT_TRUE(bucket.allow(now));
    EXPECT_TRUE(bucket.allow(now));
    EXPECT_FALSE(bucket.allow(now));

    now += std::chrono::seconds(20);

    EXPECT_TRUE(bucket.allow(now))
        << "Один токен должен быть восстановлен";

    EXPECT_FALSE(bucket.allow(now))
        << "Второго токена сразу нет";
}

TEST(TokenBucketTest, BurstCapacity) {
    TokenBucket bucket(std::chrono::seconds(40), 2);

    auto now = Clock::now();

    std::vector<bool> results;

    for (int i = 0; i < 10; i++) {
        results.push_back(bucket.allow(now));
    }

    EXPECT_TRUE(results[0]);
    EXPECT_TRUE(results[1]);

    for (size_t i = 2; i < results.size(); i++) {
        EXPECT_FALSE(results[i]);
    }
}

TEST(TokenBucketTest, TokenRefillIsPartial) {
    TokenBucket bucket(std::chrono::seconds(40), 2);

    auto now = Clock::now();

    EXPECT_TRUE(bucket.allow(now));
    EXPECT_TRUE(bucket.allow(now));
    EXPECT_FALSE(bucket.allow(now));

    now += std::chrono::seconds(10);

    EXPECT_FALSE(bucket.allow(now))
        << "За 10 секунд восстановится только 0.5 токена";
}

TEST(TokenBucketTest, TokenRefillAfterEnoughTime) {
    TokenBucket bucket(std::chrono::seconds(40), 2);

    auto now = Clock::now();

    EXPECT_TRUE(bucket.allow(now));
    EXPECT_TRUE(bucket.allow(now));
    EXPECT_FALSE(bucket.allow(now));

    now += std::chrono::seconds(21);

    EXPECT_TRUE(bucket.allow(now));
}

TEST(TokenBucketTest, CapacityDoesNotGrowAboveLimit) {
    TokenBucket bucket(std::chrono::seconds(40), 2);

    auto now = Clock::now();

    now += std::chrono::seconds(100);

    EXPECT_TRUE(bucket.allow(now));
    EXPECT_TRUE(bucket.allow(now));
    EXPECT_FALSE(bucket.allow(now))
        << "Токенов не должно стать больше capacity";
}

TEST(TokenBucketTest, CustomCapacityWorks) {
    TokenBucket bucket(std::chrono::seconds(100), 5);

    auto now = Clock::now();

    EXPECT_TRUE(bucket.allow(now));
    EXPECT_TRUE(bucket.allow(now));
    EXPECT_TRUE(bucket.allow(now));
    EXPECT_TRUE(bucket.allow(now));
    EXPECT_TRUE(bucket.allow(now));

    EXPECT_FALSE(bucket.allow(now))
        << "При max_requests = 5 шестой запрос должен быть отклонён";
}

TEST(TokenBucketTest, FastRefillWorks) {
    TokenBucket bucket(std::chrono::seconds(1), 1);

    auto now = Clock::now();

    EXPECT_TRUE(bucket.allow(now));
    EXPECT_FALSE(bucket.allow(now));

    now += std::chrono::seconds(1);

    EXPECT_TRUE(bucket.allow(now));
}

TEST(TokenBucketTest, ThreadSafety) {
    TokenBucket bucket(std::chrono::seconds(40), 2);

    constexpr int THREADS = 20;
    std::atomic<int> allowed{0};

    auto worker = [&]() {
        if (bucket.allow()) {
            allowed++;
        }
    };

    std::vector<std::thread> threads;

    for (int i = 0; i < THREADS; i++) {
        threads.emplace_back(worker);
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_LE(allowed.load(), 2);
}

TEST(TokenBucketTest, AllowWithoutTimePointWorks) {
    TokenBucket bucket(std::chrono::seconds(40), 2);

    EXPECT_TRUE(bucket.allow());
    EXPECT_TRUE(bucket.allow());
    EXPECT_FALSE(bucket.allow());
}

TEST(TokenBucketTest, TimeCannotGoBackwards) {
    TokenBucket bucket(std::chrono::seconds(40), 2);

    auto now = Clock::now();

    EXPECT_TRUE(bucket.allow(now));
    EXPECT_TRUE(bucket.allow(now));
    EXPECT_FALSE(bucket.allow(now));

    now -= std::chrono::seconds(100);

    EXPECT_FALSE(bucket.allow(now))
        << "Если время ушло назад, токены не должны восстановиться";
}
