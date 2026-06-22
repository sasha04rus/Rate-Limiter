#pragma once

#include <chrono>
#include <cstddef>
#include <string>

class IRateLimiter {
public:
    using Clock = std::chrono::steady_clock;

    virtual ~IRateLimiter() = default;
    virtual bool allow(const std::string& key, Clock::time_point now) = 0;
    virtual std::string name() const = 0;
    virtual std::size_t keyCount() const { return 0; }
    virtual std::size_t evictionCount() const { return 0; }
};