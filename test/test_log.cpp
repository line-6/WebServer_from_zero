/*
 * Log 模块测试文件
 * 测试同步/异步日志、日志级别、文件管理、多线程等功能
 */
#include "../code/log/log.h"
#include "../code/log/blockqueue.h"
#include <iostream>
#include <assert.h>
#include <thread>
#include <vector>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

// 辅助函数：检查文件是否存在
bool FileExists(const std::string& path) {
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}

// 辅助函数：读取文件内容
std::string ReadFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// 测试1：同步模式（直接写文件）
void TestSyncMode() {
    std::cout << "\n========== 测试1: 同步模式 ==========" << std::endl;
    
    Log::Instance().init(0, "./test_logs/sync", ".log", 0);  // maxQueueSize = 0 表示同步模式
    assert(Log::Instance().IsOpen());
    
    LOG_DEBUG("This is a debug message in sync mode");
    LOG_INFO("This is an info message in sync mode");
    LOG_WARN("This is a warn message in sync mode");
    LOG_ERROR("This is an error message in sync mode");
    
    Log::Instance().flush();
    std::cout << "同步模式测试完成" << std::endl;
}

// 测试2：异步模式（使用队列）
void TestAsyncMode() {
    std::cout << "\n========== 测试2: 异步模式 ==========" << std::endl;
    
    Log::Instance().init(0, "./test_logs/async", ".log", 1000);  // maxQueueSize > 0 表示异步模式
    assert(Log::Instance().IsOpen());
    
    // 写入大量日志，测试异步性能
    for (int i = 0; i < 100; i++) {
        LOG_INFO("Async log message %d", i);
    }
    
    // 等待队列中的日志被写入
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    Log::Instance().flush();
    
    std::cout << "异步模式测试完成" << std::endl;
}

// 测试3：日志级别过滤
void TestLogLevel() {
    std::cout << "\n========== 测试3: 日志级别过滤 ==========" << std::endl;
    
    // 设置日志级别为 INFO (1)，DEBUG (0) 应该被过滤
    Log::Instance().init(1, "./test_logs/level", ".log", 0);
    Log::Instance().SetLevel(1);
    assert(Log::Instance().GetLevel() == 1);
    
    LOG_DEBUG("This DEBUG message should NOT appear");
    LOG_INFO("This INFO message should appear");
    LOG_WARN("This WARN message should appear");
    LOG_ERROR("This ERROR message should appear");
    
    Log::Instance().flush();
    std::cout << "日志级别过滤测试完成（请检查日志文件确认 DEBUG 消息被过滤）" << std::endl;
}

// 测试4：日志文件自动创建和管理
void TestFileManagement() {
    std::cout << "\n========== 测试4: 文件管理 ==========" << std::endl;
    
    Log::Instance().init(0, "./test_logs/file_mgmt", ".log", 0);
    
    // 写入一些日志
    for (int i = 0; i < 10; i++) {
        LOG_INFO("File management test message %d", i);
    }
    
    Log::Instance().flush();
    
    // 检查文件是否创建（文件名格式：YYYY_MM_DD.log）
    time_t now = time(nullptr);
    struct tm* t = localtime(&now);
    char expectedFile[256];
    snprintf(expectedFile, sizeof(expectedFile), "./test_logs/file_mgmt/%04d_%02d_%02d.log",
             t->tm_year + 1900, t->tm_mon + 1, t->tm_mday);
    
    if (FileExists(expectedFile)) {
        std::cout << "✓ 日志文件创建成功: " << expectedFile << std::endl;
        
        // 读取文件内容验证
        std::string content = ReadFile(expectedFile);
        if (content.find("File management test message") != std::string::npos) {
            std::cout << "✓ 日志内容写入成功" << std::endl;
        }
    } else {
        std::cout << "✗ 日志文件未创建: " << expectedFile << std::endl;
    }
}

// 测试5：多线程并发写入
void TestMultiThread() {
    std::cout << "\n========== 测试5: 多线程并发 ==========" << std::endl;
    
    Log::Instance().init(0, "./test_logs/multithread", ".log", 1000);  // 异步模式
    
    const int numThreads = 5;
    const int logsPerThread = 20;
    std::vector<std::thread> threads;
    
    // 创建多个线程同时写入日志
    for (int t = 0; t < numThreads; t++) {
        threads.emplace_back([t, logsPerThread]() {
            for (int i = 0; i < logsPerThread; i++) {
                LOG_INFO("Thread %d: Log message %d", t, i);
            }
        });
    }
    
    // 等待所有线程完成
    for (auto& t : threads) {
        t.join();
    }
    
    // 等待队列中的日志被写入
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    Log::Instance().flush();
    
    std::cout << "多线程并发测试完成（" << numThreads << " 个线程，每个写入 " 
              << logsPerThread << " 条日志）" << std::endl;
}

