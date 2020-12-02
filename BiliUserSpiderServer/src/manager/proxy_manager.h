/*************************************************************************
** class name:  ProxyManager
**
** description: 1. 从代理IP服务商那里获取代理IP,添加到数据库
**              2. 清除数据库中失败较次数多的代理IP
**
** author: Leopard-C
**
** update: 2020/12/02
**************************************************************************/
#pragma once
#include <string>
#include <vector>
#include <mutex>
#include <atomic>
#include <map>
#include "../iclient/ic/ic_request.h"

class MysqlDbPool;


class ProxyManager {
public:
    ProxyManager();
    ~ProxyManager();

    void start();

    void stop();

public:
    /* 各种代理IP服务商，不过每次只能启用一种 */
    friend void backgroundThreadQingTing(ProxyManager* mgr);
    friend void backgroundThreadAiErYun(ProxyManager* mgr);

private:
    /* 添加新的代理ip到数据库 */
    bool addProxies(const std::vector<ic::ProxyData>& proxies);

    /* 清除数据库中失败较次数多的代理 */
    bool removeProxies();

public:
    MysqlDbPool* mysql_db_pool_ = nullptr;

    std::mutex mutex_;

    bool bg_thread_AiErYun_quit_ = true;
    bool bg_thread_QingTing_quit_ = true;
};

