#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>
#include <atomic>
#include <chrono>

namespace zc {

    // Thread-safe queue — workers pull jobs from here
    template<typename T>
    class JobQueue {
    public:

        void push(T item) {
            std::lock_guard lock(mutex_);
            queue_.push(std::move(item));
            cv_.notify_one();
        }

        // Blocks until item available or timeout or stopped
        std::optional<T> pop(
            std::chrono::milliseconds timeout = std::chrono::milliseconds(200))
        {
            std::unique_lock lock(mutex_);
            bool gotItem = cv_.wait_for(lock, timeout, [this] {
                return !queue_.empty() || stopped_;
                });

            if (!gotItem || (stopped_ && queue_.empty()))
                return std::nullopt;

            T item = std::move(queue_.front());
            queue_.pop();
            return item;
        }

        void stop() {
            stopped_ = true;
            cv_.notify_all();
        }

        bool empty() const {
            std::lock_guard lock(mutex_);
            return queue_.empty();
        }

        size_t size() const {
            std::lock_guard lock(mutex_);
            return queue_.size();
        }

    private:
        mutable std::mutex      mutex_;
        std::condition_variable cv_;
        std::queue<T>           queue_;
        std::atomic<bool>       stopped_{ false };
    };

} // namespace zc