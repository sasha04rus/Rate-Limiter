#include "TokenBucket.hpp"

TokenBucket::TokenBucket(Duration window, std::size_t max_requests)
    : tokens_(static_cast<double>(max_requests)),
      capacity_(static_cast<double>(max_requests)),
      refill_rate_(
          static_cast<double>(max_requests) /
          std::chrono::duration<double>(window).count()
      ),
      window_(window),
      max_requests_(max_requests),
      last_refill_(Clock::now())
{}

bool TokenBucket::allow(TimePoint now) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (now < last_seen_time_) {
        now = last_seen_time_;
    }

    last_seen_time_ = now;

    refill(now);

    if (tokens_ >= 1.0) {
        tokens_ -= 1.0;
        return true;
    }

    return false;
}

bool TokenBucket::allow() {
    return allow(Clock::now());
}

void TokenBucket::refill(TimePoint now) {
    double elapsed =
        std::chrono::duration<double>(now - last_refill_).count();

    double added_tokens = elapsed * refill_rate_;

    tokens_ = std::min(capacity_, tokens_ + added_tokens);

    last_refill_ = now;
}

std::size_t TokenBucket::getMaxRequests() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return max_requests_;
}

TokenBucket::Duration TokenBucket::getDuration() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return window_;
}

void TokenBucket::setMaxRequests(std::size_t new_max_requests) {
    std::lock_guard<std::mutex> lock(mutex_);

    max_requests_ = new_max_requests;
    capacity_ = static_cast<double>(new_max_requests);

    if (tokens_ > capacity_) {
        tokens_ = capacity_;
    }

    refill_rate_ =
        static_cast<double>(max_requests_) /
        std::chrono::duration<double>(window_).count();
}

void TokenBucket::setDuration(Duration new_window) {
    std::lock_guard<std::mutex> lock(mutex_);

    window_ = new_window;

    refill_rate_ =
        static_cast<double>(max_requests_) /
        std::chrono::duration<double>(window_).count();
}

double TokenBucket::availableTokens() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return tokens_;
}
