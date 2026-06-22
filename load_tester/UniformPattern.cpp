#include "TrafficPattern.hpp"

#include <algorithm>
#include <chrono>

class UniformPattern final : public TrafficPattern {
public:
    explicit UniformPattern(TrafficPatternConfig config) : TrafficPattern(config), current_(start_) {
        double safeRate = std::max(config_.ratePerSecond, 1.0);
        interval_ = std::chrono::nanoseconds(static_cast<long long>(1'000'000'000.0 / safeRate));
    }

    std::optional<TrafficRequest> next() override {
        if (expired(current_))
            return std::nullopt;

        TrafficRequest request{
            .time = current_,
            .key = key(requestIndex_++)
        };

        current_ += interval_;
        return request;
    }

    std::string name() const override { return "uniform"; }

private:
    Clock::time_point current_;
    std::chrono::nanoseconds interval_{};
    std::size_t requestIndex_ = 0;
};

std::unique_ptr<TrafficPattern> makeUniformPattern(TrafficPatternConfig config) {
    return std::make_unique<UniformPattern>(config);
}