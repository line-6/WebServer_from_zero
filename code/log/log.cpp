#include "log.h"
#include "blockqueue.h"
#include <cstdio>
#include <memory>
#include <mutex>

Log::Log() {
    lineCount_ = 0;
    isAsync_ = false;
    writeThread_ = nullptr;
    deque_ = nullptr;
    toDay_ = 0;
    fp_ = nullptr;
}

Log::~Log() {
    if (writeThread_ && writeThread_->joinable()) {
        while(!deque_->empty()) {
            deque_->flush();
        }
        deque_->close();
        writeThread_->join();
    }
    if (fp_) {
        std::lock_guard<std::mutex> locker(mtx_);
        flush();
        fclose(fp_);
    }
}

int Log::GetLevel() {
    std::lock_guard<std::mutex> locker(mtx_);
    return level_;
}

void Log::SetLevel(int level) {
    std::lock_guard<std::mutex> locker(mtx_);
    level_ = level;
}

void Log::init(int level = 1, const char* path, const char* suffix, int maxQueueCapacity) {
    isOpen_ = true;
    level = level_;
    if (maxQueueCapacity > 0) {
        isAsync_ = true;
        if (!deque_) {
            auto newDeque = std::make_unique<BlockDeque<std::string>>();
            deque_ = move(newDeque);

            auto newThread = std::make_unique<std::thread>(FlushLogThread);
            writeThread_ = move(newThread);
        }
    }
    else {
        isAsync_ = false;
    }

    lineCount_ = 0;
    time_t timer = time(nullptr);
    struct tm *sysTime = localtime(&timer);
    struct tm t = *sysTime;
    path_ = path;
    suffix_ = suffix;
    char fileName[LOG_NAME_LEN] = {0};
    snprintf(fileName, LOG_NAME_LEN - 1, "%s/%04d_%02d_%02d%s",
            path_, t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, suffix_);
    toDay_ = t.tm_mday;

    {
        std::lock_guard<std::mutex> locker(mtx_);
        buff_.RetrieveAll();
        if(fp_) {
            flush();
            fclose(fp_);
        }
        fp_ = fopen(fileName, "a");
        if (fp_ == nullptr) {
            mkdir(path_, 0777);
            fp_ = fopen(fileName, "a");
        }
        assert(fp_ != nullptr);
    }
}

void Log::write(int level, const char *format, ...) {
    // 获取时间戳
    struct timeval now = {0, 0};
    gettimeofday(&now, nullptr);
    time_t tSec = now.tv_sec;
    struct tm* sysTime = localtime(&tSec);
    struct tm t = *sysTime;
    va_list vaList;

    // 新建文件检测
    // 跨天 or 行数超过最大限制
    if (toDay_ != t.tm_mday || (lineCount_ && (lineCount_ % MAX_LINES == 0))) {
        std::unique_lock<std::mutex> locker(mtx_);
        locker.unlock();

        char newFile[LOG_NAME_LEN]; // 存储完整的日志文件路径（目录路径 + 文件名 + 后缀）
        char tail[36] = {0};    // 存储格式化的日期字符串: YYYY_MM_DD
        snprintf(tail, 36, "%04d_%02d_%02d", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);

        if (toDay_ != t.tm_mday) {
            snprintf(newFile, LOG_NAME_LEN - 72, "%s/%s%s", path_, tail, suffix_);
            // warning：这里未加锁，修改了共享变量
            toDay_ = t.tm_mday;
            lineCount_ = 0;
        }
        else {
            snprintf(newFile, LOG_NAME_LEN - 72, "%s/%s-%d%s", 
            path_, tail, (lineCount_  / MAX_LINES), suffix_);
        }

        locker.lock();
        flush();
        fclose(fp_);    // 关闭旧文件
        fp_ = fopen(newFile, "a");  // 打开新文件
        assert(fp_ != nullptr);
    }

    // 将日志内容格式化到 Buffer
    {
        std::lock_guard<std::mutex> locker(mtx_);
        lineCount_++;
        // YYYY-MM-DD HH:MM:SS.uuuuuu (精确的微秒)
        // e.g.: 2025-12-14 15:42:10.267310
        int n = snprintf(buff_.WritePtr(), 128, "%d-%02d-%02d %02d:%02d:%02d.%06ld ",
                    t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
                    t.tm_hour, t.tm_min, t.tm_sec, now.tv_usec);
        buff_.HasWritten(n);
        // e.g.: 2025-12-14 15:42:10.267310 [debug]: 
        AppendLogLevelTitle(level);

        va_start(vaList, format);
        // e.g.: 2025-12-14 15:42:10.267310 [debug]: This is a debug message in sync mode
        int m = vsnprintf(buff_.WritePtr(), buff_.WritableBytes(), format, vaList);
        va_end(vaList);

        buff_.HasWritten(m);
        buff_.Append("\n\0", 2);

        // 同步/异步模式 分支点
        if(isAsync_ && deque_ && !deque_->full()) {
            deque_->push_back(buff_.RetrieveAllToStr());
        } else {
            fputs(buff_.ReadPtr(), fp_);
        }
        buff_.RetrieveAll();
    }
}

void Log::AppendLogLevelTitle(int level) {
    switch(level) {
        case 0:
            buff_.Append("[debug]: ", 9);
            break;
        case 1:
            buff_.Append("[info]: ", 9);
            break;
        case 2:
            buff_.Append("[warn]: ", 9);
            break;
        case 3:
            buff_.Append("[error]: ", 9);
            break;
        default:
            buff_.Append("[debug]: ", 9);
            break;
    }
}

void Log::flush() {
    if (isAsync_) {
        deque_->flush();
    }
    fflush(fp_);
}

void Log::AsyncWrite() {
    std::string str = "";
    while(deque_->pop(str)) {
        std::lock_guard<std::mutex> locker(mtx_);
        fputs(str.c_str(), fp_);
    }
}

Log* Log::Instance() {
    static Log instance;
    return &instance;
}

void Log::FlushLogThread() {
    Log::Instance()->AsyncWrite();
}