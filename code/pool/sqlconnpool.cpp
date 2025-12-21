#include "sqlconnpool.h"
#include <mutex>
#include <mysql/mysql.h>
#include <semaphore.h>

SqlConnPool::SqlConnPool() {
    useCount_ = 0;
    freeCount_ = 0;
}

SqlConnPool::~SqlConnPool() {
    closePool();
}

SqlConnPool* SqlConnPool::Instance() {
    static SqlConnPool pool;
    return &pool;
}

void SqlConnPool::init(const char* host, int port,
              const char* user,const char* pwd, 
              const char* dbName, int connSize) {
    assert(connSize > 0);
    for(int i = 0; i < connSize; i ++) {
        MYSQL* sql = nullptr;
        sql = mysql_init(sql);
        if (!sql) {
            LOG_ERROR("Mysql init error!");
            assert(sql);
        }
        sql = mysql_real_connect(sql, host, user,
                                pwd, dbName, port,
                                 nullptr, 0);
        if (!sql) {
            LOG_ERROR("MySql Connect error!");
        }
        connQue_.push(sql);
    }
    MAX_CONN_ = connSize;
    sem_init(&semId_, 0, MAX_CONN_);
}

void SqlConnPool::closePool() {
    std::lock_guard<std::mutex> locker(mtx_);
    while(!connQue_.empty()) {
        auto item = connQue_.front();
        connQue_.pop();
        mysql_close(item);
    }
    mysql_library_end();
}

MYSQL* SqlConnPool::getConn() {
    MYSQL *sql = nullptr;
    if(connQue_.empty()){
        LOG_WARN("SqlConnPool busy!");
        return nullptr;
    }
    sem_wait(&semId_);
    {
        std::lock_guard<std::mutex> locker(mtx_);
        sql = connQue_.front();
        connQue_.pop();
    }
    return sql;
}

void SqlConnPool::freeConn(MYSQL* conn) {
    assert(conn);
    std::lock_guard<std::mutex> locker(mtx_);
    connQue_.push(conn);
    sem_post(&semId_);
}

int SqlConnPool::getFreeConnCount() {
    std::lock_guard<std::mutex> locker(mtx_);
    return connQue_.size();
}