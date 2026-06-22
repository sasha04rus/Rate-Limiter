#include "TrafficPattern.hpp"

class BurstPattern final : public TrafficPattern {
public:
    explicit BurstPattern(TrafficPatternConfig config)
        : TrafficPattern(config),
          currentBurstTime_(start_),
          remainingInBurst_(config_.burstSize) {}

    std::optional<TrafficRequest> next() override {
        if (remainingInBurst_ == 0) {
            currentBurstTime_ += config_.burstPeriod;
            remainingInBurst_ = config_.burstSize;
        }

        if (expired(currentBurstTime_))
            return std::nullopt;
        remainingInBurst_--;
        return TrafficRequest{
            .time = currentBurstTime_,
            .key = key(requestIndex_++)
        };
    }

    std::string name() const override { return "burst"; }

private:
    Clock::time_point currentBurstTime_;
    std::size_t remainingInBurst_;
    std::size_t requestIndex_ = 0;
};

std::unique_ptr<TrafficPattern> makeBurstPattern(TrafficPatternConfig config) {
    return std::make_unique<BurstPattern>(config);
}