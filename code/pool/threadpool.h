#pragma once

#include <cstddef>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <thread>
#include <functional>
#include <assert.h>

class ThreadPool {
public:
    explicit ThreadPool(size_t threadCount = 8): pool_(std::make_shared<Pool>()) {
        assert(threadCount > 0);
        for (size_t i = 0; i < threadCount; i ++) {
            std::thread([pool = pool_]() {
                std::unique_lock<std::mutex> locker(pool->mtx_);
                while(true) {
                    if (!pool->tasks.empty()) {
                        auto task = std::move(pool->tasks.front());
                        pool->tasks.pop();
                        locker.unlock();
                        task();
                        locker.lock();
                    }
                    else if (pool->isClosed) break;
                    else {
                        pool->cv_.wait(locker);
                    }
                }
            }).detach();
        }
    }

    ThreadPool() = default;
    ThreadPool(ThreadPool&&) = default;

    ~ThreadPool() {
        if (static_cast<bool>(pool_)) {
            std::lock_guard<std::mutex> locker(pool_->mtx_);
            pool_->isClosed = true;
        }
        pool_->cv_.notify_all();
    }
    
    template<class F>
    void addTask(F&& task) {
        {
            std::lock_guard<std::mutex> locker(pool_->mtx_);
            pool_->tasks.emplace(std::forward<F>(task));
        }
        pool_->cv_.notify_one();
    }

private:
    struct Pool {
        std::mutex mtx_;
        std::condition_variable cv_;
        bool isClosed;
        std::queue<std::function<void()>> tasks;
    };
    std::shared_ptr<Pool> pool_;
};