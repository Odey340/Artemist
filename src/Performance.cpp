#include "Performance.hpp"

PerformanceMonitor::PerformanceMonitor()
    : tickCount_(0), running_(false) {
}

void PerformanceMonitor::start() {
    startTime_ = std::chrono::high_resolution_clock::now();
    running_ = true;
}

void PerformanceMonitor::stop() {
    if (running_) {
        endTime_ = std::chrono::high_resolution_clock::now();
        running_ = false;
    }
}

double PerformanceMonitor::getLatencyMicroseconds() const {
    if (!running_ && tickCount_ > 0) {
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime_ - startTime_);
        return static_cast<double>(duration.count()) / tickCount_;
    }
    return 0.0;
}

double PerformanceMonitor::getLatencyNanoseconds() const {
    if (!running_ && tickCount_ > 0) {
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(endTime_ - startTime_);
        return static_cast<double>(duration.count()) / tickCount_;
    }
    return 0.0;
}

void PerformanceMonitor::recordTick() {
    tickCount_++;
}

void PerformanceMonitor::reset() {
    tickCount_ = 0;
    running_ = false;
}

