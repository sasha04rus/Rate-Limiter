#include "TrafficPattern.hpp"

#include <algorithm>
#include <chrono>

class ZombiePattern final : public TrafficPattern {
public:
    explicit ZombiePattern(TrafficPatternConfig config) : TrafficPattern(config), current_(start_) {
        double safeRate = std::max(config_.ratePerSecond, 1.0);
        interval_ = std::chrono::nanoseconds(static_cast<long long>(1'000'000'000.0 / safeRate));
    }

    std::optional<TrafficRequest> next() override {
        if (expired(current_)) {
            return std::nullopt;
        }

        TrafficRequest request{
            .time = current_,
            .key = "zombie_" + std::to_string(requestIndex_)
        };

        requestIndex_++;
        current_ += interval_;
        return request;
    }

    std::string name() const override { return "zombie"; }

private:
    Clock::time_point current_;
    std::chrono::nanoseconds interval_{};
    std::size_t requestIndex_ = 0;
};

std::unique_ptr<TrafficPattern> makeZombiePattern(TrafficPatternConfig config) {
    return std::make_unique<ZombiePattern>(config);
}