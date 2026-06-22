#pragma once

#include <chrono>
#include <cstdlib>
#include <deque>
#include <mutex>

class SlidingWindowLog {
public:
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;
    using Duration = Clock::duration;

private:
    std::deque<TimePoint> timestamps_;
    mutable std::mutex mutex_;
    Duration window_duration_;
    std::size_t max_requests_;

    TimePoint last_seen_time_{};

public:
    SlidingWindowLog(Duration window, std::size_t max_requests) : window_duration_(window), max_requests_(max_requests) {}

    std::size_t getMaxRequests() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return max_requests_;
    }

    Duration getDuration() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return window_duration_;
    }

    void setMaxRequests(std::size_t new_max_requests) {
        std::lock_guard<std::mutex> lock(mutex_);
        max_requests_ = new_max_requests;
    }

    void setDuration(Duration new_window) {
        std::lock_guard<std::mutex> lock(mutex_);
        window_duration_ = new_window;
    }

    std::size_t usedRequests() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return timestamps_.size();
    }

public:
    bool allow(TimePoint now) {
        std::lock_guard<std::mutex> lock(mutex_);
        return allowImpl(now);
    }

    bool allow() {
        return allow(Clock::now());
    }

private:
    bool allowImpl(TimePoint now) {
        if (now < last_seen_time_) now = last_seen_time_;
        last_seen_time_ = now;

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