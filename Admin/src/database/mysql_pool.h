/**********************************************************************
** class name:  MysqlDbPool
**
** description: Mysql连接池
**
** author: Leopard-C
**
** update: 2020/12/02
**********************************************************************/
#pragma once
#include <string>
#include <deque>
#include <mutex>
#ifdef _WIN32
#  include <mysql.h>
#else
#  include <mysql/mysql.h>
#endif


#define _ENABLE_MYSQL_SPDLOG
#define _ENABLE_MYSQL_JSONCPP


class MysqlDbPool {
public:
    MysqlDbPool(const std::string& host, unsigned int port, const std::string& user,
        const std::string& pwd, const std::string& db_name, int pool_size);

    /* 从Json文件读取连接信息 */
    MysqlDbPool(const std::string& configfile);

    ~MysqlDbPool();

    bool good() const { return ok_; }
    bool bad()  const { return !ok_; }

    MYSQL *getConnection();
    bool freeConnection(MYSQL *mysql);

private:
    int init();
    void clear();
    int connect();

    bool readConfig(const std::string& filename);

private:
    bool         ok_;
    int          pool_size_;
    unsigned int port_;
    std::string  host_;
    std::string  user_;
    std::string  password_;
    std::string  db_name_;
    std::mutex   mutex_;
    std::deque<MYSQL*> pool_;
};

