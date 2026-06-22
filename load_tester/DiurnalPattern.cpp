#include "TrafficPattern.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>

class DiurnalPattern final : public TrafficPattern {
public:
    explicit DiurnalPattern(TrafficPatternConfig config) : TrafficPattern(config), current_(start_) {}

    std::optional<TrafficRequest> next() override {
        if (expired(current_)) {
            return std::nullopt;
        }

        TrafficRequest request{
            .time = current_,
            .key = key(requestIndex_++)
        };

        current_ += intervalForCurrentTime();
        return request;
    }

    std::string name() const override { return "diurnal"; }

private:
    Clock::time_point current_;
    std::size_t requestIndex_ = 0;
    std::chrono::nanoseconds intervalForCurrentTime() const {
        constexpr double pi = 3.14159265358979323846;

        double elapsed = std::chrono::duration<double>(current_ - start_).count();
        double total = std::chrono::duration<double>(config_.duration).count();

        if (total <= 0.0)
            total = 1.0;

        double phase = elapsed / total;
        double multiplier = 0.8 + 0.5 * std::sin(2.0 * pi * phase - pi / 2.0);
        multiplier = std::clamp(multiplier, 0.3, 1.3);
        double rate = std::max(config_.ratePerSecond * multiplier, 1.0);
        return std::chrono::nanoseconds(static_cast<long long>(1'000'000'000.0 / rate));
    }
};

std::unique_ptr<TrafficPattern> makeDiurnalPattern(TrafficPatternConfig config) {
    return std::make_unique<DiurnalPattern>(config);
}