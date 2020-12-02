#include "./mysql_pool.h"
#include "../log/logger.h"
#include "../json/json.h"
#include <thread>
#include <fstream>

MysqlDbPool::MysqlDbPool(const std::string& host, unsigned int port, const std::string& user,
    const std::string& pwd, const std::string& db_name, int pool_size) :
    pool_size_(pool_size), port_(port), host_(host), user_(user),
    password_(pwd), db_name_(db_name)
{
    if (pool_size_ > 0 && init() == pool_size_) {
        ok_ = true;
    }
    else {
        ok_ = false;
    }
    LInfo("Mysql pool init successfully");
}

MysqlDbPool::MysqlDbPool(const std::string& configfile) {
    ok_ = readConfig(configfile);
    if (!ok_) {
        // any one connection failed
        // return false
        clear();
    }
    LInfo("Mysql pool init successfully");
}

MysqlDbPool::~MysqlDbPool() {
    clear();
}


/*
 *    从JSON配置文件中读取数据库连接信息, 需要具有以下key:
 *    host, port, user, password, db_name, pool_size
 */
bool MysqlDbPool::readConfig(const std::string& filename) {
    std::ifstream ifs(filename);
    if (!ifs) {
        return false;
    }

    bool ret = false;
    Json::Reader reader;
    Json::Value  root;

    do {
        if (!reader.parse(ifs, root, false)) {
            break;
        }
        if (checkNull(root, "host", "port", "user", "password", "db_name", "pool_size")) {
            break;
        }
        host_ = root["host"].asString();
        user_ = root["user"].asString();
        password_ = root["password"].asString();
        db_name_ = root["db_name"].asString();
        port_ = root["port"].asUInt();
        pool_size_ = root["pool_size"].asInt();

        ret = true;
    } while (false);

    if (ret) {
        if (pool_size_ < 1) {
            LError("key:pool_size should be 1 at least");
            return false;
        }
        return (init() == pool_size_);
    }
    else {
        return false;
    }
}

int MysqlDbPool::init() {
    int count = 0;
    for (int i = 0; i < pool_size_; ++i) {
        if (connect() == 0) {
            ++count;
        }
        else {
            return count;
        }
    }
    return count;
}

void MysqlDbPool::clear() {
    if (!pool_.empty()) {
        for (auto& mysql : pool_) {
            if (mysql) {
                mysql_close(mysql);
                mysql = nullptr;
            }
        }
        std::deque<MYSQL*>().swap(pool_);
    }
}

int MysqlDbPool::connect() {
    int ret = 0;
    MYSQL *mysql = mysql_init(NULL);
    int value = true;
    mysql_options(mysql, MYSQL_OPT_RECONNECT, (char *)&value);
    mysql_options(mysql, MYSQL_SET_CHARSET_NAME, "utf8");
    MYSQL *status =
        mysql_real_connect(mysql, host_.c_str(), user_.c_str(), password_.c_str(),
            db_name_.c_str(), port_, NULL, CLIENT_MULTI_STATEMENTS);
    if (NULL == status || mysql == NULL) {
        LError("Mysql Error: {}", mysql_error(mysql));
        ret = 1;
    }
    else {
        pool_.push_back(mysql);
    }
    return ret;
}

MYSQL* MysqlDbPool::getConnection() {
    std::lock_guard<std::mutex> lockGuard(mutex_);
    while (pool_.empty()) {
        std::this_thread::sleep_for(std::chrono::microseconds(1));
    }
    MYSQL *mysql = pool_.front();
    pool_.pop_front();
    return mysql;
}

bool MysqlDbPool::freeConnection(MYSQL *mysql) {
    if(!mysql) {
        return false;
    }
    std::lock_guard<std::mutex> guard(mutex_);
    // avoid push back to pool twice
    for (auto it = pool_.begin(); it != pool_.end(); ++it) {
        if (mysql == *it) {
            return false;
        }
    }
    pool_.push_back(mysql);
    return true;
}

