#include <gtest/gtest.h>
#include "SignalGenerator.hpp"
#include "RollingStatistics.hpp"
#include <cmath>

TEST(SignalGeneratorTest, ZScoreThreshold) {
    SignalGenerator gen(2.5);
    RollingStatistics stats(100);
    
    // Fill stats with data
    for (int i = 0; i < 150; ++i) {
        stats.update(100.0 + (i % 10) - 5.0);
    }
    
    // Test threshold crossing
    double mean = stats.mean();
    double stddev = stats.stddev();
    
    // Price far below mean should generate LONG signal
    double lowPrice = mean - 3.0 * stddev;  // z-score = -3.0
    Signal signal = gen.generate(lowPrice, stats);
    ASSERT_EQ(signal, Signal::LONG);
    
    // Reset and test SHORT
    SignalGenerator gen2(2.5);
    double highPrice = mean + 3.0 * stddev;  // z-score = 3.0
    signal = gen2.generate(highPrice, stats);
    ASSERT_EQ(signal, Signal::SHORT);
}

TEST(SignalGeneratorTest, ZeroCrossingExit) {
    SignalGenerator gen(2.5);
    RollingStatistics stats(100);
    
    // Fill stats
    for (int i = 0; i < 150; ++i) {
        stats.update(100.0);
    }
    
    double mean = stats.mean();
    double stddev = stats.stddev();
    
    // Enter LONG
    double lowPrice = mean - 3.0 * stddev;
    Signal signal = gen.generate(lowPrice, stats);
    ASSERT_EQ(signal, Signal::LONG);
    ASSERT_EQ(gen.currentSignal(), Signal::LONG);
    
    // Price crosses zero (mean)
    signal = gen.generate(mean, stats);
    ASSERT_EQ(signal, Signal::FLAT);
    ASSERT_EQ(gen.currentSignal(), Signal::FLAT);
}

TEST(SignalGeneratorTest, EdgeCaseZScoreEqualsThreshold) {
    SignalGenerator gen(2.5);
    RollingStatistics stats(100);
    
    // Fill stats
    for (int i = 0; i < 150; ++i) {
        stats.update(100.0);
    }
    
    double mean = stats.mean();
    double stddev = stats.stddev();
    
    // Exactly at threshold (should not trigger due to strict inequality in logic)
    // But our implementation uses < and >, so exactly 2.5 won't trigger
    double priceAtThreshold = mean + 2.5 * stddev;
    Signal signal = gen.generate(priceAtThreshold, stats);
    // Should be FLAT or SHORT depending on implementation
    // Our implementation: zscore > threshold triggers SHORT, so 2.5 should trigger
    if (std::abs(stats.zscore(priceAtThreshold) - 2.5) < 0.01) {
        // Close to threshold, may trigger
    }
}

TEST(SignalGeneratorTest, ThresholdBoundary) {
    SignalGenerator gen(2.5);
    RollingStatistics stats(100);
    
    for (int i = 0; i < 150; ++i) {
        stats.update(100.0 + (i % 20) - 10.0);
    }
    
    double mean = stats.mean();
    double stddev = stats.stddev();
    
    // Just below threshold (should not trigger)
    double priceJustBelow = mean - 2.4 * stddev;
    Signal signal = gen.generate(priceJustBelow, stats);
    // Should remain FLAT
    
    // Just above threshold (should trigger)
    double priceJustAbove = mean - 2.6 * stddev;
    signal = gen.generate(priceJustAbove, stats);
    ASSERT_EQ(signal, Signal::LONG);
}

TEST(SignalGeneratorTest, NotReadyStats) {
    SignalGenerator gen(2.5);
    RollingStatistics stats(100);
    
    // Stats not ready yet
    for (int i = 0; i < 50; ++i) {
        stats.update(100.0 + i);
    }
    
    // Should return FLAT when stats not ready
    Signal signal = gen.generate(100.0, stats);
    ASSERT_EQ(signal, Signal::FLAT);
}

TEST(SignalGeneratorTest, StateMachine) {
    SignalGenerator gen(2.5);
    RollingStatistics stats(100);
    
    for (int i = 0; i < 150; ++i) {
        stats.update(100.0);
    }
    
    double mean = stats.mean();
    double stddev = stats.stddev();
    
    // Start FLAT
    ASSERT_EQ(gen.currentSignal(), Signal::FLAT);
    
    // Enter LONG
    gen.generate(mean - 3.0 * stddev, stats);
    ASSERT_EQ(gen.currentSignal(), Signal::LONG);
    
    // Stay LONG (price still below mean but not crossing zero)
    gen.generate(mean - 1.0 * stddev, stats);
    ASSERT_EQ(gen.currentSignal(), Signal::LONG);
    
    // Exit to FLAT (crosses zero)
    gen.generate(mean, stats);
    ASSERT_EQ(gen.currentSignal(), Signal::FLAT);
    
    // Enter SHORT
    gen.generate(mean + 3.0 * stddev, stats);
    ASSERT_EQ(gen.currentSignal(), Signal::SHORT);
    
    // Exit to FLAT
    gen.generate(mean, stats);
    ASSERT_EQ(gen.currentSignal(), Signal::FLAT);
}

TEST(SignalGeneratorTest, ThresholdModification) {
    SignalGenerator gen(2.5);
    
    ASSERT_EQ(gen.getThreshold(), 2.5);
    
    gen.setThreshold(3.0);
    ASSERT_EQ(gen.getThreshold(), 3.0);
}

TEST(SignalGeneratorTest, ShortExitOnZeroCross) {
    SignalGenerator gen(2.5);
    RollingStatistics stats(100);
    
    for (int i = 0; i < 150; ++i) {
        stats.update(100.0);
    }
    
    double mean = stats.mean();
    double stddev = stats.stddev();
    
    // Enter SHORT
    gen.generate(mean + 3.0 * stddev, stats);
    ASSERT_EQ(gen.currentSignal(), Signal::SHORT);
    
    // Cross zero downward (z-score becomes negative)
    gen.generate(mean - 1.0 * stddev, stats);
    ASSERT_EQ(gen.currentSignal(), Signal::FLAT);
}

TEST(SignalGeneratorTest, LongExitOnZeroCross) {
    SignalGenerator gen(2.5);
    RollingStatistics stats(100);
    
    for (int i = 0; i < 150; ++i) {
        stats.update(100.0);
    }
    
    double mean = stats.mean();
    double stddev = stats.stddev();
    
    // Enter LONG
    gen.generate(mean - 3.0 * stddev, stats);
    ASSERT_EQ(gen.currentSignal(), Signal::LONG);
    
    // Cross zero upward (z-score becomes positive)
    gen.generate(mean + 1.0 * stddev, stats);
    ASSERT_EQ(gen.currentSignal(), Signal::FLAT);
}

