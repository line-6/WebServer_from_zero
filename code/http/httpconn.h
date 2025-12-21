#pragma once

#include <arpa/inet.h>

#include "../buffer/buffer.h"

class HttpConn {
public:

private:
    int fd_;
    struct sockaddr_in addr_;

    bool isClose_;

    int iovCnt_;
    struct iovec iov_[2];

    Buffer readBuf_;
    Buffer writeBuf;
};