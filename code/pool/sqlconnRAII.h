#pragma once

#include "sqlconnpool.h"
#include <cassert>

class SqlConnRAII {
public:
    /*
        使用方法：
        SqlConnRAII raii(pool);
        MYSQL* conn = raii.get();
    */
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