#include "webserver.h"


WebServer::WebServer(int port, int mode, int timeoutMs, bool optLinger,
        int sqlPort, const char* sqlUser, const char* sqlPwd, const char* dbName,
        int connPoolSize, int threadPoolSize,
        bool openLog, int logLevel, int logQueueSize):
        port_(port), timeoutMs_(timeoutMs), openLinger_(optLinger),
        isClose_(false), timer_(std::make_unique<HeapTimer>()),
        threadpool_(std::make_unique<ThreadPool>(threadPoolSize)),
        epoller_(std::make_unique<Epoller>())  {
    srcDir_ = getcwd(nullptr, 256);
    assert(srcDir_);
    strncat(srcDir_, "../../resources", 20);
    HttpConn::userCount = 0;
    HttpConn::srcDir = srcDir_;
    SqlConnPool::Instance()->init("localhost", port, sqlUser, 
    sqlPwd, dbName, connPoolSize);
    initEventModel_(mode);
    if (!initSocket_()) { isClose_ = true; }

    if (openLog) {
        Log::Instance()->init(logLevel, "./log", ".log", logQueueSize);
        if(isClose_) { LOG_ERROR("========== Server init error!=========="); }
        else {
            LOG_INFO("========== Server init ==========");
            LOG_INFO("Port:%d, OpenLinger: %s", port_, optLinger? "true":"false");
            LOG_INFO("Listen Mode: %s, OpenConn Mode: %s",
                            (listenEvent_ & EPOLLET ? "ET": "LT"),
                            (connEvent_ & EPOLLET ? "ET": "LT"));
            LOG_INFO("LogSys level: %d", logLevel);
            LOG_INFO("srcDir: %s", HttpConn::srcDir);
            LOG_INFO("SqlConnPool num: %d, ThreadPool num: %d", connPoolSize, threadPoolSize);
        }
    }
}

WebServer::~WebServer() {
    isClose_ = true;
    close(listenFd_);
    free(srcDir_);
    SqlConnPool::Instance()->closePool();
}

void WebServer::start() {
    int timeMS = -1;    // epoll wait time,无事件将阻塞
    if (!isClose_) {
        LOG_INFO("========== Server start ==========");
    }
    while(!isClose_) {
        if (timeoutMs_ > 0) {
            timeMS = timer_->getNextTick();
        }
        int eventCnt = epoller_->wait(timeMS);
        for (int i = 0; i < eventCnt; i ++) {
            int fd = epoller_->getEventFd(i);
            uint32_t events = epoller_->getEvents(i);
            if (fd == listenFd_) {
                // 新连接事件
                dealListen_();
            }
            else if (events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                // 断连事件
                assert(users_.count(fd) > 0);
                dealDisconnect_(&users_[fd]);
            }
            else if (events & EPOLLIN) {
                // 输入事件
                assert(users_.count(fd) > 0);
                dealRead_(&users_[fd]);
            }
            else if (events & EPOLLOUT) {
                // 写出事件
                assert(users_.count(fd) > 0);
                dealWrite_(&users_[fd]);
            }
            else {
                // UB
                LOG_ERROR("Unexpected event!");
            }
        }
    }
}

int WebServer::setFdNonBlock(int fd) {
    assert(fd > 0);
    return fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0) | O_NONBLOCK);
}

/* Create listen fd*/
bool WebServer::initSocket_() {
    int ret;
    struct sockaddr_in addr;

    if(port_ > 65535 || port_ < 1024) {
        LOG_ERROR("Port:%d error!",  port_);
        return false;
    }
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htonl(port_);

    struct linger optLinger = {0};
    if (openLinger_) {
        // 使用优雅关闭：直到所剩数据发完或超时
        optLinger.l_onoff = 1;
        optLinger.l_linger = 1;
    }

    listenFd_ = socket(AF_INET, SOCK_STREAM, 0);
    if(listenFd_ < 0) {
        LOG_ERROR("Create socket error!", port_);
        return false;
    }

    // 设置优雅关闭
    ret = setsockopt(listenFd_, SOL_SOCKET, SO_LINGER,
                 &optLinger, sizeof(optLinger));
    if(ret < 0) {
        close(listenFd_);
        LOG_ERROR("Init linger error!", port_);
        return false;
    }

    int optval = 1;
    // 允许地址复用(允许绑定到处于 TIME_WAIT 状态的地址)
    ret = setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR,
    (const void*)&optval, sizeof(int));
    if(ret == -1) {
        LOG_ERROR("set socket setsockopt error !");
        close(listenFd_);
        return false;
    }
    
    ret = bind(listenFd_, (sockaddr*)&addr, sizeof(addr));
    if(ret < 0) {
        LOG_ERROR("Bind Port:%d error!", port_);
        close(listenFd_);
        return false;
    }

    ret = listen(listenFd_, 6); // TODO,n设置太小？
    if(ret < 0) {
        LOG_ERROR("Listen port:%d error!", port_);
        close(listenFd_);
        return false;
    }

    ret = epoller_->addFd(listenFd_, listenEvent_ | EPOLLIN);
    if(ret == 0) {
        LOG_ERROR("Add listen fd error!");
        close(listenFd_);
        return false;
    }
    setFdNonBlock(listenFd_);
    LOG_INFO("Server port:%d", port_);
    return true;
}

