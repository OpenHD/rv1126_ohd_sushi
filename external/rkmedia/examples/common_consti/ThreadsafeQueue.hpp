//
// Created by consti10 on 13.02.22.
//

#ifndef RV1126_OHD_SUSHI_THREADSAFEQUEUE_HPP
#define RV1126_OHD_SUSHI_THREADSAFEQUEUE_HPP

#include <queue>
#include <mutex>
#include <memory>

// NOTE: the item is wrapped as std::shared_ptr
template<typename T>
class ThreadsafeQueue {
    std::queue<std::shared_ptr<T>> queue_;
    mutable std::mutex mutex_;
    // Moved out of public interface to prevent races between this
    // and pop().
    bool empty() const {
        return queue_.empty();
    }
public:
    ThreadsafeQueue() = default;
    ThreadsafeQueue(const ThreadsafeQueue<T> &) = delete ;
    ThreadsafeQueue& operator=(const ThreadsafeQueue<T> &) = delete ;
    ThreadsafeQueue(ThreadsafeQueue<T>&& other) {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_ = std::move(other.queue_);
    }
    virtual ~ThreadsafeQueue() { }
    unsigned long size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }
    // returns the oldest item in the queue if available
    // nullptr otherwise
    std::shared_ptr<T> popIfAvailable() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.empty()) {
            return std::shared_ptr<T>(nullptr);
        }
        auto tmp = queue_.front();
        queue_.pop();
        return tmp;
    }
    // adds a new item to the queue
    void push(std::shared_ptr<T> item) {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(std::move(item));
    }
};


#endif //RV1126_OHD_SUSHI_THREADSAFEQUEUE_HPP