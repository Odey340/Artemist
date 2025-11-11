#pragma once

#include <atomic>
#include <memory>
#include <cstddef>
#include <stdexcept>

template<typename T>
class LockFreeQueue {
public:
    explicit LockFreeQueue(size_t capacity)
        : capacity_(capacity),
          mask_(capacity - 1),
          buffer_(new std::atomic<T*>[capacity]),
          head_(0),
          tail_(0) {
        // Ensure capacity is power of 2
        if ((capacity & mask_) != 0) {
            throw std::invalid_argument("Capacity must be power of 2");
        }
        
        // Initialize all slots to nullptr
        for (size_t i = 0; i < capacity_; ++i) {
            buffer_[i].store(nullptr, std::memory_order_relaxed);
        }
    }
    
    ~LockFreeQueue() {
        T* item;
        while (tryPop(item)) {
            delete item;
        }
        delete[] buffer_;
    }
    
    // Non-copyable
    LockFreeQueue(const LockFreeQueue&) = delete;
    LockFreeQueue& operator=(const LockFreeQueue&) = delete;
    
    // Push item (MPSC - Multiple Producers Single Consumer)
    bool tryPush(T* item) {
        if (item == nullptr) {
            return false;
        }
        
        size_t currentTail = tail_.load(std::memory_order_relaxed);
        size_t nextTail = (currentTail + 1) & mask_;
        
        // Check if queue is full
        if (nextTail == head_.load(std::memory_order_acquire)) {
            return false;  // Queue is full
        }
        
        // Try to place item
        T* expected = nullptr;
        if (buffer_[currentTail].compare_exchange_weak(expected, item,
                                                      std::memory_order_release,
                                                      std::memory_order_relaxed)) {
            tail_.store(nextTail, std::memory_order_release);
            return true;
        }
        
        return false;
    }
    
    // Pop item (single consumer)
    bool tryPop(T*& item) {
        size_t currentHead = head_.load(std::memory_order_relaxed);
        
        if (currentHead == tail_.load(std::memory_order_acquire)) {
            return false;  // Queue is empty
        }
        
        T* value = buffer_[currentHead].load(std::memory_order_acquire);
        if (value == nullptr) {
            return false;  // Slot not yet written
        }
        
        if (buffer_[currentHead].compare_exchange_strong(value, nullptr,
                                                         std::memory_order_release,
                                                         std::memory_order_relaxed)) {
            item = value;
            head_.store((currentHead + 1) & mask_, std::memory_order_release);
            return true;
        }
        
        return false;
    }
    
    bool empty() const {
        return head_.load(std::memory_order_acquire) == 
               tail_.load(std::memory_order_acquire);
    }
    
    size_t size() const {
        size_t head = head_.load(std::memory_order_acquire);
        size_t tail = tail_.load(std::memory_order_acquire);
        return (tail - head) & mask_;
    }

private:
    const size_t capacity_;
    const size_t mask_;
    std::atomic<T*>* buffer_;
    alignas(64) std::atomic<size_t> head_;  // Cache line aligned
    alignas(64) std::atomic<size_t> tail_;  // Cache line aligned
};

