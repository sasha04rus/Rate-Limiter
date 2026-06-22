#include "TokenBucket.hpp"

#include <algorithm>
#include <thread>

Bucket::Bucket(double cap, double refill)
    : Bucket(cap, refill, Clock::now()) {}

Bucket::Bucket(double cap, double refill, Clock::time_point now)
    : tokens(cap),
      capacity(cap),
      refillRate(refill),
      lastRefill(now),
      lastAccess(now) {}

RateLimiter::RateLimiter(
    std::chrono::seconds ttl,
    std::chrono::seconds cleanInterval,
    double bucket_capacity,
    double bucket_refill
)
    : ttl_(ttl),
      cleanInterval_(cleanInterval),
      bucket_capacity_(bucket_capacity),
      bucket_refill_(bucket_refill) {
    cleanThread_ = std::thread(&RateLimiter::cleanLoop, this);
}

RateLimiter::~RateLimiter() {
    run_ = false;

    if (cleanThread_.joinable()) {
        cleanThread_.join();
    }
}

bool RateLimiter::allow(const std::string& userID, TimePoint now) {
    std::lock_guard mapLock(mapMutex);

    auto it = buckets_.find(userID);

    if (it == buckets_.end()) {
        auto bucket = std::make_unique<Bucket>(
            bucket_capacity_,
            bucket_refill_,
            now
        );

        it = buckets_.emplace(userID, std::move(bucket)).first;
    }

    Bucket& bucket = *it->second;

    std::lock_guard bucketLock(bucket.mutex);

    refill(bucket, now);
    bucket.lastAccess = now;

    if (bucket.tokens >= 1.0) {
        bucket.tokens -= 1.0;
        return true;
    }

    return false;
}

void RateLimiter::refill(Bucket& bucket, TimePoint now) {
    if (now <= bucket.lastRefill) {
        return;
    }

    double interval =
        std::chrono::duration<double>(now - bucket.lastRefill).count();

    bucket.tokens = std::min(
        bucket.capacity,
        bucket.tokens + interval * bucket.refillRate
    );

    bucket.lastRefill = now;
}

std::size_t RateLimiter::cleanup(TimePoint now) {
    std::lock_guard mapLock(mapMutex);

    std::size_t removed = 0;

    auto it = buckets_.begin();

    while (it != buckets_.end()) {
        Bucket& bucket = *it->second;

        std::lock_guard bucketLock(bucket.mutex);

        if (now - bucket.lastAccess > ttl_) {
            it = buckets_.erase(it);
            ++removed;
            ++evictionCount_;
        } else {
            ++it;
        }
    }

    return removed;
}

void RateLimiter::cleanLoop() {
    while (run_) {
        std::this_thread::sleep_for(cleanInterval_);

        if (run_) {
            cleanup(Clock::now());
        }
    }
}

std::size_t RateLimiter::activeKeys() const {
    std::lock_guard lock(mapMutex);
    return buckets_.size();
}

bool RateLimiter::bucketExists(const std::string& userID) const {
    std::lock_guard lock(mapMutex);
    return buckets_.find(userID) != buckets_.end();
}