#pragma once

#include "RollingStatistics.hpp"

enum class Signal {
    FLAT = 0,
    LONG = 1,
    SHORT = -1
};

class SignalGenerator {
public:
    explicit SignalGenerator(double threshold = 2.5);
    
    // Generate signal based on current z-score
    Signal generate(double price, const RollingStatistics& stats);
    
    // Get current signal
    Signal currentSignal() const { return currentSignal_; }
    
    // Set threshold
    void setThreshold(double threshold) { threshold_ = threshold; }
    double getThreshold() const { return threshold_; }

private:
    double threshold_;
    Signal currentSignal_;
    double lastZScore_;
    
    bool shouldEnterLong(double zscore) const;
    bool shouldEnterShort(double zscore) const;
    bool shouldExit(double zscore) const;
};

