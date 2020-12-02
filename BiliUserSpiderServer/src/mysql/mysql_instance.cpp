#include "mysql_instance.h"
#include "mysql_pool.h"
#include "../log/logger.h"

MysqlInstance::MysqlInstance(MysqlDbPool* pool) {
    pool_ = pool;
    if (pool_) {
        mysql_ = pool_->getConnection();
    }
}

MysqlInstance::~MysqlInstance() {
    free();
}

void MysqlInstance::free() {
    if (result_) {
        mysql_free_result(result_);
        result_ = nullptr;
    }
    if (mysql_) {
        pool_->freeConnection(mysql_);
        mysql_ = nullptr;
    }
}


bool MysqlInstance::exec(const char* sql) {
    if (result_) {
        mysql_free_result(result_);
        result_ = nullptr;
    }
    if (mysql_query(mysql_, sql) != 0) {
        LError("Exec sql failed: {}", sql);
        LError("Mysql Error: {}", mysql_error(mysql_));
        return false;
    }
    result_ = mysql_store_result(mysql_);
    return true;
}

std::string MysqlInstance::escapeString(const std::string& str) {
    if (str.empty()) {
        return std::string();
    }
    char* tmpOut = new char[str.length() * 2 + 1];
    unsigned long len = mysql_real_escape_string(mysql_, tmpOut, str.c_str(), str.length());
    tmpOut[len] = 0;
    std::string out(tmpOut);
    delete[] tmpOut;
    return out;
}

unsigned long MysqlInstance::escapeString(char* to, const char* from, unsigned long len) {
    return mysql_real_escape_string(mysql_, to, from ,len);
}

