#pragma once

#include <cstddef>
#include <mutex>
#include <deque>
#include <condition_variable>
#include <sys/time.h>
#include <assert.h>

template<class T>
class BlockDeque {
public:
    explicit BlockDeque(size_t MaxCapacity = 1000);
    ~BlockDeque();

    void clear();
    bool empty();
    bool full();
    void close();

    size_t size();
    size_t capacity();
    
    T front();
    T back();

    void push_back(const T &item);
    void push_front(const T &item);

    bool pop(T &item);
    bool pop(T &item, int timeout);

    void flush();
private:
    std::deque<T> deque_;
    size_t capacity_;
    std::mutex mtx_;
    bool isClose_;
    std::condition_variable condConsumer_;
    std::condition_variable condProducer_;
};

template<class T>
BlockDeque<T>::BlockDeque(size_t MaxCapacity): capacity_(MaxCapacity) {
    assert(capacity_ > 0);
    isClose_ = false;
}

template<class T>
BlockDeque<T>::~BlockDeque() {
    close();
}

template<class T>
void BlockDeque<T>::close() {
    {
        std::lock_guard<std::mutex> locker(mtx_);
        deque_.clear();
        isClose_ = true;
    }
    condProducer_.notify_all();
    condConsumer_.notify_all();
}

template<class T>
void BlockDeque<T>::flush() {
    condConsumer_.notify_one();
}

template<class T>
void BlockDeque<T>::clear() {
    std::lock_guard<std::mutex> locker(mtx_);
    deque_.clear();
}

template<class T>
T BlockDeque<T>::front() {
    std::lock_guard<std::mutex> locker(mtx_);
    return deque_.front();
}

template<class T>
T BlockDeque<T>::back() {
    std::lock_guard<std::mutex> locker(mtx_);
    return deque_.back();
}

template<class T>
size_t BlockDeque<T>::size() {
    std::lock_guard<std::mutex> locker(mtx_);
    return deque_.size();
}

template<class T>
size_t BlockDeque<T>::capacity() {
    std::lock_guard<std::mutex> locker(mtx_);
    return capacity_;
}

template<class T>
void BlockDeque<T>::push_back(const T &item) {
    std::unique_lock<std::mutex> locker(mtx_);
    // 队列若满（由lambda表达式控制），生产者等待
    condProducer_.wait(locker, [this] {return deque_.size() < capacity_;});
    deque_.push_back(item);
    condConsumer_.notify_one();
}

template<class T>
void BlockDeque<T>::push_front(const T &item) {
    std::unique_lock<std::mutex> locker(mtx_);
    // 队列满，生产者等待
    condProducer_.wait(locker, [this] {return deque_.size() < capacity_;});
    deque_.push_front(item);
    condConsumer_.notify_one();
}

template<class T>
bool BlockDeque<T>::empty() {
    std::lock_guard<std::mutex> locker(mtx_);
    return deque_.empty();
}

template<class T>
bool BlockDeque<T>::full() {
    std::lock_guard<std::mutex> locker(mtx_);
    return deque_.size() >= capacity_;
}

template<class T>
bool BlockDeque<T>::pop(T &item) {
    std::unique_lock<std::mutex> locker(mtx_);
    // 队列空，消费者等待
    while(deque_.empty()) {
        condConsumer_.wait(locker);
        if (isClose_) {
            return false;
        }
    }
    item = deque_.front();
    deque_.pop_front();
    condProducer_.notify_one();
    return true;
}

template<class T>
bool BlockDeque<T>::pop(T &item, int timeout) {
    std::unique_lock<std::mutex> locker(mtx_);
    // 队列空，消费者等待
    while(deque_.empty()) {
        if (condConsumer_.wait_for(locker, std::chrono::seconds(timeout)) 
            == std::cv_status::timeout) {
                // 如果等待超时
                return false;
            }
        if (isClose_) {
            return false;
        }
    }
    item = deque_.front();
    deque_.pop_front();
    condProducer_.notify_one();
    return true;
}