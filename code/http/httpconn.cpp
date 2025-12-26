#include "httpconn.h"

const char* HttpConn::srcDir;
std::atomic<int> HttpConn::userCount;
bool HttpConn::isET;

HttpConn::HttpConn() {
    fd_ = -1;
    addr_ = {0};
    isClose_ = true;
}

HttpConn::~HttpConn() {
    closeConn();
}

void HttpConn::initConn(int sockFd, const sockaddr_in& addr) {
    assert(sockFd > 0);
    userCount ++;
    addr_ = addr;
    fd_ = sockFd;
    writeBuffer_.RetrieveAll();
    readBuffer_.RetrieveAll();
    isClose_ = false;
    LOG_INFO("Client[%d](%s:%d) in, userCount:%d", fd_,
            getIP(), getPort(), (int)userCount);
}

void HttpConn::closeConn() {
    response_.unmapFile();
    if (isClose_ == false) {
        isClose_ = true; 
        userCount--;
        close(fd_);
        LOG_INFO("Client[%d](%s:%d) quit, UserCount:%d", fd_, getIP(), getPort(), (int)userCount);
    }
}

ssize_t HttpConn::read(int* saveError) {
    ssize_t len = -1;
    do {
        // 从 fd 读取数据到 buffer
        len = readBuffer_.ReadFd(fd_, saveError);
        if (len < 0) break;
    } while(isET);

    return len;
}

ssize_t HttpConn::write(int* saveError) {
    ssize_t len = -1;
    do {
        len = writev(fd_, iov_, iovCnt_);
        if(len <= 0) {
            *saveError = errno;
            break;
        }
        if (iov_[0].iov_len + iov_[1].iov_len == 0) { break; }    // 传输结束
        else if (static_cast<size_t>(len) > iov_[0].iov_len) {
            // iov[0] 全部写，iov[1] 部分写
            iov_[1].iov_base = (uint8_t*) iov_[1].iov_base + (len - iov_[0].iov_len);
            iov_[1].iov_len -= (len - iov_[0].iov_len);
            if (iov_[0].iov_len) {
                writeBuffer_.RetrieveAll(); // what
                iov_[0].iov_len = 0;
            }
        }
        else {
            // iov[0] 部分写
            iov_[0].iov_base = (uint8_t*)iov_[0].iov_base + len;
            iov_[0].iov_len -= len;
            writeBuffer_.Retrieve(len); // what
        }
    } while( isET || toWriteBytes() > 10240);   // 当采用LT模式，只有当待发送数据 > 10KB 才循环 what
    return len;
}

bool HttpConn::process() {
    request_.init();
    if (readBuffer_.ReadableBytes() <= 0) {
        return false;
    }
    else if (request_.parse(readBuffer_)) {
        LOG_DEBUG("%s", request_.path().c_str());
        response_.init(srcDir, request_.path(),
                request_.IsKeepAlive(), 200);
    }
    else {
        response_.init(srcDir, request_.path(), false, 400);        
    }

    response_.makeResponse(writeBuffer_);
    // 响应头
    iov_[0].iov_base = const_cast<char*>(writeBuffer_.ReadPtr());
    iov_[0].iov_len = writeBuffer_.ReadableBytes();
    iovCnt_ = 1;
    
    // 文件
    if (response_.fileLen() > 0 && response_.file()) {
        iov_[1].iov_base = response_.file();
        iov_[1].iov_len = response_.fileLen();
        iovCnt_ = 2;
    }
    LOG_DEBUG("filesize:%d, %d  to %d", response_.fileLen() , iovCnt_, toWriteBytes());
    return true;
}