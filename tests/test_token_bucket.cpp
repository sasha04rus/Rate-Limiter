#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <atomic>
#include "../include/rate_limiter/TokenBucket.hpp" 


TEST(RateLimiterTest, RateAccuracy) {
    RateLimiter limiter(std::chrono::seconds(60), std::chrono::seconds(10), 2.0, 1.0 / 20.0);
    EXPECT_TRUE(limiter.allow("user1"))
    << "Первый запрос должен пройти";
    EXPECT_TRUE(limiter.allow("user1"))
    << "Второй запрос должен пройти";
    EXPECT_FALSE(limiter.allow("user1"))
    << "Лимит исчерпан";
    std::this_thread::sleep_for(std::chrono::seconds(20));
    EXPECT_TRUE(limiter.allow("user1"))
    << "Один токен должен быть восстановлен";
    EXPECT_FALSE(limiter.allow("user1"))
    << "Второго токена сразу нет, только через 20 секунд";
};

TEST(RateLimiterTest, BurstCapasity) {
    RateLimiter limiter(std::chrono::seconds(60), std::chrono::seconds(10), 2.0, 1.0 / 20.0);
    std::vector<bool> results;
    for (int i = 0; i < 10; i++) {
        results.push_back(limiter.allow("burstUser"));
    }
    EXPECT_EQ(results[0], true);
    EXPECT_EQ(results[1], true);
    for (size_t i = 2; i < results.size(); i++) {
        EXPECT_FALSE(results[i])
        << "Ожидаем: results = [true, true, false, false, false, false, false, false, false, false]";
    }
};


TEST(RateLimiterTest, ThreadSafety) {
    RateLimiter limiter(std::chrono::seconds(60), std::chrono::seconds(10), 2.0, 1.0 / 20.0);
    constexpr int THREADS = 20;
    std::atomic<int> allowed{0};
    auto worker = [&]() {
        if (limiter.allow("the_same_user")) {
            allowed++;
        }
    };
    std::vector<std::thread> threads;
    for (int i = 0; i < THREADS; i++) {
        threads.emplace_back(worker);
    }
    for (auto& t : threads)
        t.join();
    EXPECT_LE(allowed.load(), 2);
}

TEST(RateLimiterTest, CleanOldBucket) {
    RateLimiter limiter(std::chrono::seconds(3), std::chrono::seconds(1), 2.0, 1.0 / 20.0);
    limiter.allow("old_user");
    std::this_thread::sleep_for(std::chrono::seconds(2));
    limiter.allow("new_user");
    EXPECT_TRUE(limiter.bucketExists("old_user"));
    EXPECT_TRUE(limiter.bucketExists("new_user"));
    std::this_thread::sleep_for(std::chrono::seconds(2));
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_FALSE(limiter.bucketExists("old_user"));
    EXPECT_TRUE(limiter.bucketExists("new_user"));
}

TEST(RateLimiterTest, DifferentUsersHaveDifferentBuckets) {
    RateLimiter limiter(std::chrono::seconds(60), std::chrono::seconds(10), 2.0, 1.0 / 20.0);

    EXPECT_TRUE(limiter.allow("user1"));
    EXPECT_TRUE(limiter.allow("user1"));
    EXPECT_FALSE(limiter.allow("user1"));

    EXPECT_TRUE(limiter.allow("user2"))
    << "У другого пользователя должен быть свой отдельный bucket";

    EXPECT_TRUE(limiter.allow("user2"));
    EXPECT_FALSE(limiter.allow("user2"));
}

TEST(RateLimiterTest, TokenRefillIsPartial) {
    RateLimiter limiter(std::chrono::seconds(60), std::chrono::seconds(10), 2.0, 1.0 / 20.0);

    EXPECT_TRUE(limiter.allow("user1"));
    EXPECT_TRUE(limiter.allow("user1"));
    EXPECT_FALSE(limiter.allow("user1"));

    std::this_thread::sleep_for(std::chrono::seconds(10));

    EXPECT_FALSE(limiter.allow("user1"))
    << "За 10 секунд восстановится только 0.5 токена, этого недостаточно";
}

TEST(RateLimiterTest, TokenRefillAfterEnoughTime) {
    RateLimiter limiter(std::chrono::seconds(60), std::chrono::seconds(10), 2.0, 1.0 / 20.0);

    EXPECT_TRUE(limiter.allow("user1"));
    EXPECT_TRUE(limiter.allow("user1"));
    EXPECT_FALSE(limiter.allow("user1"));

    std::this_thread::sleep_for(std::chrono::seconds(21));

    EXPECT_TRUE(limiter.allow("user1"))
    << "Через 20+ секунд должен восстановиться один токен";
}

TEST(RateLimiterTest, CapacityDoesNotGrowAboveLimit) {
    RateLimiter limiter(std::chrono::seconds(60), std::chrono::seconds(10), 2.0, 1.0 / 20.0);

    std::this_thread::sleep_for(std::chrono::seconds(45));

    EXPECT_TRUE(limiter.allow("user1"));
    EXPECT_TRUE(limiter.allow("user1"));
    EXPECT_FALSE(limiter.allow("user1"))
    << "Даже после долгого ожидания токенов не должно быть больше capacity";
}

TEST(RateLimiterTest, BucketCreatedAfterFirstRequest) {
    RateLimiter limiter(std::chrono::seconds(60), std::chrono::seconds(10), 2.0, 1.0 / 20.0);

    EXPECT_FALSE(limiter.bucketExists("new_user"));

    limiter.allow("new_user");

    EXPECT_TRUE(limiter.bucketExists("new_user"))
    << "Bucket должен создаться после первого запроса пользователя";
}

TEST(RateLimiterTest, BucketRecreatedAfterClean) {
    RateLimiter limiter(std::chrono::seconds(2), std::chrono::seconds(1), 2.0, 1.0 / 20.0);

    limiter.allow("temp_user");
    EXPECT_TRUE(limiter.bucketExists("temp_user"));

    std::this_thread::sleep_for(std::chrono::seconds(4));

    EXPECT_FALSE(limiter.bucketExists("temp_user"))
    << "Старый bucket должен быть удалён";

    EXPECT_TRUE(limiter.allow("temp_user"));
    EXPECT_TRUE(limiter.bucketExists("temp_user"))
    << "После нового запроса bucket должен создаться заново";
}

TEST(RateLimiterTest, CustomCapacityWorks) {
    RateLimiter limiter(std::chrono::seconds(60), std::chrono::seconds(10), 5.0, 1.0 / 20.0);

    EXPECT_TRUE(limiter.allow("user1"));
    EXPECT_TRUE(limiter.allow("user1"));
    EXPECT_TRUE(limiter.allow("user1"));
    EXPECT_TRUE(limiter.allow("user1"));
    EXPECT_TRUE(limiter.allow("user1"));

    EXPECT_FALSE(limiter.allow("user1"))
    << "При capacity = 5 шестой запрос должен быть отклонён";
}

TEST(RateLimiterTest, FastRefillWorks) {
    RateLimiter limiter(std::chrono::seconds(60), std::chrono::seconds(10), 1.0, 1.0);

    EXPECT_TRUE(limiter.allow("user1"));
    EXPECT_FALSE(limiter.allow("user1"));

    std::this_thread::sleep_for(std::chrono::seconds(1));

    EXPECT_TRUE(limiter.allow("user1"))
    << "При refillRate = 1 токен в секунду токен должен восстановиться через 1 секунду";
}
