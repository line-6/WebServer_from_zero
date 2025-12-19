#pragma once

#include <chrono>
#include <cstddef>
#include <functional>
#include <vector>
#include <unordered_map>
#include <assert.h>

typedef std::chrono::high_resolution_clock Clock;
typedef Clock::time_point TimeStamp;
typedef std::chrono::milliseconds MS;
typedef std::function<void()> TimeOutCallBack;

// 定时器节点
struct TimerNode {
    int id;
    TimeStamp expires;  // 过期时间点
    TimeOutCallBack cb;

    bool operator< (const TimerNode& t) {
        return expires < t.expires;
    }

};

class HeapTimer {
public:
    HeapTimer() {heap_.reserve(128);}

    ~HeapTimer() {clear();}

    /* 调整节点 */
    void adjust(int id, int newExpires);

    /* 添加节点 */
    void add(int id, int timeout, const TimeOutCallBack& cb);

    /* 删除指定节点，触发回调函数 */
    void doWork(int id);

    void clear();

    /* 清除超时节点 */
    void tick();

    void pop();

    int getNextTick();


private:
    void del_(size_t idx);
    void siftUp_(size_t idx);   //上浮
    bool siftDown_(size_t idx, size_t n);   // 下沉
    void swapNode_(size_t i, size_t j);

    std::vector<TimerNode> heap_;   //自定义最小堆实现，不使用priority_queue因为其不支持随机访问
    std::unordered_map<int, size_t> id2HeapIdx; // 节点 id 到堆索引的映射（实现 O(1) 查找）


};