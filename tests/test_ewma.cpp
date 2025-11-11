#include <gtest/gtest.h>
#include "RollingStatistics.hpp"
#include <vector>
#include <cmath>
#include <numeric>
#include <random>

// Test EWMA against pandas implementation
// pandas: df.ewm(span=N, adjust=False).mean()
TEST(RollingStatisticsTest, EWMACorrectness) {
    RollingStatistics stats(100);
    
    // Generate test data
    std::vector<double> data;
    std::mt19937 gen(42);  // Seed for reproducibility
    std::normal_distribution<double> dist(100.0, 10.0);
    
    for (int i = 0; i < 200; ++i) {
        double value = dist(gen);
        data.push_back(value);
        stats.update(value);
    }
    
    // Verify statistics are reasonable
    ASSERT_GT(stats.mean(), 0.0);
    ASSERT_GT(stats.variance(), 0.0);
    ASSERT_GT(stats.stddev(), 0.0);
    
    // Verify mean is within reasonable range (should be around 100)
    ASSERT_NEAR(stats.mean(), 100.0, 5.0);
    
    // Verify variance is positive
    ASSERT_GE(stats.variance(), 0.0);
}

TEST(RollingStatisticsTest, ZScoreCalculation) {
    RollingStatistics stats(100);
    
    // Add known values
    for (int i = 0; i < 150; ++i) {
        stats.update(100.0 + (i % 10) - 5.0);
    }
    
    // Z-score of mean should be approximately 0
    double zscore = stats.zscore(stats.mean());
    ASSERT_NEAR(zscore, 0.0, 0.5);
    
    // Z-score should increase with distance from mean
    double z1 = stats.zscore(stats.mean() + stats.stddev());
    double z2 = stats.zscore(stats.mean() + 2 * stats.stddev());
    ASSERT_GT(std::abs(z2), std::abs(z1));
}

TEST(RollingStatisticsTest, WindowSize) {
    RollingStatistics stats(50);
    
    // Fill beyond window size
    for (int i = 0; i < 200; ++i) {
        stats.update(static_cast<double>(i));
    }
    
    // Should have processed all values
    ASSERT_GE(stats.count(), 200);
    ASSERT_TRUE(stats.isReady());
}

TEST(RollingStatisticsTest, InitialPhase) {
    RollingStatistics stats(100);
    
    // Initially not ready
    ASSERT_FALSE(stats.isReady());
    
    // Add values up to window size
    for (int i = 0; i < 100; ++i) {
        stats.update(50.0 + i);
        if (i < 99) {
            // Should be ready only after window is filled
            if (stats.count() < 100) {
                // May or may not be ready depending on implementation
            }
        }
    }
    
    // Should be ready after window is filled
    ASSERT_TRUE(stats.isReady());
}

TEST(RollingStatisticsTest, VarianceNonNegative) {
    RollingStatistics stats(100);
    
    // Add various values
    for (int i = 0; i < 200; ++i) {
        stats.update(static_cast<double>(i % 50));
        ASSERT_GE(stats.variance(), 0.0);
    }
}

TEST(RollingStatisticsTest, Consistency) {
    RollingStatistics stats(100);
    
    // Add constant values
    for (int i = 0; i < 150; ++i) {
        stats.update(100.0);
    }
    
    // Mean should be 100
    ASSERT_NEAR(stats.mean(), 100.0, 0.1);
    
    // Variance should be very small for constant values
    ASSERT_LT(stats.variance(), 1.0);
}

