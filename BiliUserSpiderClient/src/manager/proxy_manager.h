/*************************************************************************
** class name:  ProxyManager
**
** description: 本地维护一个代理IP池，定期和数据库中的代理IP池同步,
**              爬虫线程只需要从这里获取一个随机代理IP就行
**              而不需要每次都从数据库随机获取
**
** author: Leopard-C
**
** update: 2020/12/02
**************************************************************************/
#pragma once
#include <string>
#include <vector>
#include <mutex>
#include <map>
#include "../config/config.h"
#include "../database/mysql_pool.h"
#include "../iclient/ic/ic.h"

class ProxyManager {
public:
    ProxyManager();
    ~ProxyManager();
    bool getRandomProxy(ic::ProxyData& proxyData); 

public:
    void start(MysqlDbPool* mysqlDbPool);
    void stop();

    void errorProxy(const ic::ProxyData& proxyData);
    void removeProxy(const ic::ProxyData& proxyData);
    size_t getNumProxies() const { return proxies_.size(); }

    bool isRunning() const { return !bg_thread_quit_; }

public:
    /* 仅供backgroundThread访问 */
    /* 与数据库中的代理IP池同步数据 */
    bool reportAllErrorProxiesToDb();
    bool getAllProxiesFromDb(std::vector<ic::ProxyData>& proxiesData);
    bool getNumProxiesFromDb(int* num);

    static void backgroundThread(ProxyManager* proxySvr);

private:
    struct hash_function {
        bool operator()(const ic::ProxyData& p1, const ic::ProxyData& p2) const {
            if (p1.host < p2.host) {
                return true;
            }
            else if (p1.host > p2.host) {
                return false;
            }
            else {
                if (p1.port < p2.port) {
                    return true;
                }
                else {
                    return false;
                }
            }
        }
    };

public:
    /* <代理IP信息，失败次数> */
    std::map<ic::ProxyData, int, hash_function> proxies_;

    std::mutex mutex_;
    bool bg_thread_quit_ = true;
    MysqlDbPool* mysql_db_pool_ = nullptr;
};
