#pragma once

#include <chrono>
#include <cstddef>
#include <mutex>
#include <algorithm>

class TokenBucket {
public:
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;
    using Duration = Clock::duration;

    TokenBucket(Duration window, std::size_t max_requests);

    bool allow(TimePoint now);

    bool allow();

    std::size_t getMaxRequests() const;
    Duration getDuration() const;

    void setMaxRequests(std::size_t new_max_requests);
    void setDuration(Duration new_window);

    double availableTokens() const;

private:
    void refill(TimePoint now);

    mutable std::mutex mutex_;

    double tokens_;
    double capacity_;
    double refill_rate_;

    Duration window_;
    std::size_t max_requests_;

    TimePoint last_refill_;
    TimePoint last_seen_time_{};
};
