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
#include "./mysql_pool.h"

class MysqlInstance {
public:
    MysqlInstance(MysqlDbPool* pool);
    ~MysqlInstance();

    // force free connection
    // not recommended
    void free();

    bool good() const { return mysql_ != nullptr; }
    bool bad()  const { return mysql_ == nullptr; }

    void setEnabled(bool b) { enable_ = b; }
    bool isEnabled() const  { return enable_; }
    bool isDisabled() const { return !enable_; }

    bool exec(const char* sql);

    MYSQL_RES* getResult() const { return result_; }

    MYSQL_ROW fetchRow() const { return mysql_fetch_row(result_); }

    size_t affectedRows() const { return mysql_affected_rows(mysql_); }

    size_t numRows()   const { return mysql_num_rows(result_); }
    size_t numFields() const { return mysql_num_fields(result_); }

    std::string escapeString(const std::string& str);
    unsigned long escapeString(char* to, const char* from, unsigned long len);

private:
    MysqlDbPool* pool_;
    MYSQL* mysql_      = nullptr;
    MYSQL_RES* result_ = nullptr;
    bool enable_       = true;
};

