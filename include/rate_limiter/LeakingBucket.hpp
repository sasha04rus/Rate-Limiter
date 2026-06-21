#include <iostream>
#include <chrono>
#include <mutex>
#include <thread>
#include <iomanip>

class LeakingBucket {
private:
    double capacity;
    double leakRate;
    double counter;
    std::chrono::steady_clock::time_point lastLeakTime;
    std::mutex mtx;

    double getCurrentTimeSeconds() const {
        auto now = std::chrono::steady_clock::now();
        auto duration = now.time_since_epoch();
        return std::chrono::duration<double>(duration).count();
    }

public:
    LeakingBucket(double cap, double rate) 
        : capacity(cap), leakRate(rate), counter(0.0) {
        lastLeakTime = std::chrono::steady_clock::now();
    }

    bool allowRequest() {
        std::lock_guard<std::mutex> lock(mtx);
        
        auto now = std::chrono::steady_clock::now();
        
        std::chrono::duration<double> elapsed = now - lastLeakTime;
        double elapsedSeconds = elapsed.count();
        
        double leaks = elapsedSeconds * leakRate;
        
        counter = std::max(0.0, counter - leaks);
        
        lastLeakTime = now;
        
        if (counter + 1.0 <= capacity) {
            counter += 1.0;
            return true;
        } else {
            return false;
        }
    }

    double getCurrentLoad() {
        std::lock_guard<std::mutex> lock(mtx);
        
        auto now = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed = now - lastLeakTime;
        double elapsedSeconds = elapsed.count();
        
        double leaks = elapsedSeconds * leakRate;
        return std::max(0.0, counter - leaks);
    }

    void reset() {
        std::lock_guard<std::mutex> lock(mtx);
        counter = 0.0;
        lastLeakTime = std::chrono::steady_clock::now();
    }
};

