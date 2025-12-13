#include "buffer.h"
#include <cerrno>
#include <cstddef>
#include <sys/types.h>

Buffer::Buffer(int initBufferSize):buffer_(initBufferSize), 
            readPos_(0), writePos_(0) {}

size_t Buffer::WriteableBytes() const {
    return buffer_.size() - writePos_;
}

size_t Buffer::ReadableBytes() const {
    return writePos_ - readPos_;
}

size_t Buffer::PrependBytes() const {
    return readPos_;
}

void Buffer::MakeSpace_(size_t len) {
    // 扩容
    if (WriteableBytes() + PrependBytes() < len) {
        buffer_.resize(writePos_ + len + 1);
    }
    // 搬移
    else {
        size_t readable = ReadableBytes();
        std::copy(BeginPtr_() + readPos_, BeginPtr_() + writePos_, BeginPtr_());
        readPos_ = 0;
        writePos_ = readPos_ + readable;
        assert(readable == ReadableBytes());
    }
}

void Buffer::EnsureWriteable(size_t len) {
    // [writePos_, buffer_.size()]容量不够，可以检查[0, readPos_]位置，这是一段已经
    // 被读过的位置，可以让后面的数据位置总体前移
    // readPos_ -> 0, writePos_ -> writePos_ - readPos_
    // 若是移动后容量仍不够，再考虑真正的resize
    if (WriteableBytes() < len) {
        MakeSpace_(len);
    }
    assert(WriteableBytes() >= len);
}

void Buffer::HasWritten(size_t len) {
    writePos_ += len;
}

void Buffer::Retrieve(size_t len) {
    assert(len <= ReadableBytes());
    readPos_ += len;
}

void Buffer::RetrieveUntil(const char* end) {
    assert(ReadPtr() <= end);
    Retrieve(end - ReadPtr());
}

void Buffer::RetrieveAll() {
    bzero(&buffer_[0], buffer_.size());
    readPos_ = 0;
    writePos_ = 0;
}

std::string Buffer::RetrieveAllToStr() {
    std::string str(ReadPtr(), ReadableBytes());
    RetrieveAll();
    return str;
}

void Buffer::Append(const char *str, size_t len) {
    assert(str);
    EnsureWriteable(len);
    std::copy(str, str + len, WritePtr());
    HasWritten(len);
}

/*
最大程度减少系统调用（read）的次数：
构造两个 iovec -> iovec[0] 指向buffer的空闲区域 -> iovec[1] 指向一个临时分配的栈上数组 ->
readv 读取 fd（只需调用一次readv） -> 数据读入iovec -> iovec[1] 合并到 buffer 中
*/
ssize_t Buffer::ReadFd(int fd, int* Errno) {
    char temp_buf[65535];
    struct iovec iov[2];
    const size_t writeable = WriteableBytes();

    iov[0].iov_base = WritePtr();
    iov[0].iov_len = writeable;
    iov[1].iov_base = temp_buf;
    iov[1].iov_len = sizeof(temp_buf);

    /* 分散读 */
    const ssize_t len = readv(fd, iov, 2);
    if (len < 0) {
        *Errno =  errno;
    }
    else if (static_cast<size_t>(len) <= writeable) {
        writePos_ += len;
    }
    else {
        writePos_ = buffer_.size();
        Append(temp_buf, len - writeable);
    }
    return len;
}

ssize_t Buffer::WriteFd(int fd, int* Errno) {
    ssize_t readable = ReadableBytes();
    ssize_t len = write(fd, ReadPtr(), readable);
    if (len < 0) {
        *Errno = errno;
        return len;
    }
    readPos_ += len;
    return len;
}