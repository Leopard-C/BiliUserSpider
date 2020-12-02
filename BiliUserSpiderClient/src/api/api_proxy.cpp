#include "api_proxy.h"
#include "../log/logger.h"
#include "../database/mysql_instance.h"
#include "../util/string_util.h"


/*
 *   和数据库通信, 获取/修改代理IP信息
**/


/* 从数据库获取一个随机的代理 */
/* 弃用 */
int api_get_random_proxy(MysqlInstance& mysql, ic::ProxyData* proxyData, std::string* errMsg) {
    int ret = -1;
    do {
        if (mysql.bad()) {
            *errMsg = "Mysql instance is bad";
            LError("Mysql instance is bad");
            break;
        }
        const char* sql = "SELECT `host`,`port` FROM proxy ORDER BY rand() LIMIT 1;";
        if (!mysql.exec(sql)) {
            *errMsg = "Execute sql statement failed";
            break;
        }
        if (mysql.numRows() != 1) {
            *errMsg = "Exec sql: No proxy returned";
            break;
        }

        auto row = mysql.fetchRow();
        proxyData->host = util::to_string(row[0]);
        proxyData->port = util::to_int(row[1], 0);

        ret = 0;
    } while (false);

    return ret;
}


/* 获取数据库中的所有代理 */
/* 然后本地维护一个代理池 */
/* 每隔一段时间和数据库交换一次数据 */
int api_get_all_proxies(MysqlInstance& mysql, int maxErrorCount, std::vector<ic::ProxyData>* proxies, std::string* errMsg) {
    if (mysql.bad()) {
        *errMsg = "Mysql instance is bad";
        LError("Mysql instance is bad");
        return -1;
    }

    char sql[128];
    sprintf(sql, "SELECT `host`,`port` FROM proxy WHERE `error_count`<=%d;", maxErrorCount);
    if (!mysql.exec(sql) || mysql.numRows() < 1) {
        *errMsg = "Execute sql statement failed";
        return -1;
    }

    while (auto row = mysql.fetchRow()) {
        ic::ProxyData proxyData;
        proxyData.host = util::to_string(row[0]);
        proxyData.port = util::to_int(row[1], 0);
        proxies->emplace_back(proxyData);
    }

    return 0;
}


/* 提交代理IP失败的次数(累加) */
int api_error_proxy(MysqlInstance& mysql, const ic::ProxyData& proxyData, int count, std::string* errMsg) {
    if (count < 1) {
        return 0;
    }
    if (mysql.bad()) {
        *errMsg = "Mysql instance is bad";
        LError("Mysql instance is bad");
        return -1;
    }

    char sql[256];
    sprintf(sql, "UPDATE proxy SET error_count=error_count+%d WHERE `host`='%s' AND `port`=%u;",
        count, proxyData.host.c_str(), proxyData.port);
    if (!mysql.exec(sql)) {
        *errMsg = "Execute sql statement failed";
        return -1;
    }

    return 0;
}


/* 从数据库中清除代理IP */
int api_remove_proxy(MysqlInstance& mysql, const ic::ProxyData& proxyData, std::string* errMsg) {
    if (mysql.bad()) {
        *errMsg = "Mysql instance is bad";
        LError("Mysql instance is bad");
        return -1;
    }

    char sql[128];
    sprintf(sql, "DELETE FROM proxy WHERE `host`='%s' AND `port`=%u;",
        proxyData.host.c_str(), proxyData.port);
    if (!mysql.exec(sql)) {
        *errMsg = "Execute sql statement failed";
        return -1;
    }

    return 0;
}


int api_get_num_proxies(MysqlInstance& mysql, int* num, std::string* errMsg) {
    if (mysql.bad()) {
        *errMsg = "Mysql instance is bad";
        LError("Mysql instance is bad");
        return -1;
    }

    const char* sql = "SELECT count(1) FROM proxy;";
    if (!mysql.exec(sql) || mysql.numRows() != 1) {
        *errMsg = "Execute sql statement failed";
        return -1;
    }

    auto row = mysql.fetchRow();
    *num = util::to_int(row[0], 0);

    return 0;
}

