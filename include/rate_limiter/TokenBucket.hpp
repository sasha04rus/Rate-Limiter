#pragma once
#include <string>
#include <chrono>
#include <mutex>
#include <unordered_map>
#include <memory>
#include <thread>
#include <atomic>

struct Bucket {
    double tokens; double capacity; double refillRate;
    std::mutex mutex;
    std::chrono::steady_clock::time_point lastRefill;
    std::chrono::steady_clock::time_point lastAccess;
    Bucket(double cap, double refill);
};

class RateLimiter {

public:
    RateLimiter(std::chrono::seconds ttl = std::chrono::seconds(60), 
                std::chrono::seconds cleanInterval = std::chrono::seconds(10),
                double bucket_capacity = 2.0,
                double bucket_refill = 1.0 / 20.0);   
    ~RateLimiter();

    bool allow (const std::string& userID);
    bool bucketExists(const std::string& userId) const;
    
private:
    void refill(Bucket& bucket);
    void cleanLoop();
    void cleaner(); 
    
    std::chrono::seconds ttl_; std::chrono::seconds cleanInterval_; std::atomic<bool> run_{true}; std::thread cleanThread_;
    double bucket_capacity_; double bucket_refill_;
    mutable std::mutex mapMutex;
    std::unordered_map<std::string, std::unique_ptr<Bucket>> buckets_;
 
};

