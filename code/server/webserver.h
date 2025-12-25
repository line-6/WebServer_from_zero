#pragma once

#include <unordered_map>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "epoller.h"
#include "../log/log.h"
#include "../timer/heaptimer.h"
#include "../pool/sqlconnpool.h"
#include "../pool/threadpool.h"
#include "../pool/sqlconnRAII.h"
#include "../http/httpconn.h"

class WebServer {
public:
    WebServer(int port, int mode, int timeoutMs, bool optLinger,    // 端口，ET模式，timeoutMs， 优雅退出
        int sqlPort, const char* sqlUser, const char* sqlPwd, const char* dbName,  // Mysql配置
        int connPoolSize, int threadPoolSize,   // 连接池，线程池大小 
        bool openLog, int logLevel, int logQueueSize); // 日志开关 日志等级 日志异步队列容量
    ~WebServer();

    void start();

    int setFdNonBlock(int fd);

private:
    void initEventModel_(int mode);

    bool initSocket_();

    static const int MAX_FD = 65536;

    int port_;
    int timeoutMs_;
    bool openLinger_;   // socket优雅关闭
    bool isClose_;
    int listenFd_;
    char* srcDir_;

    uint32_t listenEvent_;  // listen fd对应的events
    uint32_t connEvent_;

    std::unique_ptr<HeapTimer> timer_;
    std::unique_ptr<ThreadPool> threadpool_;
    std::unique_ptr<Epoller> epoller_;
    std::unordered_map<int, HttpConn> user_;
};