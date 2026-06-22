#include "../include/rate_limiter/TokenBucket.hpp"

#include <algorithm>

TokenBucket::TokenBucket(Duration window, std::size_t max_requests)
    : tokens_(static_cast<double>(max_requests)),
      capacity_(static_cast<double>(max_requests)),
      refill_rate_(
          static_cast<double>(max_requests) /
          std::chrono::duration<double>(window).count()
      ),
      last_refill_(Clock::now())
{}

bool TokenBucket::allow(TimePoint now) {
    double elapsed =
        std::chrono::duration<double>(now - last_refill_).count();

    double new_tokens = elapsed * refill_rate_;

    tokens_ = std::min(capacity_, tokens_ + new_tokens);
    last_refill_ = now;

    if (tokens_ >= 1.0) {
        tokens_ -= 1.0;
        return true;
    }

    return false;
}

std::size_t TokenBucket::cleanup(TimePoint) {
    return 0;
}

std::size_t TokenBucket::activeKeys() const {
    return 1;
}

std::size_t TokenBucket::evictionCount() const {
    return 0;
}

std::string TokenBucket::name() const {
    return "token_bucket";
}
