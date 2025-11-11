#include "SignalGenerator.hpp"
#include <cmath>

SignalGenerator::SignalGenerator(double threshold)
    : threshold_(threshold),
      currentSignal_(Signal::FLAT),
      lastZScore_(0.0) {
}

Signal SignalGenerator::generate(double price, const RollingStatistics& stats) {
    if (!stats.isReady()) {
        return Signal::FLAT;
    }
    
    double zscore = stats.zscore(price);
    lastZScore_ = zscore;
    
    // Current state machine logic:
    // - Enter LONG when z-score < -threshold and currently FLAT
    // - Enter SHORT when z-score > threshold and currently FLAT
    // - Exit when z-score crosses 0 (change sign)
    
    switch (currentSignal_) {
        case Signal::FLAT:
            if (zscore < -threshold_) {
                currentSignal_ = Signal::LONG;
            } else if (zscore > threshold_) {
                currentSignal_ = Signal::SHORT;
            }
            break;
            
        case Signal::LONG:
            if (zscore >= 0.0) {  // Crossed zero upward
                currentSignal_ = Signal::FLAT;
            }
            break;
            
        case Signal::SHORT:
            if (zscore <= 0.0) {  // Crossed zero downward
                currentSignal_ = Signal::FLAT;
            }
            break;
    }
    
    return currentSignal_;
}

bool SignalGenerator::shouldEnterLong(double zscore) const {
    return zscore < -threshold_;
}

bool SignalGenerator::shouldEnterShort(double zscore) const {
    return zscore > threshold_;
}

bool SignalGenerator::shouldExit(double zscore) const {
    return (currentSignal_ == Signal::LONG && zscore >= 0.0) ||
           (currentSignal_ == Signal::SHORT && zscore <= 0.0);
}

