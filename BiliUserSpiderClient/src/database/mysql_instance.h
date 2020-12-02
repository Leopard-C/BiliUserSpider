/*****************************************************************************************
** class name:  MysqlInstance
**
** description: 封装libmysql，方便调用。只能从MysqlDbPool对象中获取MYSQL连接对象
**
** author: Leopard-C
**
** update: 2020/12/02
*****************************************************************************************/
#pragma once
#include <string>
#include "mysql_pool.h"


class MysqlInstance {
public:
    MysqlInstance(MysqlDbPool* mysqlDbPool_);
    ~MysqlInstance();

    // force free connection
    // not recommended
    void free();

    bool good() const { return mysql_ != nullptr; }
    bool bad()  const { return mysql_ == nullptr; }

    bool setEnabled(bool b) { enable_ = b; }
    bool isEnabled() const  { return enable_; }
    bool isDisabled() const { return !enable_; }

    bool exec(const char* sql, int errorCount = 1);

    MYSQL_RES* getResult() const { return result_; }

    MYSQL_ROW fetchRow() const { return mysql_fetch_row(result_); }

    uint64_t affectedRows() const { return mysql_affected_rows(mysql_); }

    uint64_t numRows()   const { return mysql_num_rows(result_); }
    unsigned int numFields() const { return mysql_num_fields(result_); }

    std::string escapeString(const std::string& str);
    unsigned long escapeString(char* to, const char* from, unsigned long len);

private:
    MysqlDbPool* pool_ = nullptr;
    MYSQL* mysql_      = nullptr;
    MYSQL_RES* result_ = nullptr;
    bool enable_       = true;
};

