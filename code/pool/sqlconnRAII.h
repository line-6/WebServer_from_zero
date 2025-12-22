#pragma once

#include "sqlconnpool.h"
#include <cassert>

/* 
用于从数据库连接池获取一个连接
使用方法：
    SqlConnRAII raii(pool);
    MYSQL* sql = raii.get();
为什么不直接 MYSQL* sql = SqlConnPool::Instance()->getConn() ?
不符合 RAII 思想，必须手动归还：SqlConnPool::Instance()->FreeConn(sql);
*/
class SqlConnRAII {
public:
    SqlConnRAII(SqlConnPool* pool): sql_(pool->getConn()), pool_(pool) {}

    ~SqlConnRAII() {
        if (sql_) {
            pool_->freeConn(sql_);
        }
    }

    MYSQL* get() const {return sql_;}

private:
    MYSQL* sql_;
    SqlConnPool* pool_;
};