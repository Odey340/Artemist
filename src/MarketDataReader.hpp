#pragma once

#include <string>
#include <cstdint>
#include <memory>
#include <functional>

struct Tick {
    int64_t timestamp;  // microseconds since epoch
    double bid;
    double ask;
    int64_t volume;
    
    double mid() const { return (bid + ask) / 2.0; }
};

class MarketDataReader {
public:
    MarketDataReader(const std::string& filepath);
    ~MarketDataReader();
    
    // Non-copyable
    MarketDataReader(const MarketDataReader&) = delete;
    MarketDataReader& operator=(const MarketDataReader&) = delete;
    
    // Moveable
    MarketDataReader(MarketDataReader&&) noexcept;
    MarketDataReader& operator=(MarketDataReader&&) noexcept;
    
    // Read next tick, returns false if EOF
    bool next(Tick& tick);
    
    // Reset to beginning
    void reset();
    
    // Get total number of ticks (approximate, based on file size)
    size_t approximateTickCount() const;
    
    // Check if file is valid
    bool isValid() const { return data_ != nullptr && size_ > 0; }

private:
    void* data_;           // Memory-mapped data
    size_t size_;          // File size
    size_t position_;      // Current read position
    std::string filepath_;
    
    bool parseLine(const char* line, size_t len, Tick& tick);
};

