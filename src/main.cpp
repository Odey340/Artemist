#include "Backtester.hpp"
#include "Performance.hpp"
#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <thread>
#include <atomic>
#include <iostream>
#include <string>
#include <chrono>
#include <iomanip>
#include <stdexcept>

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

void setThreadAffinity(int core) {
#ifdef _WIN32
    SetThreadAffinityMask(GetCurrentThread(), 1ULL << core);
#else
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core, &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
#endif
}

int main(int argc, char* argv[]) {
    // Initialize spdlog async logger
    try {
        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("artemis.log", true);
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        
        std::vector<spdlog::sink_ptr> sinks{file_sink, console_sink};
        auto logger = std::make_shared<spdlog::logger>("artemis", sinks.begin(), sinks.end());
        
        spdlog::set_default_logger(logger);
        spdlog::set_level(spdlog::level::info);
        spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
    } catch (const spdlog::spdlog_ex& ex) {
        std::cerr << "Log initialization failed: " << ex.what() << std::endl;
        return 1;
    }
    
    // Parse command line arguments
    std::string dataFile = "data/ES_futures_sample.csv";
    double threshold = 2.5;
    
    if (argc > 1) {
        dataFile = argv[1];
    }
    if (argc > 2) {
        threshold = std::stod(argv[2]);
    }
    
    spdlog::info("Starting Artemis backtester");
    spdlog::info("Data file: {}", dataFile);
    spdlog::info("Threshold: {}", threshold);
    
    try {
        // Run backtest
        Backtester backtester(2.10, 1.0);  // $2.10 commission, 1 tick slippage
        
        auto startTime = std::chrono::high_resolution_clock::now();
        PerformanceMetrics metrics = backtester.run(dataFile, threshold);
        auto endTime = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
        double totalTimeSeconds = duration.count() / 1e6;
        
        // Write results
        backtester.writeResults("results.csv");
        
        // Print metrics
        std::cout << "\n=== Backtest Results ===\n";
        std::cout << "Total Return: " << std::fixed << std::setprecision(4) 
                  << metrics.totalReturn * 100.0 << "%\n";
        std::cout << "Volatility: " << metrics.volatility * 100.0 << "%\n";
        std::cout << "Sharpe Ratio: " << metrics.sharpeRatio << "\n";
        std::cout << "Max Drawdown: " << metrics.maxDrawdown * 100.0 << "%\n";
        std::cout << "Win Rate: " << metrics.winRate * 100.0 << "%\n";
        std::cout << "Avg Trade Length: " << metrics.avgTradeLength << " seconds\n";
        std::cout << "Ticks Processed: " << metrics.totalTicks << "\n";
        std::cout << "Ticks/Second: " << metrics.ticksPerSecond << "\n";
        std::cout << "Total Trades: " << metrics.totalTrades << "\n";
        std::cout << "Winning Trades: " << metrics.winningTrades << "\n";
        std::cout << "Processing Time: " << totalTimeSeconds << " seconds\n";
        std::cout << "Avg Latency: " << (totalTimeSeconds * 1e6 / metrics.totalTicks) << " µs/tick\n";
        
        spdlog::info("Backtest completed successfully");
        spdlog::info("Sharpe: {}, Max DD: {}, Throughput: {} ticks/min",
                     metrics.sharpeRatio, metrics.maxDrawdown,
                     metrics.ticksPerSecond * 60.0);
        
    } catch (const std::exception& e) {
        spdlog::error("Error: {}", e.what());
        return 1;
    }
    
    return 0;
}

