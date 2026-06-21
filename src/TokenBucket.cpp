#include "RateLimiter.hpp"
#include <thread>
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <memory>


Bucket::Bucket(double cap, double refill)
        : tokens(cap), capacity(cap), refillRate(refill), lastRefill(std::chrono::steady_clock::now()), lastAccess(std::chrono::steady_clock::now())
{}

RateLimiter::RateLimiter(std::chrono::seconds ttl, std::chrono::seconds cleanInterval, double bucket_capacity, double bucket_refill) 
        : ttl_(ttl), cleanInterval_(cleanInterval), bucket_capacity_(bucket_capacity), bucket_refill_(bucket_refill) {
        cleanThread_ = std::thread(&RateLimiter::cleanLoop, this);
}

RateLimiter::~RateLimiter() {
    run_ = false; 
    if (cleanThread_.joinable()) {
        cleanThread_.join(); 
    }
}

bool RateLimiter::allow (const std::string& userID) {
    auto now = std::chrono::steady_clock::now();
    Bucket* bucketPtr = nullptr;
    {
        std::lock_guard lock(mapMutex);
        auto it = buckets_.find(userID);
        if (it == buckets_.end()) {
            auto newBucket = std::make_unique<Bucket>(bucket_capacity_, bucket_refill_);
            it = buckets_.emplace(userID, std::move(newBucket)).first;
        }
        bucketPtr = it->second.get();
        bucketPtr->lastAccess = now;
    }
    std::lock_guard bucketLock(bucketPtr->mutex);
    refill(*bucketPtr);
    bool allowed = (bucketPtr->tokens >= 1.0);
    if (allowed) {
        bucketPtr->tokens -= 1.0;
    }
    return allowed;
}

bool RateLimiter::bucketExists(const std::string& userId) const {
    std::lock_guard lock(mapMutex);
    return (buckets_.find(userId) != buckets_.end());
}

void RateLimiter::refill(Bucket& bucket) {
    auto now = std::chrono::steady_clock::now();
    double interval = std::chrono::duration<double>(now - bucket.lastRefill).count();
    double add_tokens = interval * bucket.refillRate;

    if (bucket.tokens + add_tokens > bucket.capacity) {
        bucket.tokens = bucket.capacity;
    } else {
        bucket.tokens += add_tokens;
    };

    bucket.lastRefill = now;
}

void RateLimiter::cleanLoop() {
    while (run_) {
        std::this_thread::sleep_for(cleanInterval_);
        cleaner();
    }
}

void RateLimiter::cleaner() {
    auto now = std::chrono::steady_clock::now();
    std::lock_guard lock(mapMutex); 
    auto it = buckets_.begin();
    while (it != buckets_.end()) {
        auto age = std::chrono::duration_cast<std::chrono::seconds>(now - it->second->lastAccess);
        if (age > ttl_) {
            std::unique_lock bucketLock(it->second->mutex, std::try_to_lock);
            if (bucketLock.owns_lock()) {
                it = buckets_.erase(it);
                continue;
            } 
        }
        ++it;
    }
}
