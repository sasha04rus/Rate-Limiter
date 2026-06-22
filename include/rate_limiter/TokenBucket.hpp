#pragma once

#include "IRateLimiter.hpp"

#include <chrono>
#include <cstddef>
#include <string>

class TokenBucket : public IRateLimiter {
public:
    using Clock = IRateLimiter::Clock;
    using TimePoint = IRateLimiter::TimePoint;
    using Duration = IRateLimiter::Duration;

    using IRateLimiter::allow;
    using IRateLimiter::cleanup;

    TokenBucket(Duration window, std::size_t max_requests);

    bool allow(TimePoint now) override;

    std::size_t cleanup(TimePoint now) override;

    std::size_t activeKeys() const override;

    std::size_t evictionCount() const override;

    std::string name() const override;

private:
    double tokens_;
    double capacity_;
    double refill_rate_;

    TimePoint last_refill_;
};
