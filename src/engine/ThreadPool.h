#pragma once
#include <vector>
#include <thread>
#include <functional>
#include <atomic>
#include "JobQueue.h"

namespace zc {

    class ThreadPool {
    public:
        // Default: use all CPU cores minus one so UI stays responsive
        explicit ThreadPool(
            size_t numThreads = std::max(1u,
                std::thread::hardware_concurrency() - 1))
        {
            running_ = true;
            for (size_t i = 0; i < numThreads; ++i) {
                workers_.emplace_back([this] { workerLoop(); });
            }
        }

        ~ThreadPool() {
            shutdown();
        }

        // No copy
        ThreadPool(const ThreadPool&) = delete;
        ThreadPool& operator=(const ThreadPool&) = delete;

        // Submit any callable
        template<typename F>
        void submit(F&& fn) {
            queue_.push(std::function<void()>(std::forward<F>(fn)));
        }

        void shutdown() {
            if (!running_) return;
            running_ = false;
            queue_.stop();
            for (auto& t : workers_)
                if (t.joinable()) t.join();
        }

        size_t threadCount() const {
            return workers_.size();
        }

    private:
        void workerLoop() {
            while (running_) {
                auto task = queue_.pop();
                if (task) (*task)();
            }
        }

        JobQueue<std::function<void()>> queue_;
        std::vector<std::thread>        workers_;
        std::atomic<bool>               running_{ false };
    };

} // namespace zc