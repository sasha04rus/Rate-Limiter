#pragma once

#include <chrono>
#include <cstddef>
#include <string>

class IRateLimiter {
public:
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;
    using Duration = Clock::duration;

    virtual ~IRateLimiter() = default;
    bool allow(const std::string& key) {
        return allow(key, Clock::now());
    }

    virtual bool allow(const std::string& key, TimePoint now) = 0;
    virtual std::size_t cleanup(TimePoint now) {
        return 0;
    }

    std::size_t cleanup() {
        return cleanup(Clock::now());
    }

    virtual std::size_t activeKeys() const {
        return 0;
    }

    virtual std::size_t evictionCount() const {
        return 0;
    }

    virtual std::string name() const = 0;
};