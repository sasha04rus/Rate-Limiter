#pragma once


#include <chrono>
#include <cstdlib>
#include <deque>

class SlidingWindowLog {
public:
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;
    using Duration = Clock::duration;

private:
    std::deque<TimePoint> timestamps_;

    Duration window_duration_;
    std::size_t max_requests_;

public:
    SlidingWindowLog(Duration window, std::size_t max_requests) : window_duration_(window), max_requests_(max_requests) {}

    std::size_t getMaxRequests() const { return max_requests_; }
    void setMaxRequests(std::size_t new_max_requests) { max_requests_ = new_max_requests; }
    Duration getDuration() const { return window_duration_; }
    void setDuration(Duration new_window) { window_duration_ = new_window; }

    std::size_t usedRequests() const { return timestamps_.size(); }

public:
    bool allow() { return allow(Clock::now()); }

    bool allow(TimePoint now) {
        const auto cutoff = now - window_duration_;
        while (!timestamps_.empty() && timestamps_.front() <= cutoff) {
            timestamps_.pop_front();
        }

        if (timestamps_.size() < max_requests_) {
            timestamps_.push_back(now);
            return true;
        }
        return false;
    }
};