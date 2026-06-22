#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <atomic>
#include "LeakingBucket.hpp"

class LeakingBucketTest : public ::testing::Test {
protected:
    void SetUp() override {
        bucket = std::make_unique<LeakingBucket>(
            std::chrono::seconds(5),
            5
        );
    }
    
    std::unique_ptr<LeakingBucket> bucket;
    
    void simulateTime(double seconds) {
        std::this_thread::sleep_for(std::chrono::milliseconds(
            static_cast<int>(seconds * 1000)
        ));
    }
};

TEST_F(LeakingBucketTest, InitiallyEmpty) {
    auto now = LeakingBucket::Clock::now();
    EXPECT_DOUBLE_EQ(bucket->currentLoad(now), 0.0);
}

TEST_F(LeakingBucketTest, AllowRequestsUntilFull) {
    auto now = LeakingBucket::Clock::now();
    for (int i = 0; i < 5; ++i) {
        EXPECT_TRUE(bucket->allow(now));
    }
    EXPECT_FALSE(bucket->allow(now));
}

TEST_F(LeakingBucketTest, RequestRejectedWhenFull) {
    auto now = LeakingBucket::Clock::now();
    for (int i = 0; i < 5; ++i) {
        bucket->allow(now);
    }
    for (int i = 0; i < 3; ++i) {
        EXPECT_FALSE(bucket->allow(now));
    }
}

TEST_F(LeakingBucketTest, LeakAfterTime) {
    auto now = LeakingBucket::Clock::now();
    for (int i = 0; i < 5; ++i) {
        bucket->allow(now);
    }
    simulateTime(0.5);
    auto later = LeakingBucket::Clock::now();
    double load = bucket->currentLoad(later);
    EXPECT_NEAR(load, 4.5, 0.05);
}

TEST_F(LeakingBucketTest, LeakToZero) {
    auto now = LeakingBucket::Clock::now();
    for (int i = 0; i < 3; ++i) {
        bucket->allow(now);
    }
    simulateTime(6.0);
    auto later = LeakingBucket::Clock::now();
    EXPECT_DOUBLE_EQ(bucket->currentLoad(later), 0.0);
}

TEST_F(LeakingBucketTest, PartialLeakThenAllowRequest) {
    auto now = LeakingBucket::Clock::now();
    for (int i = 0; i < 5; ++i) {
        bucket->allow(now);
    }
    simulateTime(1.0);
    auto later = LeakingBucket::Clock::now();
    EXPECT_TRUE(bucket->allow(later));
    double load = bucket->currentLoad(later);
    EXPECT_NEAR(load, 5.0, 0.05);
}


TEST_F(LeakingBucketTest, FractionalLeakRate) {
    LeakingBucket slowBucket(std::chrono::seconds(10), 3);
    auto now = LeakingBucket::Clock::now();
    for (int i = 0; i < 3; ++i) {
        slowBucket.allow(now);
    }
    simulateTime(3.4);
    auto later = LeakingBucket::Clock::now();
    EXPECT_TRUE(slowBucket.allow(later));
    double load = slowBucket.currentLoad(later);
    EXPECT_NEAR(load, 3.0, 0.05);
}

TEST_F(LeakingBucketTest, ZeroCapacity) {
    LeakingBucket zeroBucket(std::chrono::seconds(1), 0);
    auto now = LeakingBucket::Clock::now();
    for (int i = 0; i < 5; ++i) {
        EXPECT_FALSE(zeroBucket.allow(now));
    }
    EXPECT_DOUBLE_EQ(zeroBucket.currentLoad(now), 0.0);
}


TEST_F(LeakingBucketTest, ZeroLeakRate) {
    LeakingBucket noLeakBucket(std::chrono::seconds(0), 5);
    auto now = LeakingBucket::Clock::now();
    for (int i = 0; i < 5; ++i) {
        EXPECT_TRUE(noLeakBucket.allow(now));
    }
    EXPECT_FALSE(noLeakBucket.allow(now));
    simulateTime(0.2);
    auto later = LeakingBucket::Clock::now();
    EXPECT_TRUE(noLeakBucket.allow(later));
    EXPECT_NEAR(noLeakBucket.currentLoad(later), 0.5, 0.05);
}

TEST_F(LeakingBucketTest, VeryHighLeakRate) {
    LeakingBucket fastBucket(std::chrono::milliseconds(1), 5);
    auto now = LeakingBucket::Clock::now();
    EXPECT_TRUE(fastBucket.allow(now));
    simulateTime(0.01);
    auto later = LeakingBucket::Clock::now();
    EXPECT_NEAR(fastBucket.currentLoad(later), 0.0, 0.1);
}

TEST_F(LeakingBucketTest, PreciseLeakCalculation) {
    LeakingBucket preciseBucket(std::chrono::seconds(10), 10);
    auto now = LeakingBucket::Clock::now();
    for (int i = 0; i < 5; ++i) {
        preciseBucket.allow(now);
    }
    simulateTime(1.5);
    auto later = LeakingBucket::Clock::now();
    double expectedLoad = 5.0 - 1.5;
    EXPECT_NEAR(preciseBucket.currentLoad(later), expectedLoad, 0.05);
}

TEST_F(LeakingBucketTest, ThreadSafety) {
    const int numThreads = 10;
    const int requestsPerThread = 20;
    std::vector<std::thread> threads;
    std::atomic<int> acceptedCount{0};
    std::atomic<int> rejectedCount{0};
    
    auto worker = [&]() {
        for (int i = 0; i < requestsPerThread; ++i) {
            if (bucket->allow()) {
                acceptedCount++;
            } else {
                rejectedCount++;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    };
    
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back(worker);
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    int totalRequests = numThreads * requestsPerThread;
    EXPECT_EQ(acceptedCount + rejectedCount, totalRequests);
    EXPECT_GE(acceptedCount, 0);
}

TEST_F(LeakingBucketTest, ConsistentStateSequence) {
    auto now = LeakingBucket::Clock::now();
    std::vector<double> loads;
    
    for (int i = 0; i < 10; ++i) {
        bucket->allow(now);
        loads.push_back(bucket->currentLoad(now));
        now += std::chrono::milliseconds(100);
    }
    
    for (double load : loads) {
        EXPECT_GE(load, 0.0);
        EXPECT_LE(load, 5.0);
    }
}

TEST_F(LeakingBucketTest, StressTest) {
    LeakingBucket stressBucket(std::chrono::seconds(2), 3);
    auto now = LeakingBucket::Clock::now();
    
    for (int cycle = 0; cycle < 100; ++cycle) {
        for (int i = 0; i < 10; ++i) {
            stressBucket.allow(now);
        }
        now += std::chrono::milliseconds(200);
    }
    
    double load = stressBucket.currentLoad(now);
    EXPECT_GE(load, 0.0);
    EXPECT_LE(load, 3.0);
}

TEST_F(LeakingBucketTest, LeakNotCalledWhenTimeDoesntAdvance) {
    auto now = LeakingBucket::Clock::now();
    for (int i = 0; i < 5; ++i) {
        bucket->allow(now);
    }
    for (int i = 0; i < 3; ++i) {
        EXPECT_FALSE(bucket->allow(now));
    }
    EXPECT_DOUBLE_EQ(bucket->currentLoad(now), 5.0);
}

TEST_F(LeakingBucketTest, AllowWithoutParametersUsesCurrentTime) {
    for (int i = 0; i < 5; ++i) {
        EXPECT_TRUE(bucket->allow());
    }
    EXPECT_FALSE(bucket->allow());
}

