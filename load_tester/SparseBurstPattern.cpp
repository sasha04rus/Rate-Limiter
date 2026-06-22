#include "TrafficPattern.hpp"

#include <algorithm>
#include <chrono>

class SparseBurstPattern final : public TrafficPattern {
public:
    explicit SparseBurstPattern(TrafficPatternConfig config)
        : TrafficPattern(config),
          current_(start_),
          nextBurstTime_(start_ + config_.burstPeriod),
          remainingInBurst_(0) {
        double sparseRate = std::max(config_.ratePerSecond * 0.05, 1.0);

        sparseInterval_ = std::chrono::nanoseconds(
            static_cast<long long>(1'000'000'000.0 / sparseRate)
        );
    }

    std::optional<TrafficRequest> next() override {
        if (expired(current_))
            return std::nullopt;

        if (current_ >= nextBurstTime_ && remainingInBurst_ == 0) {
            remainingInBurst_ = config_.burstSize;
            burstTime_ = current_;
            nextBurstTime_ += config_.burstPeriod;
        }

        if (remainingInBurst_ > 0) {
            remainingInBurst_--;
            TrafficRequest request{
                .time = burstTime_,
                .key = key(requestIndex_++)
            };

            if (remainingInBurst_ == 0)
                current_ = burstTime_ + sparseInterval_;

            return request;
        }

        TrafficRequest request{
            .time = current_,
            .key = key(requestIndex_++)
        };

        current_ += sparseInterval_;
        return request;
    }

    std::string name() const override { return "sparse_burst"; }

private:
    Clock::time_point current_;
    Clock::time_point nextBurstTime_;
    Clock::time_point burstTime_;
    std::size_t remainingInBurst_;
    std::size_t requestIndex_ = 0;
    std::chrono::nanoseconds sparseInterval_{};
};

std::unique_ptr<TrafficPattern> makeSparseBurstPattern(TrafficPatternConfig config) {
    return std::make_unique<SparseBurstPattern>(config);
}