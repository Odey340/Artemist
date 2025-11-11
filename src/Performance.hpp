#pragma once

#include <chrono>
#include <cstdint>

class PerformanceMonitor {
public:
    PerformanceMonitor();
    
    void start();
    void stop();
    
    double getLatencyMicroseconds() const;
    double getLatencyNanoseconds() const;
    uint64_t getTickCount() const { return tickCount_; }
    
    void recordTick();
    void reset();

private:
    std::chrono::high_resolution_clock::time_point startTime_;
    std::chrono::high_resolution_clock::time_point endTime_;
    uint64_t tickCount_;
    bool running_;
};

