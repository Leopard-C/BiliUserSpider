#include "mysql_instance.h"
#include "mysql_pool.h"
#include "../log/logger.h"


MysqlInstance::MysqlInstance(MysqlDbPool* mysqlDbPool_) {
    pool_ = mysqlDbPool_;
    if (pool_) {
        mysql_ = pool_->getConnection();
    }
}

MysqlInstance::~MysqlInstance() {
    free();
}

/* 不需要手动调用， RAII思想！ */
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

bool MysqlInstance::exec(const char* sql, int errorCount/* = 1*/) {
    if (result_) {
        mysql_free_result(result_);
        result_ = nullptr;
    }
    for (int i = 0; i < errorCount; ++i) {
        if (mysql_query(mysql_, sql) == 0) {
            result_ = mysql_store_result(mysql_);
            return true;
        }
        else {
            LError("Exec sql failed: {}", sql);
            LError("Mysql Error: {}", mysql_error(mysql_));
        }
    }
    return false;
}

/* 对于含有特殊字符的字符串进行编码 */
std::string MysqlInstance::escapeString(const std::string& str) {
    if (str.empty()) {
        return std::string();
    }
    char* tmpOut = new char[str.length() * 2 + 1];
    unsigned long len = mysql_real_escape_string(mysql_, tmpOut, str.c_str(), static_cast<unsigned long>(str.length()));
    std::string out(tmpOut, static_cast<size_t>(len));
    delete[] tmpOut;
    return out;
}

unsigned long MysqlInstance::escapeString(char* to, const char* from, unsigned long len) {
    return mysql_real_escape_string(mysql_, to, from ,len);
}

