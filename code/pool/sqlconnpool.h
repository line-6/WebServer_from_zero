#pragma once

#include <mysql/mysql.h>
#include <string>
#include <queue>
#include <mutex>
#include <semaphore.h>
#include <thread>
#include "../log/log.h"

class SqlConnPool {
public:
    void init(const char* host, int port,
              const char* user,const char* pwd, 
              const char* dbName, int connSize);
    
    void closePool();

    static SqlConnPool* Instance();
    
    MYSQL* getConn();   //  从池中获取一个连接
    
    void freeConn(MYSQL* conn); // 向池中归还一个连接
    
    int getFreeConnCount();

private:
    SqlConnPool();
    ~SqlConnPool();

    int MAX_CONN_;
    int useCount_;
    int freeCount_;

    std::queue<MYSQL *> connQue_;
    std::mutex mtx_;
    sem_t semId_;
};