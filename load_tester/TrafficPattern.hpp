#pragma once

#include <chrono>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>

struct TrafficRequest {
    using Clock = std::chrono::steady_clock;
    Clock::time_point time;
    std::string key;
};

struct TrafficPatternConfig {
    std::size_t clients = 100;
    double ratePerSecond = 1000.0;
    std::chrono::seconds duration{10};
    std::size_t burstSize = 200;
    std::chrono::milliseconds burstPeriod{1000};
    unsigned seed = 42;
};

class TrafficPattern {
public:
    using Clock = TrafficRequest::Clock;

    explicit TrafficPattern(TrafficPatternConfig config) : config_(config), start_(Clock::now()) {}

    virtual ~TrafficPattern() = default;

    virtual std::optional<TrafficRequest> next() = 0;
    virtual std::string name() const = 0;

protected:
    TrafficPatternConfig config_;
    Clock::time_point start_;

    bool expired(Clock::time_point t) const {
        return t >= start_ + config_.duration;
    }

    std::string key(std::size_t index) const {
        if (config_.clients == 0)
            return "client_0";
        return "client_" + std::to_string(index % config_.clients);
    }
};

std::unique_ptr<TrafficPattern> makeUniformPattern(TrafficPatternConfig config);
std::unique_ptr<TrafficPattern> makeBurstPattern(TrafficPatternConfig config);
std::unique_ptr<TrafficPattern> makeDiurnalPattern(TrafficPatternConfig config);
std::unique_ptr<TrafficPattern> makeSparseBurstPattern(TrafficPatternConfig config);
std::unique_ptr<TrafficPattern> makeZombiePattern(TrafficPatternConfig config);