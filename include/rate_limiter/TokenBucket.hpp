#pragma once

#include "IRateLimiter.hpp"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>

struct Bucket {
    using Clock = std::chrono::steady_clock;

    double tokens;
    double capacity;
    double refillRate;

    std::mutex mutex;

    Clock::time_point lastRefill;
    Clock::time_point lastAccess;

    Bucket(double cap, double refill);
    Bucket(double cap, double refill, Clock::time_point now);
};

class RateLimiter final : public IRateLimiter {
public:
    using Clock = IRateLimiter::Clock;
    using TimePoint = IRateLimiter::TimePoint;

    using IRateLimiter::allow;
    using IRateLimiter::cleanup;

    RateLimiter(
        std::chrono::seconds ttl = std::chrono::seconds(60),
        std::chrono::seconds cleanInterval = std::chrono::seconds(10),
        double bucket_capacity = 2.0,
        double bucket_refill = 1.0 / 20.0
    );

    ~RateLimiter();

    bool allow(const std::string& userID, TimePoint now) override;

    std::size_t cleanup(TimePoint now) override;

    std::size_t activeKeys() const override;

    std::size_t evictionCount() const override {
        return evictionCount_.load();
    }

    std::string name() const override {
        return "token_bucket";
    }

    bool bucketExists(const std::string& userID) const;

private:
    void refill(Bucket& bucket, TimePoint now);
    void cleanLoop();

    std::chrono::seconds ttl_;
    std::chrono::seconds cleanInterval_;

    std::atomic<bool> run_{true};
    std::thread cleanThread_;

    double bucket_capacity_;
    double bucket_refill_;

    mutable std::mutex mapMutex;
    std::unordered_map<std::string, std::unique_ptr<Bucket>> buckets_;

    std::atomic<std::size_t> evictionCount_{0};
};