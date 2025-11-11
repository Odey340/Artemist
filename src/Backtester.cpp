#include "Backtester.hpp"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <sstream>
#include <iomanip>
#include <stdexcept>

Backtester::Backtester(double commission, double slippage)
    : commission_(commission),
      slippage_(slippage * tickSize_),  // Convert ticks to price
      currentPosition_(Signal::FLAT),
      entryPrice_(0.0),
      entryTime_(0),
      equity_(100000.0),  // Starting capital
      peakEquity_(100000.0),
      maxDrawdown_(0.0) {
    equityCurve_.push_back(equity_);
    equityTimestamps_.push_back(0);
}

double Backtester::getFillPrice(double midPrice, Signal direction) const {
    double fillPrice = midPrice;
    
    // Apply slippage: 1 tick against us
    if (direction == Signal::LONG) {
        fillPrice += slippage_;  // Buy at ask (higher)
    } else if (direction == Signal::SHORT) {
        fillPrice -= slippage_;  // Sell at bid (lower)
    }
    
    return fillPrice;
}

void Backtester::updatePosition(double price, int64_t timestamp, Signal signal) {
    if (signal == currentPosition_) {
        return;  // No change
    }
    
    // Close existing position if any
    if (currentPosition_ != Signal::FLAT) {
        closePosition(price, timestamp);
    }
    
    // Open new position if needed
    if (signal != Signal::FLAT) {
        double fillPrice = getFillPrice(price, signal);
        currentPosition_ = signal;
        entryPrice_ = fillPrice;
        entryTime_ = timestamp;
        
        // Deduct commission on entry
        equity_ -= commission_;
    }
    
    // Update equity curve
    equityCurve_.push_back(equity_);
    equityTimestamps_.push_back(timestamp);
    
    if (equity_ > peakEquity_) {
        peakEquity_ = equity_;
    }
    
    double drawdown = (peakEquity_ - equity_) / peakEquity_;
    if (drawdown > maxDrawdown_) {
        maxDrawdown_ = drawdown;
    }
}

void Backtester::closePosition(double price, int64_t timestamp) {
    if (currentPosition_ == Signal::FLAT) {
        return;
    }
    
    Signal exitDirection = (currentPosition_ == Signal::LONG) ? Signal::SHORT : Signal::LONG;
    double fillPrice = getFillPrice(price, exitDirection);
    
    double pnl = 0.0;
    if (currentPosition_ == Signal::LONG) {
        pnl = (fillPrice - entryPrice_) * 50;  // ES multiplier is 50
    } else {
        pnl = (entryPrice_ - fillPrice) * 50;
    }
    
    pnl -= commission_;  // Exit commission
    
    equity_ += pnl;
    
    Trade trade;
    trade.entryTime = entryTime_;
    trade.exitTime = timestamp;
    trade.entryPrice = entryPrice_;
    trade.exitPrice = fillPrice;
    trade.direction = currentPosition_;
    trade.pnl = pnl;
    trade.duration = timestamp - entryTime_;
    
    trades_.push_back(trade);
    
    currentPosition_ = Signal::FLAT;
}

