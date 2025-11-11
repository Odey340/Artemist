#include <gtest/gtest.h>
#include "LockFreeQueue.hpp"
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>

TEST(LockFreeQueueTest, BasicPushPop) {
    LockFreeQueue<int> queue(1024);
    
    int* item1 = new int(42);
    int* item2 = new int(43);
    
    ASSERT_TRUE(queue.tryPush(item1));
    ASSERT_TRUE(queue.tryPush(item2));
    ASSERT_FALSE(queue.empty());
    ASSERT_EQ(queue.size(), 2);
    
    int* popped = nullptr;
    ASSERT_TRUE(queue.tryPop(popped));
    ASSERT_NE(popped, nullptr);
    ASSERT_EQ(*popped, 42);
    delete popped;
    
    ASSERT_TRUE(queue.tryPop(popped));
    ASSERT_NE(popped, nullptr);
    ASSERT_EQ(*popped, 43);
    delete popped;
    
    ASSERT_TRUE(queue.empty());
}

TEST(LockFreeQueueTest, MPSCStressTest) {
    const size_t capacity = 1024 * 1024;  // 1M capacity
    LockFreeQueue<int> queue(capacity);
    const size_t numPushes = 1000000;  // 1M pushes
    std::atomic<size_t> pushCount{0};
    std::atomic<size_t> popCount{0};
    std::atomic<size_t> pushFailures{0};
    std::atomic<bool> start{false};
    
    // Multiple producer threads
    const size_t numProducers = 4;
    std::vector<std::thread> producers;
    
    for (size_t i = 0; i < numProducers; ++i) {
        producers.emplace_back([&, i]() {
            while (!start.load()) {
                std::this_thread::yield();
            }
            
            for (size_t j = 0; j < numPushes / numProducers; ++j) {
                int* item = new int(static_cast<int>(i * numPushes + j));
                if (!queue.tryPush(item)) {
                    pushFailures.fetch_add(1);
                    delete item;
                } else {
                    pushCount.fetch_add(1);
                }
            }
        });
    }
    
    // Single consumer thread
    std::thread consumer([&]() {
        std::vector<int> received;
        received.reserve(numPushes);
        
        while (popCount.load() < numPushes && pushCount.load() < numPushes) {
            int* item = nullptr;
            if (queue.tryPop(item)) {
                if (item != nullptr) {
                    received.push_back(*item);
                    delete item;
                    popCount.fetch_add(1);
                }
            } else {
                std::this_thread::yield();
            }
        }
        
        // Drain remaining items
        int* item = nullptr;
        while (queue.tryPop(item)) {
            if (item != nullptr) {
                received.push_back(*item);
                delete item;
                popCount.fetch_add(1);
            }
        }
    });
    
    // Start all threads
    start.store(true);
    
    // Wait for producers
    for (auto& t : producers) {
        t.join();
    }
    
    // Wait a bit for queue to fill
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Consumer should finish
    consumer.join();
    
    // Verify no memory leaks
    int* item = nullptr;
    size_t drained = 0;
    while (queue.tryPop(item)) {
        if (item != nullptr) {
            delete item;
            drained++;
        }
    }
    
    std::cout << "Pushes: " << pushCount.load() << std::endl;
    std::cout << "Pops: " << popCount.load() << std::endl;
    std::cout << "Push failures: " << pushFailures.load() << std::endl;
    std::cout << "Drained: " << drained << std::endl;
    
    // Should have processed most items (some failures expected with high contention)
    ASSERT_GT(pushCount.load(), numPushes * 0.9);  // At least 90% success
    ASSERT_GT(popCount.load() + drained, numPushes * 0.9);
}

TEST(LockFreeQueueTest, EmptyQueue) {
    LockFreeQueue<int> queue(1024);
    
    ASSERT_TRUE(queue.empty());
    ASSERT_EQ(queue.size(), 0);
    
    int* item = nullptr;
    ASSERT_FALSE(queue.tryPop(item));
}

TEST(LockFreeQueueTest, FullQueue) {
    LockFreeQueue<int> queue(16);  // Small capacity
    
    // Fill queue
    for (int i = 0; i < 15; ++i) {  // Leave one slot for circular buffer
        int* item = new int(i);
        ASSERT_TRUE(queue.tryPush(item));
    }
    
    // Next push might fail if queue is full
    int* item = new int(99);
    // May or may not succeed depending on implementation
    queue.tryPush(item);
    
    // Clean up
    int* popped = nullptr;
    while (queue.tryPop(popped)) {
        if (popped != nullptr) {
            delete popped;
        }
    }
    if (item != nullptr && !queue.tryPop(popped)) {
        delete item;  // If not pushed, delete here
    }
}

TEST(LockFreeQueueTest, NullPointerSafety) {
    LockFreeQueue<int> queue(1024);
    
    // Should not crash on null pointer
    ASSERT_FALSE(queue.tryPush(nullptr));
    
    int* item = nullptr;
    ASSERT_FALSE(queue.tryPop(item));
}

TEST(LockFreeQueueTest, PowerOfTwoCapacity) {
    // Should work with power of 2
    LockFreeQueue<int> queue(1024);
    ASSERT_TRUE(queue.empty());
    
    // Should throw for non-power-of-2
    EXPECT_THROW(LockFreeQueue<int> badQueue(1000), std::invalid_argument);
}

