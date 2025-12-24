#pragma once

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>     // readv/writev
#include <arpa/inet.h>
#include <stdlib.h>      // atoi()
#include <errno.h> 
#include <assert.h>

#include "../log/log.h"
#include "../pool/sqlconnRAII.h"
#include "../buffer/buffer.h"
#include "httprequest.h"
#include "httpresponse.h"

class HttpConn {
public:
    HttpConn();
    ~HttpConn();

    void initConn(int sockFd, const sockaddr_in& addr);
    void closeConn();

    ssize_t read(int* saveError);
    ssize_t write(int* saveError);

    int getFd() const { return fd_; }
    const char* getIP() const { return inet_ntoa(addr_.sin_addr); }
    int getPort() const { return addr_.sin_port; }
    struct sockaddr_in getAddr() const { return addr_; }

    bool process();

    int toWriteBytes() { return iov_[0].iov_len + iov_[1].iov_len; }

    bool isKeepAlive() const { return request_.IsKeepAlive(); }

    static bool isET;
    static const char* srcDir;
    static std::atomic<int> userCount;
private:
    int fd_;
    struct sockaddr_in addr_;   // 客户端地址信息

    bool isClose_;

    int iovCnt_;
    struct iovec iov_[2];   // iov_[0]: 响应头, iov_[1]: 响应体(文件)

    Buffer readBuffer_;
    Buffer writeBuffer_;

    HttpRequest request_;
    HttpResponse response_;
};