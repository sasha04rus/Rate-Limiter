#include <gtest/gtest.h>

#include "../include/rate_limiter/TokenBucket.hpp"
#include "../include/rate_limiter/RateLimiterManager.hpp"

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

using RateLimiter = RateLimiterManager<TokenBucket>;

TEST(RateLimiterTest, RateAccuracy) {
    RateLimiter limiter(40s, 2, 60s, "token_bucket");

    auto t = RateLimiter::Clock::now();

    EXPECT_TRUE(limiter.allow("user1", t))
        << "Первый запрос должен пройти";

    EXPECT_TRUE(limiter.allow("user1", t))
        << "Второй запрос должен пройти";

    EXPECT_FALSE(limiter.allow("user1", t))
        << "Лимит исчерпан";

    EXPECT_TRUE(limiter.allow("user1", t + 20s))
        << "Один токен должен быть восстановлен";

    EXPECT_FALSE(limiter.allow("user1", t + 20s))
        << "Второго токена сразу нет, только через 20 секунд";
}

TEST(RateLimiterTest, BurstCapacity) {
    RateLimiter limiter(40s, 2, 60s, "token_bucket");

    auto t = RateLimiter::Clock::now();

    std::vector<bool> results;

    for (int i = 0; i < 10; ++i) {
        results.push_back(limiter.allow("burstUser", t));
    }

    EXPECT_TRUE(results[0]);
    EXPECT_TRUE(results[1]);

    for (std::size_t i = 2; i < results.size(); ++i) {
        EXPECT_FALSE(results[i])
            << "Ожидаем: results = [true, true, false, false, false, false, false, false, false, false]";
    }
}

TEST(RateLimiterTest, ThreadSafety) {
    RateLimiter limiter(40s, 2, 60s, "token_bucket");

    constexpr int THREADS = 20;

    std::atomic<int> allowed{0};
    auto t = RateLimiter::Clock::now();

    auto worker = [&]() {
        if (limiter.allow("the_same_user", t)) {
            ++allowed;
        }
    };

    std::vector<std::thread> threads;

    for (int i = 0; i < THREADS; ++i) {
        threads.emplace_back(worker);
    }

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_LE(allowed.load(), 2);
}

TEST(RateLimiterTest, CleanOldBucket) {
    RateLimiter limiter(40s, 2, 3s, "token_bucket");

    auto t = RateLimiter::Clock::now();

    EXPECT_TRUE(limiter.allow("old_user", t));
    EXPECT_TRUE(limiter.allow("new_user", t + 2s));

    EXPECT_EQ(limiter.activeKeys(), 2);

    std::size_t removed = limiter.cleanup(t + 4s);

    EXPECT_EQ(removed, 1);
    EXPECT_EQ(limiter.activeKeys(), 1);

    EXPECT_TRUE(limiter.allow("new_user", t + 4s));
    EXPECT_EQ(limiter.activeKeys(), 1);
}

TEST(RateLimiterTest, DifferentUsersHaveDifferentBuckets) {
    RateLimiter limiter(40s, 2, 60s, "token_bucket");

    auto t = RateLimiter::Clock::now();

    EXPECT_TRUE(limiter.allow("user1", t));
    EXPECT_TRUE(limiter.allow("user1", t));
    EXPECT_FALSE(limiter.allow("user1", t));

    EXPECT_TRUE(limiter.allow("user2", t))
        << "У другого пользователя должен быть свой отдельный bucket";

    EXPECT_TRUE(limiter.allow("user2", t));
    EXPECT_FALSE(limiter.allow("user2", t));
}

TEST(RateLimiterTest, TokenRefillIsPartial) {
    RateLimiter limiter(40s, 2, 60s, "token_bucket");

    auto t = RateLimiter::Clock::now();

    EXPECT_TRUE(limiter.allow("user1", t));
    EXPECT_TRUE(limiter.allow("user1", t));
    EXPECT_FALSE(limiter.allow("user1", t));

    EXPECT_FALSE(limiter.allow("user1", t + 10s))
        << "За 10 секунд восстановится только 0.5 токена, этого недостаточно";
}

TEST(RateLimiterTest, TokenRefillAfterEnoughTime) {
    RateLimiter limiter(40s, 2, 60s, "token_bucket");

    auto t = RateLimiter::Clock::now();

    EXPECT_TRUE(limiter.allow("user1", t));
    EXPECT_TRUE(limiter.allow("user1", t));
    EXPECT_FALSE(limiter.allow("user1", t));

    EXPECT_TRUE(limiter.allow("user1", t + 21s))
        << "Через 20+ секунд должен восстановиться один токен";
}

TEST(RateLimiterTest, CapacityDoesNotGrowAboveLimit) {
    RateLimiter limiter(40s, 2, 60s, "token_bucket");

    auto t = RateLimiter::Clock::now();

    EXPECT_TRUE(limiter.allow("user1", t));
    EXPECT_TRUE(limiter.allow("user1", t));
    EXPECT_FALSE(limiter.allow("user1", t));

    EXPECT_TRUE(limiter.allow("user1", t + 100s));
    EXPECT_TRUE(limiter.allow("user1", t + 100s));

    EXPECT_FALSE(limiter.allow("user1", t + 100s))
        << "Даже после долгого ожидания токенов не должно быть больше capacity";
}

TEST(RateLimiterTest, BucketCreatedAfterFirstRequest) {
    RateLimiter limiter(40s, 2, 60s, "token_bucket");

    auto t = RateLimiter::Clock::now();

    EXPECT_EQ(limiter.activeKeys(), 0);

    EXPECT_TRUE(limiter.allow("new_user", t));

    EXPECT_EQ(limiter.activeKeys(), 1)
        << "Bucket должен создаться после первого запроса пользователя";
}

TEST(RateLimiterTest, BucketRecreatedAfterClean) {
    RateLimiter limiter(40s, 2, 2s, "token_bucket");

    auto t = RateLimiter::Clock::now();

    EXPECT_TRUE(limiter.allow("temp_user", t));
    EXPECT_EQ(limiter.activeKeys(), 1);

    EXPECT_EQ(limiter.cleanup(t + 3s), 1);
    EXPECT_EQ(limiter.activeKeys(), 0)
        << "Старый bucket должен быть удалён";

    EXPECT_TRUE(limiter.allow("temp_user", t + 4s));
    EXPECT_EQ(limiter.activeKeys(), 1)
        << "После нового запроса bucket должен создаться заново";
}

TEST(RateLimiterTest, CustomCapacityWorks) {
    // capacity = max_requests = 5
    RateLimiter limiter(100s, 5, 60s, "token_bucket");

    auto t = RateLimiter::Clock::now();

    EXPECT_TRUE(limiter.allow("user1", t));
    EXPECT_TRUE(limiter.allow("user1", t));
    EXPECT_TRUE(limiter.allow("user1", t));
    EXPECT_TRUE(limiter.allow("user1", t));
    EXPECT_TRUE(limiter.allow("user1", t));

    EXPECT_FALSE(limiter.allow("user1", t))
        << "При capacity = 5 шестой запрос должен быть отклонён";
}

TEST(RateLimiterTest, FastRefillWorks) {
    RateLimiter limiter(1s, 1, 60s, "token_bucket");

    auto t = RateLimiter::Clock::now();

    EXPECT_TRUE(limiter.allow("user1", t));
    EXPECT_FALSE(limiter.allow("user1", t));

    EXPECT_TRUE(limiter.allow("user1", t + 1s))
        << "При refillRate = 1 токен в секунду токен должен восстановиться через 1 секунду";
}