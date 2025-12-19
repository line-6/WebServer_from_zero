#include "heaptimer.h"
#include <cstddef>

void HeapTimer::adjust(int id, int newExpires) {
    assert(!heap_.empty() && id2HeapIdx.count(id) > 0);
    heap_[id2HeapIdx[id]].expires = Clock::now() + MS(newExpires);
    siftDown_(id2HeapIdx[id], heap_.size());
}

void HeapTimer::add(int id, int timeout, const TimeOutCallBack& cb) {
    assert(id >= 0);
    size_t idx;
    if (id2HeapIdx.count(id) == 0) {
    // 新 id: 堆尾插入，上浮调整
        idx = heap_.size();
        id2HeapIdx[id] = idx;
        heap_.emplace_back(id, Clock::now() + MS(timeout), cb);
        siftUp_(idx);
    }
    else {
    // 堆中已有此 id
        idx = id2HeapIdx[id];
        heap_[idx].expires = Clock::now() + MS(timeout);
        heap_[idx].cb = cb;
        if (!siftDown_(idx, heap_.size())) {
            siftUp_(idx);
        }
    }
}

void HeapTimer::doWork(int id) {
    if(heap_.empty() || id2HeapIdx.count(id) == 0) {
        return;
    }
    size_t idx = id2HeapIdx[id];
    TimerNode node = heap_[idx];
    node.cb();
    del_(idx);
}

void HeapTimer::clear() {
    id2HeapIdx.clear();
    heap_.clear();
}

void HeapTimer::tick() {
    if(heap_.empty()) {
        return;
    }
    while(!heap_.empty()) {
        TimerNode node = heap_.front();
        if(std::chrono::duration_cast<MS>(node.expires - Clock::now()).count() > 0) { 
            break; 
        }
        node.cb();
        pop();
    }
}

void HeapTimer::pop() {
    assert(!heap_.empty());
    del_(0);
}

int HeapTimer::getNextTick() {
    tick();
    size_t res = -1;
    if (!heap_.empty()) {
        res = std::chrono::duration_cast<MS>(heap_.front().expires - Clock::now()).count();
        if (res < 0) res = 0;
    }
    return res;
}


void HeapTimer::del_(size_t idx) {
    assert(!heap_.empty() && idx >= 0 && idx < heap_.size());
    size_t i = idx;
    size_t n = heap_.size() - 1;
    assert(i <= n);
    if(i < n) {
        swapNode_(i, n);
        if(!siftDown_(i, n)) {
            siftUp_(i);
        }
    }
    id2HeapIdx.erase(heap_.back().id);
    heap_.pop_back();
}

void HeapTimer::siftUp_(size_t idx) {
    assert(idx >= 0 && idx < heap_.size());
    size_t j = (idx - 1) / 2;   // idx 对应的父节点
    while (j >= 0 && idx > 0) { // 这里不加 idx > 0，当idx = 0 时报错
        if (heap_[j] < heap_[idx]) break;
        swapNode_(idx, j);
        idx = j;
        j = (idx - 1) / 2;
    }    
}

bool HeapTimer::siftDown_(size_t idx, size_t n) {
    assert(idx >= 0 && idx < heap_.size());
    assert(n >= 0 && n <= heap_.size());
    size_t i = idx;
    size_t j = i * 2 + 1;
    while(j < n) {
        if(j + 1 < n && heap_[j + 1] < heap_[j]) j++;
        if(heap_[i] < heap_[j]) break;
        swapNode_(i, j);
        i = j;
        j = i * 2 + 1;
    }
    return i > idx;
}

void HeapTimer::swapNode_(size_t i, size_t j) {
    assert(i >= 0 && i < heap_.size());
    assert(j >= 0 && j < heap_.size());
    std::swap(heap_[i], heap_[j]);
    id2HeapIdx[heap_[i].id] = i;
    id2HeapIdx[heap_[j].id] = j;
}