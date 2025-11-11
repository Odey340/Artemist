#pragma once

#include "MarketDataReader.hpp"
#include "RollingStatistics.hpp"
#include "SignalGenerator.hpp"
#include <string>
#include <vector>
#include <fstream>

struct Trade {
    int64_t entryTime;
    int64_t exitTime;
    double entryPrice;
    double exitPrice;
    Signal direction;
    double pnl;
    int64_t duration;  // in microseconds
};

struct PerformanceMetrics {
    double totalReturn;
    double volatility;
    double sharpeRatio;
    double maxDrawdown;
    double winRate;
    double avgTradeLength;  // in seconds
    double ticksPerSecond;
    size_t totalTrades;
    size_t winningTrades;
    size_t totalTicks;
};

class Backtester {
public:
    Backtester(double commission = 2.10, double slippage = 1.0);  // slippage in ticks
    
    // Run backtest
    PerformanceMetrics run(const std::string& dataFile, double threshold = 2.5);
    
    // Get all trades
    const std::vector<Trade>& getTrades() const { return trades_; }
    
    // Get equity curve
    const std::vector<double>& getEquityCurve() const { return equityCurve_; }
    const std::vector<int64_t>& getEquityTimestamps() const { return equityTimestamps_; }
    
    // Write results to CSV
    void writeResults(const std::string& filename) const;

private:
    double commission_;
    double slippage_;  // in price units (1 tick = 0.25 for ES)
    const double tickSize_ = 0.25;  // ES futures tick size
    
    std::vector<Trade> trades_;
    std::vector<double> equityCurve_;
    std::vector<int64_t> equityTimestamps_;
    
    Signal currentPosition_;
    double entryPrice_;
    int64_t entryTime_;
    double equity_;
    double peakEquity_;
    double maxDrawdown_;
    
    // Trade execution
    double getFillPrice(double midPrice, Signal direction) const;
    void updatePosition(double price, int64_t timestamp, Signal signal);
    void closePosition(double price, int64_t timestamp);
    
    // Performance calculation
    PerformanceMetrics calculateMetrics(int64_t startTime, int64_t endTime, size_t tickCount) const;
};

