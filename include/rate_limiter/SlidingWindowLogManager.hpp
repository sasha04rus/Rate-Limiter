#pragma once

#include <chrono>
#include <string>
#include <unordered_map>
#include <mutex>

#include "sliding_window_log.h"


class SlidingWindowLogManager/* : public IRateLimiter*/ {
public:
    using Clock = std::chrono::steady_clock;
    using Duration = Clock::duration;

    SlidingWindowLogManager(Duration window, size_t max_requests)
        : window_duration_(window), max_requests_(max_requests) {}

    
    bool allow(const std::string& key) /*override*/ {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = limiters_.find(key);
        if (it == limiters_.end()) {
            auto [new_it, _] = limiters_.emplace(key, SlidingWindowLog(window_duration_, max_requests_));
            it = new_it;
        }
        
        return it->second.allow();
    }

private:
    Duration window_duration_;
    size_t max_requests_;
    std::unordered_map<std::string, SlidingWindowLog> limiters_;
    mutable std::mutex mutex_;
};