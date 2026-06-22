#pragma once

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <mutex>

class LeakingBucket {
public:
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;
    using Duration = Clock::duration;

    LeakingBucket(Duration window, std::size_t max_requests)
        : capacity_(static_cast<double>(max_requests)),
          counter_(0.0),
          lastLeakTime_(Clock::now())
    {
        double windowSeconds = std::chrono::duration<double>(window).count();

        if (windowSeconds <= 0.0) {
            leakRate_ = capacity_;
        } else {
            leakRate_ = capacity_ / windowSeconds;
        }
    }

    bool allow(TimePoint now) {
        std::lock_guard<std::mutex> lock(mutex_);

        leak(now);

        if (counter_ + 1.0 <= capacity_) {
            counter_ += 1.0;
            return true;
        }

        return false;
    }

    bool allow() {
        return allow(Clock::now());
    }

    double currentLoad(TimePoint now) {
        std::lock_guard<std::mutex> lock(mutex_);

        leak(now);

        return counter_;
    }

private:
    void leak(TimePoint now) {
        if (now <= lastLeakTime_) {
            return;
        }

        double elapsedSeconds =
            std::chrono::duration<double>(now - lastLeakTime_).count();

        double leaked = elapsedSeconds * leakRate_;

        counter_ = std::max(0.0, counter_ - leaked);
        lastLeakTime_ = now;
    }

    double capacity_;
    double leakRate_;
    double counter_;

    TimePoint lastLeakTime_;

    std::mutex mutex_;
};