void WebServer::initEventModel_(int mode) {
    listenEvent_ = EPOLLRDHUP;  // 监听对端关闭
    connEvent_ = EPOLLONESHOT | EPOLLRDHUP; // ONESHOT 确保一个socket在任一时刻只被一个线程处理
    switch (mode) {
        case 0: break;
        case 1: 
            connEvent_ |= EPOLLET;
            break;
        case 2:
            listenEvent_ |= EPOLLET;
            break;
        case 3:
            listenEvent_ |= EPOLLET;
            connEvent_ |= EPOLLET;
            break;
        default:
            listenEvent_ |= EPOLLET;
            connEvent_ |= EPOLLET;
            break;
    }
    HttpConn::isET = (connEvent_ & EPOLLET);
}

void WebServer::dealListen_() {
    struct sockaddr_in clientAddr;
    socklen_t addrLen = sizeof(clientAddr);
    do {
        int fd = accept(listenFd_, (sockaddr*)&clientAddr, &addrLen);
        if (fd < 0) return;
        else if (HttpConn::userCount >= MAX_FD) {
            sendError_(fd, "Server is busy!");
            LOG_WARN("Server is full!");
            return;
        }
        addClient_(fd, clientAddr);
    } while (listenEvent_ & EPOLLET);   // why
}

void WebServer::dealDisconnect_(HttpConn* client) {
    assert(client);
    LOG_INFO("Client[%d] quit!", client->getFd());
    epoller_->delFd(client->getFd());
    client->closeConn();
}

void WebServer::dealRead_(HttpConn* client) {
    assert(client);
    extendTime_(client);
    threadpool_->addTask(std::bind(&WebServer::onRead_, this, client));
}

void WebServer::dealWrite_(HttpConn* client) {
    assert(client);
    extendTime_(client);
    threadpool_->addTask(std::bind(&WebServer::onWrite_, this, client));
}

void WebServer::onRead_(HttpConn* client) {
    assert(client);
    int ret = -1;
    int readErrno = 0;
    ret = client->read(&readErrno);
    if (ret <= 0 && readErrno != EAGAIN) {
        dealDisconnect_(client);
        return;
    }
    onProcess(client);

}

void WebServer::onWrite_(HttpConn* client) {
    assert(client);
    int ret = -1;
    int writeErrno = 0;
    ret = client->write(&writeErrno);
    if (client->toWriteBytes() == 0) {
        if (client->isKeepAlive()) {
            onProcess(client);
            return;
        }
    }
    else if (ret < 0) {
        if (writeErrno == EAGAIN) {
            epoller_->modFd(client->getFd(), connEvent_ | EPOLLOUT);
            return;
        }
    }
    dealDisconnect_(client);
}

void WebServer::onProcess(HttpConn* client) {
    if (client->process()) {
        epoller_->modFd(client->getFd(), connEvent_ | EPOLLOUT);
    }
    else {
        epoller_->modFd(client->getFd(), connEvent_ | EPOLLIN);
    }
}

void WebServer::addClient_(int fd, struct sockaddr_in clientAddr) {
    assert(fd > 0);
    users_[fd].initConn(fd, clientAddr);
    if (timeoutMs_ > 0) {
        timer_->add(fd, timeoutMs_,
            std::bind(&WebServer::dealDisconnect_, this, &users_[fd]));
    }
    epoller_->addFd(fd, connEvent_ | EPOLLIN);
    setFdNonBlock(fd);
    LOG_INFO("Client[%d] in!", users_[fd].getFd());
}

void WebServer::sendError_(int fd, const std::string& message) {

}

void WebServer::extendTime_(HttpConn* client) {
    assert(client);
    if (timeoutMs_ > 0) {
        timer_->adjust(client->getFd(), timeoutMs_);
    }
}