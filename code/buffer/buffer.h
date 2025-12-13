#pragma once
#include <cstddef>
#include <cstring>   //perror
#include <iostream>
#include <sys/types.h>
#include <unistd.h>  // write
#include <sys/uio.h> //readv
#include <vector> //readv
#include <atomic>
#include <assert.h>

class Buffer {
/*
| 预留空间 (Prepend) |  已读数据 (Readable)  |  可写空间 (Writable)  |
^                    ^                       ^                       ^
0                 readPos_                writePos_             size()
*/
public:
    Buffer(int initBufferSize = 1024);
    ~Buffer() = default;

    size_t WritableBytes() const;
    size_t ReadableBytes() const;
    size_t PrependBytes() const;

    const char* ReadPtr() const {return BeginPtr_() + readPos_;}
    char* WritePtr() { return BeginPtr_() + writePos_; }

    void EnsureWriteable(size_t len);
    void HasWritten(size_t len);

    void Retrieve(size_t len);
    void RetrieveUntil(const char* end);
    void RetrieveAll();
    std::string RetrieveAllToStr();

    void Append(const char* str, size_t len);
    void Append(const std::string& str);

    ssize_t ReadFd(int fd, int* Errno);
    ssize_t WriteFd(int fd, int* Errno);


private:
    char* BeginPtr_() { return &*buffer_.begin(); }
    const char* BeginPtr_() const { return &*buffer_.begin(); }

    void MakeSpace_(size_t len);

    std::vector<char> buffer_;
    // atomic 保证线程安全的pos参数访问
    std::atomic<std::size_t> readPos_;
    std::atomic<std::size_t> writePos_;
};