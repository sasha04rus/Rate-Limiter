#pragma once

#include "IRateLimiter.hpp"

#include <chrono>
#include <cstddef>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>

template <typename LimiterType>
class RateLimiterManager : public IRateLimiter {
public:
    using Clock = IRateLimiter::Clock;
    using TimePoint = IRateLimiter::TimePoint;
    using Duration = IRateLimiter::Duration;

    using IRateLimiter::allow;
    using IRateLimiter::cleanup;

    RateLimiterManager(
        Duration window,
        std::size_t max_requests,
        Duration ttl = Duration::zero(),
        std::string name = "rate_limiter_manager"
    )
        : window_(window),
          max_requests_(max_requests),
          ttl_(ttl),
          name_(std::move(name))
    {}

    bool allow(const std::string& key, TimePoint now) override {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = states_.find(key);

        if (it == states_.end()) {
            KeyState state{
                std::make_unique<LimiterType>(window_, max_requests_),
                now
            };

            it = states_.emplace(key, std::move(state)).first;
        } else {
            it->second.last_access = now;
        }

        return it->second.limiter->allow(now);
    }

    std::size_t cleanup(TimePoint now) override {
        if (ttl_ == Duration::zero()) {
            return 0;
        }

        std::lock_guard<std::mutex> lock(mutex_);

        std::size_t removed = 0;

        auto it = states_.begin();

        while (it != states_.end()) {
            if (now - it->second.last_access > ttl_) {
                it = states_.erase(it);
                ++removed;
                ++eviction_count_;
            } else {
                ++it;
            }
        }

        return removed;
    }

    std::size_t activeKeys() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        return states_.size();
    }

    std::size_t evictionCount() const override {
        return eviction_count_;
    }

    std::string name() const override {
        return name_;
    }

private:
    struct KeyState {
        std::unique_ptr<LimiterType> limiter;
        TimePoint last_access;
    };

    Duration window_;
    std::size_t max_requests_;
    Duration ttl_;
    std::string name_;

    std::unordered_map<std::string, KeyState> states_;
    mutable std::mutex mutex_;

    std::size_t eviction_count_ = 0;
};