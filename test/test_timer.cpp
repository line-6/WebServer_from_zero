/*
 * HeapTimer 模块测试文件
 * 测试定时器的添加、删除、更新、超时触发等功能
 */
#include "../code/timer/heaptimer.h"
#include <iostream>
#include <assert.h>
#include <vector>
#include <atomic>
#include <chrono>
#include <thread>
#include <mutex>
#include <set>

// 全局计数器，用于验证回调执行
std::atomic<int> callbackCount(0);
std::atomic<int> timeoutCount(0);
std::mutex outputMutex;
std::set<int> executedCallbacks;

// 辅助函数：打印测试信息（线程安全）
void PrintTest(const std::string& msg) {
    std::lock_guard<std::mutex> lock(outputMutex);
    std::cout << msg << std::endl;
}

// 辅助函数：等待指定毫秒数
void WaitMs(int milliseconds) {
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

// 测试1：基本添加和删除
void TestBasicAddDelete() {
    PrintTest("\n========== 测试1: 基本添加和删除 ==========");
    
    HeapTimer timer;
    callbackCount = 0;
    
    // 添加定时器
    timer.add(1, 100, []() {
        callbackCount++;
        PrintTest("  Timer 1 triggered");
    });
    
    timer.add(2, 200, []() {
        callbackCount++;
        PrintTest("  Timer 2 triggered");
    });
    
    // 等待定时器触发
    WaitMs(250);
    timer.tick();
    
    assert(callbackCount == 2);
    PrintTest("✓ 基本添加和删除测试通过（2 个定时器都触发）");
}

// 测试2：定时器超时顺序（验证最小堆性质）
void TestTimeoutOrder() {
    PrintTest("\n========== 测试2: 定时器超时顺序 ==========");
    
    HeapTimer timer;
    std::vector<int> executionOrder;
    std::mutex orderMutex;
    
    // 添加多个定时器，过期时间不同
    timer.add(1, 300, [&executionOrder, &orderMutex]() {
        std::lock_guard<std::mutex> lock(orderMutex);
        executionOrder.push_back(1);
    });
    
    timer.add(2, 100, [&executionOrder, &orderMutex]() {
        std::lock_guard<std::mutex> lock(orderMutex);
        executionOrder.push_back(2);
    });
    
    timer.add(3, 200, [&executionOrder, &orderMutex]() {
        std::lock_guard<std::mutex> lock(orderMutex);
        executionOrder.push_back(3);
    });
    
    // 等待所有定时器触发
    WaitMs(350);
    timer.tick();
    
    // 验证执行顺序：应该是 2, 3, 1（按过期时间排序）
    assert(executionOrder.size() == 3);
    assert(executionOrder[0] == 2);  // 100ms
    assert(executionOrder[1] == 3);  // 200ms
    assert(executionOrder[2] == 1);  // 300ms
    
    PrintTest("✓ 定时器超时顺序测试通过（按过期时间顺序触发）");
}

// 测试3：adjust 功能（更新定时器）
void TestAdjust() {
    PrintTest("\n========== 测试3: adjust 功能 ==========");
    
    HeapTimer timer;
    callbackCount = 0;
    
    // 添加定时器，100ms 后过期
    timer.add(1, 100, []() {
        callbackCount++;
        PrintTest("  Timer 1 should not trigger (adjusted)");
    });
    
    // 等待 50ms
    WaitMs(50);
    
    // 调整定时器，延长到 200ms
    timer.adjust(1, 200);
    
    // 再等待 100ms（总共 150ms，原定时器应该已过期，但调整后还没过期）
    WaitMs(100);
    timer.tick();
    
    assert(callbackCount == 0);  // 应该还没触发
    
    // 再等待 100ms（总共 250ms，调整后的定时器应该过期）
    WaitMs(100);
    timer.tick();
    
    assert(callbackCount == 1);  // 现在应该触发了
    PrintTest("✓ adjust 功能测试通过（定时器成功延长）");
}

// 测试4：doWork 功能（手动触发并删除）
void TestDoWork() {
    PrintTest("\n========== 测试4: doWork 功能 ==========");
    
    HeapTimer timer;
    callbackCount = 0;
    
    // 添加定时器
    timer.add(1, 1000, []() {
        callbackCount++;
        PrintTest("  Timer 1 should not trigger (doWork called)");
    });
    
    // 立即调用 doWork，应该触发回调并删除定时器
    timer.doWork(1);
    
    assert(callbackCount == 1);
    
    // 等待原定时间，定时器应该已经被删除，不会再次触发
    WaitMs(1100);
    timer.tick();
    
    assert(callbackCount == 1);  // 仍然是 1，没有再次触发
    
    PrintTest("✓ doWork 功能测试通过（手动触发并删除）");
}

// 测试5：tick 功能（清除超时定时器）
void TestTick() {
    PrintTest("\n========== 测试5: tick 功能 ==========");
    
    HeapTimer timer;
    callbackCount = 0;
    
    // 添加多个定时器
    timer.add(1, 50, []() { callbackCount++; });
    timer.add(2, 100, []() { callbackCount++; });
    timer.add(3, 150, []() { callbackCount++; });
    
    // 等待 75ms，只有 timer 1 应该过期
    WaitMs(75);
    timer.tick();
    
    assert(callbackCount == 1);
    
    // 再等待 50ms，timer 2 应该过期
    WaitMs(50);
    timer.tick();
    
    assert(callbackCount == 2);
    
    // 再等待 50ms，timer 3 应该过期
    WaitMs(50);
    timer.tick();
    
    assert(callbackCount == 3);
    
    PrintTest("✓ tick 功能测试通过（逐步清除超时定时器）");
}

// 测试6：GetNextTick 功能
void TestGetNextTick() {
    PrintTest("\n========== 测试6: GetNextTick 功能 ==========");
    
    HeapTimer timer;
    
    // 空定时器
    int nextTick = timer.getNextTick();
    assert(nextTick == -1);
    
    // 添加定时器，100ms 后过期
    timer.add(1, 100, []() {});
    
    // 立即获取下一个到期时间
    nextTick = timer.getNextTick();
    assert(nextTick >= 0 && nextTick <= 100);  // 应该在 0-100ms 之间
    
    PrintTest("✓ GetNextTick 功能测试通过（返回下一个到期时间: " + 
              std::to_string(nextTick) + "ms）");
}

// 测试7：更新已存在的定时器（add 的更新功能）
void TestUpdateExisting() {
    PrintTest("\n========== 测试7: 更新已存在的定时器 ==========");
    
    HeapTimer timer;
    callbackCount = 0;
    
    // 添加定时器
    timer.add(1, 100, []() {
        callbackCount++;
        PrintTest("  Original callback");
    });
    
    // 更新定时器（使用相同的 ID）
    timer.add(1, 200, []() {
        callbackCount++;
        PrintTest("  Updated callback");
    });
    
    // 等待 150ms，原定时器应该已过期，但更新后还没过期
    WaitMs(150);
    timer.tick();
    
    assert(callbackCount == 0);  // 应该还没触发（使用新的回调）
    
    // 再等待 100ms，更新后的定时器应该过期
    WaitMs(100);
    timer.tick();
    
    assert(callbackCount == 1);  // 应该触发（使用新的回调）
    
    PrintTest("✓ 更新已存在的定时器测试通过");
}

// 测试8：clear 功能
void TestClear() {
    PrintTest("\n========== 测试8: clear 功能 ==========");
    
    HeapTimer timer;
    callbackCount = 0;
    
    // 添加多个定时器
    timer.add(1, 100, []() { callbackCount++; });
    timer.add(2, 200, []() { callbackCount++; });
    timer.add(3, 300, []() { callbackCount++; });
    
    // 清空定时器
    timer.clear();
    
    // 等待所有定时器应该过期的时间
    WaitMs(350);
    timer.tick();
    
    assert(callbackCount == 0);  // 应该没有触发（已清空）
    
    PrintTest("✓ clear 功能测试通过（所有定时器被清除）");
}

// 测试9：大量定时器性能测试
void TestPerformance() {
    PrintTest("\n========== 测试9: 性能测试 ==========");
    
    HeapTimer timer;
    callbackCount = 0;
    const int numTimers = 1000;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // 添加大量定时器
    for (int i = 0; i < numTimers; i++) {
        timer.add(i, 1000 + i, []() { callbackCount++; });
    }
    
    auto addEnd = std::chrono::high_resolution_clock::now();
    auto addDuration = std::chrono::duration_cast<std::chrono::microseconds>(addEnd - start);
    
    // 更新部分定时器
    for (int i = 0; i < numTimers; i += 10) {
        timer.adjust(i, 2000);
    }
    
    auto adjustEnd = std::chrono::high_resolution_clock::now();
    auto adjustDuration = std::chrono::duration_cast<std::chrono::microseconds>(adjustEnd - addEnd);
    
    PrintTest("✓ 性能测试完成（添加 " + std::to_string(numTimers) + 
              " 个定时器: " + std::to_string(addDuration.count()) + "μs，" +
              "更新 " + std::to_string(numTimers / 10) + " 个: " + 
              std::to_string(adjustDuration.count()) + "μs）");
}

// 测试10：边界情况测试
void TestEdgeCases() {
    PrintTest("\n========== 测试10: 边界情况 ==========");
    
    HeapTimer timer;
    callbackCount = 0;
    
    // 测试1：删除不存在的定时器
    timer.doWork(999);  // 应该不崩溃
    assert(callbackCount == 0);
    
    // 测试2：调整不存在的定时器（应该崩溃，但这里不测试，因为代码中有 assert）
    // timer.adjust(999, 100);  // 这会触发 assert
    
    // 测试3：空定时器的 tick
    timer.tick();  // 应该不崩溃
    
    // 测试4：空定时器的 pop（应该崩溃，但这里不测试）
    // timer.pop();  // 这会触发 assert
    
    PrintTest("✓ 边界情况测试通过");
}

// 测试11：定时器回调中操作定时器（嵌套操作）
void TestNestedOperations() {
    PrintTest("\n========== 测试11: 嵌套操作 ==========");
    
    HeapTimer timer;
    callbackCount = 0;
    
    // 在回调中添加新定时器
    timer.add(1, 50, [&timer]() {
        callbackCount++;
        timer.add(2, 50, []() {
            callbackCount++;
        });
    });
    
    // 等待第一个定时器触发
    WaitMs(60);
    timer.tick();
    
    assert(callbackCount == 1);
    
    // 等待第二个定时器触发
    WaitMs(60);
    timer.tick();
    
    assert(callbackCount == 2);
    
    PrintTest("✓ 嵌套操作测试通过（回调中可以操作定时器）");
}

// 测试12：相同过期时间的多个定时器
void TestSameExpireTime() {
    PrintTest("\n========== 测试12: 相同过期时间 ==========");
    
    HeapTimer timer;
    callbackCount = 0;
    
    // 添加多个相同过期时间的定时器
    timer.add(1, 100, []() { callbackCount++; });
    timer.add(2, 100, []() { callbackCount++; });
    timer.add(3, 100, []() { callbackCount++; });
    
    // 等待定时器触发
    WaitMs(150);
    timer.tick();
    
    assert(callbackCount == 3);  // 所有定时器都应该触发
    
    PrintTest("✓ 相同过期时间测试通过（所有定时器都触发）");
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "    HeapTimer 模块全面测试开始" << std::endl;
    std::cout << "========================================" << std::endl;
    
    try {
        TestBasicAddDelete();
        TestTimeoutOrder();
        TestAdjust();
        TestDoWork();
        TestTick();
        TestGetNextTick();
        TestUpdateExisting();
        TestClear();
        TestPerformance();
        TestEdgeCases();
        TestNestedOperations();
        TestSameExpireTime();
        
        std::cout << "\n========================================" << std::endl;
        std::cout << "    所有测试完成！" << std::endl;
        std::cout << "========================================" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "测试失败: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}