#pragma once

#include <cstdint>
#include <atomic>
#include <cmath>
#include <cstring>
#ifdef __AVX2__
#include <immintrin.h>  // For SIMD
#endif

class RollingStatistics {
public:
    explicit RollingStatistics(size_t windowSize = 20000);
    ~RollingStatistics();
    
    // Non-copyable
    RollingStatistics(const RollingStatistics&) = delete;
    RollingStatistics& operator=(const RollingStatistics&) = delete;
    
    // Update with new value (lock-free, O(1))
    void update(double value);
    
    // Get current statistics
    double mean() const { return mean_; }
    double variance() const { return variance_; }
    double stddev() const { return std::sqrt(variance_); }
    double zscore(double value) const {
        double sd = stddev();
        return sd > 1e-10 ? (value - mean_) / sd : 0.0;
    }
    
    // Get number of values processed
    size_t count() const { return count_.load(std::memory_order_acquire); }
    
    // Check if enough data for valid statistics
    bool isReady() const { return count() >= windowSize_; }

private:
    const size_t windowSize_;
    double* buffer_;           // Ring buffer
    std::atomic<size_t> writeIndex_;
    std::atomic<size_t> count_;
    
    // EWMA parameters
    double alpha_;             // Decay factor
    double mean_;
    double variance_;
    double m2_;                // Second moment for variance calculation
    
    // Lock-free update
    void updateEWMA(double value);
};