// 测试6：BlockDeque 基本功能
void TestBlockDeque() {
    std::cout << "\n========== 测试6: BlockDeque 基本功能 ==========" << std::endl;
    
    BlockDeque<int> deque(10);  // 容量为 10
    
    // 测试基本操作
    assert(deque.empty());
    assert(!deque.full());
    assert(deque.size() == 0);
    assert(deque.capacity() == 10);
    
    // 测试 push_back 和 pop
    for (int i = 0; i < 5; i++) {
        deque.push_back(i);
    }
    
    assert(deque.size() == 5);
    assert(!deque.empty());
    assert(!deque.full());
    
    int value;
    for (int i = 0; i < 5; i++) {
        assert(deque.pop(value));
        assert(value == i);
    }
    
    assert(deque.empty());
    std::cout << "✓ BlockDeque 基本操作测试通过" << std::endl;
    
    // 测试 push_front
    for (int i = 0; i < 3; i++) {
        deque.push_front(i);
    }
    
    assert(deque.size() == 3);
    assert(deque.pop(value) && value == 2);  // 后进先出
    assert(deque.pop(value) && value == 1);
    assert(deque.pop(value) && value == 0);
    
    std::cout << "✓ BlockDeque push_front 测试通过" << std::endl;
}

// 测试7：BlockDeque 阻塞机制（生产者-消费者）
void TestBlockDequeBlocking() {
    std::cout << "\n========== 测试7: BlockDeque 阻塞机制 ==========" << std::endl;
    
    BlockDeque<std::string> deque(3);  // 小容量，容易触发阻塞
    
    std::atomic<bool> producerDone(false);
    std::atomic<bool> consumerDone(false);
    std::vector<std::string> consumed;
    
    // 消费者线程
    std::thread consumer([&]() {
        std::string item;
        while (!producerDone || !deque.empty()) {
            if (deque.pop(item, 1)) {  // 1秒超时
                consumed.push_back(item);
            }
        }
        consumerDone = true;
    });
    
    // 生产者线程
    std::thread producer([&]() {
        for (int i = 0; i < 10; i++) {
            std::string msg = "Message " + std::to_string(i);
            deque.push_back(msg);
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        producerDone = true;
    });
    
    producer.join();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    deque.close();  // 关闭队列，唤醒消费者
    consumer.join();
    
    assert(consumed.size() == 10);
    std::cout << "✓ BlockDeque 阻塞机制测试通过（生产了 10 条消息，消费了 " 
              << consumed.size() << " 条）" << std::endl;
}

// 测试8：日志格式化（可变参数）
void TestLogFormatting() {
    std::cout << "\n========== 测试8: 日志格式化 ==========" << std::endl;
    
    Log::Instance().init(0, "./test_logs/format", ".log", 0);
    
    int userId = 12345;
    const char* username = "Alice";
    double balance = 99.99;
    
    LOG_INFO("User %s (ID: %d) has balance: %.2f", username, userId, balance);
    LOG_WARN("Warning: User %d attempted invalid operation", userId);
    LOG_ERROR("Error: Failed to process request for user %s", username);
    
    Log::Instance().flush();
    std::cout << "日志格式化测试完成" << std::endl;
}

// 测试9：日志级别动态切换
void TestDynamicLevelChange() {
    std::cout << "\n========== 测试9: 动态切换日志级别 ==========" << std::endl;
    
    Log::Instance().init(0, "./test_logs/dynamic", ".log", 0);
    
    // 初始级别：DEBUG (0)，所有日志都应该显示
    Log::Instance().SetLevel(0);
    LOG_DEBUG("This should appear (level 0)");
    LOG_INFO("This should appear (level 1)");
    
    // 切换到 INFO (1)，DEBUG 应该被过滤
    Log::Instance().SetLevel(1);
    LOG_DEBUG("This should NOT appear (level 0, filtered)");
    LOG_INFO("This should appear (level 1)");
    LOG_WARN("This should appear (level 2)");
    
    // 切换到 ERROR (3)，只有 ERROR 显示
    Log::Instance().SetLevel(3);
    LOG_DEBUG("This should NOT appear");
    LOG_INFO("This should NOT appear");
    LOG_WARN("This should NOT appear");
    LOG_ERROR("This should appear (level 3)");
    
    Log::Instance().flush();
    std::cout << "动态切换日志级别测试完成" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "    Log 模块全面测试开始" << std::endl;
    std::cout << "========================================" << std::endl;
    
    // 创建测试日志目录
    system("mkdir -p ./test_logs/sync ./test_logs/async ./test_logs/level "
           "./test_logs/file_mgmt ./test_logs/multithread ./test_logs/format "
           "./test_logs/dynamic");
    
    try {
        TestBlockDeque();
        TestBlockDequeBlocking();
        TestSyncMode();
        TestAsyncMode();
        TestLogLevel();
        TestFileManagement();
        TestMultiThread();
        TestLogFormatting();
        TestDynamicLevelChange();
        
        std::cout << "\n========================================" << std::endl;
        std::cout << "    所有测试完成！" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "\n提示：请检查 ./test_logs/ 目录下的日志文件验证结果" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "测试失败: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}