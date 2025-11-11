#include "RollingStatistics.hpp"
#include <cstdlib>
#include <algorithm>
#include <cstring>
#ifdef __AVX2__
#include <immintrin.h>
#endif
#ifdef _WIN32
#include <malloc.h>  // For _aligned_malloc
#elif defined(__APPLE__)
#include <stdlib.h>  // For posix_memalign
#endif

RollingStatistics::RollingStatistics(size_t windowSize)
    : windowSize_(windowSize),
      buffer_(nullptr),
      writeIndex_(0),
      count_(0),
      alpha_(2.0 / (windowSize + 1.0)),  // EWMA decay factor
      mean_(0.0),
      variance_(0.0),
      m2_(0.0) {
    
    // Align buffer for SIMD
    size_t alignedSize = (windowSize + 7) & ~7;  // Align to 8
    size_t totalSize = alignedSize * sizeof(double);
    
#ifdef _WIN32
    // Windows: use _aligned_malloc
    buffer_ = static_cast<double*>(_aligned_malloc(totalSize, 64));
#else
    // POSIX: use aligned_alloc (C11/C++17)
    #ifdef __APPLE__
        // macOS doesn't support aligned_alloc, use posix_memalign
        void* ptr = nullptr;
        if (posix_memalign(&ptr, 64, totalSize) == 0) {
            buffer_ = static_cast<double*>(ptr);
        } else {
            buffer_ = nullptr;
        }
    #else
        buffer_ = static_cast<double*>(std::aligned_alloc(64, totalSize));
    #endif
#endif
    
    if (buffer_) {
        std::memset(buffer_, 0, totalSize);
    }
}

RollingStatistics::~RollingStatistics() {
    if (buffer_) {
#ifdef _WIN32
        _aligned_free(buffer_);
#else
        std::free(buffer_);
#endif
        buffer_ = nullptr;
    }
}

void RollingStatistics::update(double value) {
    size_t idx = writeIndex_.fetch_add(1, std::memory_order_acq_rel) % windowSize_;
    size_t oldCount = count_.fetch_add(1, std::memory_order_acq_rel);
    
    double oldValue = buffer_[idx];
    buffer_[idx] = value;
    
    if (oldCount < windowSize_) {
        // Initial fill phase
        if (oldCount == 0) {
            mean_ = value;
            variance_ = 0.0;
            m2_ = 0.0;
        } else {
            // Welford's online algorithm for initial variance
            double delta = value - mean_;
            mean_ += delta / (oldCount + 1);
            double delta2 = value - mean_;
            m2_ += delta * delta2;
            variance_ = m2_ / (oldCount + 1);
        }
    } else {
        // Rolling window: update EWMA
        updateEWMA(value);
    }
}

void RollingStatistics::updateEWMA(double value) {
    // EWMA update for mean
    double oldMean = mean_;
    mean_ = alpha_ * value + (1.0 - alpha_) * mean_;
    
    // Update variance using exponential weighted variance
    // Simplified approach: update based on new and old means
    double delta = value - oldMean;
    double deltaMean = mean_ - oldMean;
    
    // Exponential weighted variance approximation
    variance_ = (1.0 - alpha_) * (variance_ + alpha_ * delta * delta);
    
    // Ensure variance is non-negative
    if (variance_ < 0.0) {
        variance_ = 0.0;
    }
}

