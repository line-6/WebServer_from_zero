#pragma once

#include <mutex>
#include <string>
#include <thread>
#include <sys/time.h>
#include <string.h>
#include <stdarg.h>           // vastart va_end
#include <assert.h>
#include <sys/stat.h>         //mkdir
#include "blockqueue.h"
#include "../buffer/buffer.h"

/*
┌──────────────┐         ┌──────────────┐         ┌──────────────┐
│  业务线程     │  push   │  BlockDeque  │  pop    │  写日志线程   │  write   │  文件
│ (生产者)      │ ──────> │  (缓冲区)    │ ──────> │ (消费者)      │ ──────> │
└──────────────┘         └──────────────┘         └──────────────┘
*/

class Log {
private:
    Log();
    virtual ~Log();

    void AppendLogLevelTitle(int level);
    void AsyncWrite();

public:
    void init(int level, const char* path = "./log",
                const char* suffix = ".log", 
                int maxQueueCapacity = 1024);

    static Log* Instance();
    static void FlushLogThread();

    void write(int level, const char* format, ...);
    void flush();

    int GetLevel();
    void SetLevel(int level);
    bool IsOpen() {return isOpen_;}

private:
    static const int LOG_PATH_LEN = 256;
    static const int LOG_NAME_LEN = 256;
    static const int MAX_LINES = 50000;

    const char* path_;
    const char* suffix_;

    int MAX_LINES_;

    int lineCount_; // 当前文件行数
    int toDay_; // 当前天数

    bool isOpen_;

    Buffer buff_;   // 用于格式化日志内容的缓冲区
    int level_;
    bool isAsync_;  // 是否异步模式

    FILE* fp_;  // 指向当前的 .log 文件

    std::unique_ptr<BlockDeque<std::string>> deque_;
    std::unique_ptr<std::thread> writeThread_;  // 后台写线程
    std::mutex mtx_;
}; 

#define LOG_BASE(level, format, ...) \
    do {\
        Log* log = Log::Instance();\
        if (log->IsOpen() && log->GetLevel() <= level) {\
            log->write(level, format, ##__VA_ARGS__); \
            log->flush();\
        }\
    } while(0);

#define LOG_DEBUG(format, ...) do {LOG_BASE(0, format, ##__VA_ARGS__)} while(0);
#define LOG_INFO(format, ...) do {LOG_BASE(1, format, ##__VA_ARGS__)} while(0);
#define LOG_WARN(format, ...) do {LOG_BASE(2, format, ##__VA_ARGS__)} while(0);
#define LOG_ERROR(format, ...) do {LOG_BASE(3, format, ##__VA_ARGS__)} while(0);