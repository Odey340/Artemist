#include <gtest/gtest.h>
#include "MarketDataReader.hpp"
#include <fstream>
#include <sstream>
#include <cstdio>

TEST(MarketDataReaderTest, BasicReading) {
    // Create test CSV file
    std::string testFile = "test_data.csv";
    std::ofstream out(testFile);
    out << "timestamp,bid,ask,volume\n";
    out << "1000000,4500.25,4500.50,100\n";
    out << "2000000,4500.75,4501.00,200\n";
    out << "3000000,4501.25,4501.50,150\n";
    out.close();
    
    MarketDataReader reader(testFile);
    ASSERT_TRUE(reader.isValid());
    
    Tick tick;
    ASSERT_TRUE(reader.next(tick));
    ASSERT_EQ(tick.timestamp, 1000000);
    ASSERT_DOUBLE_EQ(tick.bid, 4500.25);
    ASSERT_DOUBLE_EQ(tick.ask, 4500.50);
    ASSERT_EQ(tick.volume, 100);
    ASSERT_DOUBLE_EQ(tick.mid(), 4500.375);
    
    ASSERT_TRUE(reader.next(tick));
    ASSERT_EQ(tick.timestamp, 2000000);
    
    ASSERT_TRUE(reader.next(tick));
    ASSERT_EQ(tick.timestamp, 3000000);
    
    ASSERT_FALSE(reader.next(tick));  // EOF
    
    // Clean up
    remove(testFile.c_str());
}

TEST(MarketDataReaderTest, Reset) {
    std::string testFile = "test_data_reset.csv";
    std::ofstream out(testFile);
    out << "timestamp,bid,ask,volume\n";
    out << "1000000,4500.25,4500.50,100\n";
    out << "2000000,4500.75,4501.00,200\n";
    out.close();
    
    MarketDataReader reader(testFile);
    ASSERT_TRUE(reader.isValid());
    
    Tick tick;
    reader.next(tick);
    reader.next(tick);
    ASSERT_FALSE(reader.next(tick));  // EOF
    
    reader.reset();
    ASSERT_TRUE(reader.next(tick));
    ASSERT_EQ(tick.timestamp, 1000000);
    
    remove(testFile.c_str());
}

TEST(MarketDataReaderTest, InvalidFile) {
    MarketDataReader reader("nonexistent_file.csv");
    ASSERT_FALSE(reader.isValid());
    
    Tick tick;
    ASSERT_FALSE(reader.next(tick));
}

TEST(MarketDataReaderTest, EmptyFile) {
    std::string testFile = "test_empty.csv";
    std::ofstream out(testFile);
    out << "timestamp,bid,ask,volume\n";
    out.close();
    
    MarketDataReader reader(testFile);
    // May or may not be valid depending on implementation
    Tick tick;
    ASSERT_FALSE(reader.next(tick));
    
    remove(testFile.c_str());
}

TEST(MarketDataReaderTest, MidPriceCalculation) {
    Tick tick;
    tick.bid = 4500.25;
    tick.ask = 4500.75;
    
    ASSERT_DOUBLE_EQ(tick.mid(), 4500.50);
}

TEST(MarketDataReaderTest, LargeFile) {
    std::string testFile = "test_large.csv";
    std::ofstream out(testFile);
    out << "timestamp,bid,ask,volume\n";
    
    // Generate 1000 ticks
    for (int i = 0; i < 1000; ++i) {
        out << (1000000 + i * 1000) << ",4500.25,4500.50," << (100 + i) << "\n";
    }
    out.close();
    
    MarketDataReader reader(testFile);
    ASSERT_TRUE(reader.isValid());
    
    Tick tick;
    int count = 0;
    while (reader.next(tick)) {
        count++;
        ASSERT_GT(tick.timestamp, 0);
        ASSERT_GT(tick.bid, 0);
        ASSERT_GT(tick.ask, 0);
        ASSERT_GT(tick.volume, 0);
    }
    
    ASSERT_EQ(count, 1000);
    
    remove(testFile.c_str());
}

TEST(MarketDataReaderTest, ApproximateTickCount) {
    std::string testFile = "test_approx.csv";
    std::ofstream out(testFile);
    out << "timestamp,bid,ask,volume\n";
    for (int i = 0; i < 100; ++i) {
        out << (1000000 + i) << ",4500.25,4500.50,100\n";
    }
    out.close();
    
    MarketDataReader reader(testFile);
    size_t approx = reader.approximateTickCount();
    ASSERT_GT(approx, 0);
    
    remove(testFile.c_str());
}

TEST(MarketDataReaderTest, WindowsLineEndings) {
    std::string testFile = "test_windows.csv";
    std::ofstream out(testFile, std::ios::binary);
    out << "timestamp,bid,ask,volume\r\n";
    out << "1000000,4500.25,4500.50,100\r\n";
    out << "2000000,4500.75,4501.00,200\r\n";
    out.close();
    
    MarketDataReader reader(testFile);
    ASSERT_TRUE(reader.isValid());
    
    Tick tick;
    ASSERT_TRUE(reader.next(tick));
    ASSERT_EQ(tick.timestamp, 1000000);
    
    ASSERT_TRUE(reader.next(tick));
    ASSERT_EQ(tick.timestamp, 2000000);
    
    remove(testFile.c_str());
}

TEST(MarketDataReaderTest, MalformedLines) {
    std::string testFile = "test_malformed.csv";
    std::ofstream out(testFile);
    out << "timestamp,bid,ask,volume\n";
    out << "1000000,4500.25,4500.50,100\n";
    out << "invalid_line\n";
    out << "2000000,4500.75,4501.00,200\n";
    out << "another,bad,line\n";
    out << "3000000,4501.25,4501.50,150\n";
    out.close();
    
    MarketDataReader reader(testFile);
    ASSERT_TRUE(reader.isValid());
    
    Tick tick;
    // Should skip malformed lines and read valid ones
    int validTicks = 0;
    while (reader.next(tick)) {
        validTicks++;
    }
    
    // Should have at least the valid ticks
    ASSERT_GE(validTicks, 3);
    
    remove(testFile.c_str());
}