PerformanceMetrics Backtester::calculateMetrics(int64_t startTime, int64_t endTime, size_t tickCount) const {
    PerformanceMetrics metrics;
    metrics.totalTicks = tickCount;
    
    if (trades_.empty()) {
        metrics.totalReturn = 0.0;
        metrics.volatility = 0.0;
        metrics.sharpeRatio = 0.0;
        metrics.maxDrawdown = maxDrawdown_;
        metrics.winRate = 0.0;
        metrics.avgTradeLength = 0.0;
        metrics.totalTrades = 0;
        metrics.winningTrades = 0;
        
        if (endTime > startTime) {
            double seconds = (endTime - startTime) / 1e6;
            metrics.ticksPerSecond = seconds > 0 ? tickCount / seconds : 0.0;
        } else {
            metrics.ticksPerSecond = 0.0;
        }
        return metrics;
    }
    
    // Calculate returns
    double initialEquity = 100000.0;
    double finalEquity = equity_;
    metrics.totalReturn = (finalEquity - initialEquity) / initialEquity;
    
    // Calculate trade statistics
    metrics.totalTrades = trades_.size();
    metrics.winningTrades = std::count_if(trades_.begin(), trades_.end(),
                                         [](const Trade& t) { return t.pnl > 0.0; });
    metrics.winRate = metrics.totalTrades > 0 ? 
                      static_cast<double>(metrics.winningTrades) / metrics.totalTrades : 0.0;
    
    // Average trade length
    double totalDuration = std::accumulate(trades_.begin(), trades_.end(), 0.0,
                                          [](double sum, const Trade& t) {
                                              return sum + t.duration;
                                          });
    metrics.avgTradeLength = metrics.totalTrades > 0 ?
                            (totalDuration / metrics.totalTrades) / 1e6 : 0.0;  // Convert to seconds
    
    // Calculate volatility from equity curve returns
    if (equityCurve_.size() > 1) {
        std::vector<double> returns;
        for (size_t i = 1; i < equityCurve_.size(); ++i) {
            if (equityCurve_[i-1] > 0) {
                returns.push_back((equityCurve_[i] - equityCurve_[i-1]) / equityCurve_[i-1]);
            }
        }
        
        if (!returns.empty()) {
            double meanReturn = std::accumulate(returns.begin(), returns.end(), 0.0) / returns.size();
            double variance = 0.0;
            for (double r : returns) {
                variance += (r - meanReturn) * (r - meanReturn);
            }
            variance /= returns.size();
            metrics.volatility = std::sqrt(variance) * std::sqrt(252 * 24 * 60 * 60);  // Annualized (assuming 1-second bars)
        } else {
            metrics.volatility = 0.0;
        }
    } else {
        metrics.volatility = 0.0;
    }
    
    // Sharpe ratio (annualized)
    if (metrics.volatility > 1e-10) {
        metrics.sharpeRatio = metrics.totalReturn / metrics.volatility * std::sqrt(252.0);
    } else {
        metrics.sharpeRatio = 0.0;
    }
    
    metrics.maxDrawdown = maxDrawdown_;
    
    // Ticks per second
    if (endTime > startTime) {
        double seconds = (endTime - startTime) / 1e6;
        metrics.ticksPerSecond = seconds > 0 ? tickCount / seconds : 0.0;
    } else {
        metrics.ticksPerSecond = 0.0;
    }
    
    return metrics;
}

PerformanceMetrics Backtester::run(const std::string& dataFile, double threshold) {
    MarketDataReader reader(dataFile);
    if (!reader.isValid()) {
        throw std::runtime_error("Failed to open data file: " + dataFile);
    }
    
    RollingStatistics stats(20000);
    SignalGenerator signalGen(threshold);
    
    trades_.clear();
    equityCurve_.clear();
    equityTimestamps_.clear();
    equity_ = 100000.0;
    peakEquity_ = 100000.0;
    maxDrawdown_ = 0.0;
    currentPosition_ = Signal::FLAT;
    
    equityCurve_.push_back(equity_);
    equityTimestamps_.push_back(0);
    
    Tick tick;
    Tick lastTick;
    int64_t startTime = 0;
    int64_t endTime = 0;
    size_t tickCount = 0;
    
    while (reader.next(tick)) {
        if (startTime == 0) {
            startTime = tick.timestamp;
        }
        endTime = tick.timestamp;
        tickCount++;
        lastTick = tick;  // Keep track of last tick
        
        double midPrice = tick.mid();
        stats.update(midPrice);
        Signal signal = signalGen.generate(midPrice, stats);
        
        updatePosition(midPrice, tick.timestamp, signal);
    }
    
    // Close any open position at the end
    if (currentPosition_ != Signal::FLAT && tickCount > 0) {
        closePosition(lastTick.mid(), lastTick.timestamp);
    }
    
    return calculateMetrics(startTime, endTime, tickCount);
}

void Backtester::writeResults(const std::string& filename) const {
    std::ofstream out(filename);
    if (!out.is_open()) {
        throw std::runtime_error("Failed to open output file: " + filename);
    }
    
    // Write header
    out << "timestamp,equity\n";
    for (size_t i = 0; i < equityCurve_.size(); ++i) {
        out << equityTimestamps_[i] << "," 
            << std::fixed << std::setprecision(2) << equityCurve_[i] << "\n";
    }
    
    out.close();
    
    // Write trades to separate file
    std::string tradesFile = filename;
    size_t dotPos = tradesFile.find_last_of('.');
    if (dotPos != std::string::npos) {
        tradesFile = tradesFile.substr(0, dotPos) + "_trades.csv";
    } else {
        tradesFile += "_trades.csv";
    }
    
    std::ofstream tradesOut(tradesFile);
    if (tradesOut.is_open()) {
        tradesOut << "entry_time,exit_time,entry_price,exit_price,direction,pnl,duration_us\n";
        for (const auto& trade : trades_) {
            tradesOut << trade.entryTime << ","
                     << trade.exitTime << ","
                     << std::fixed << std::setprecision(2) << trade.entryPrice << ","
                     << trade.exitPrice << ","
                     << (trade.direction == Signal::LONG ? "LONG" : "SHORT") << ","
                     << trade.pnl << ","
                     << trade.duration << "\n";
        }
        tradesOut.close();
    }
}

