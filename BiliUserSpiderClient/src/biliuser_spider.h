#pragma once
#include "manager/proxy_manager.h"
#include "manager/user_agent_manager.h"
#include "manager/task_manager.h"
#include "database/mysql_instance.h"
#include "config/config.h"
#include <mutex>


class BiliUserSpider {
public:
    ~BiliUserSpider();

    int start();

public:
    friend void spiderThread(BiliUserSpider* spider, int tid);

private:
    bool readConfig();
    bool init();
    void startAllThreads();

private:
    bool connectToServer();
    bool clientJoin();
    bool clientQuit();

public:
    ProxyManager* proxy_mgr_ = nullptr;
    UserAgentManager* ua_mgr_ = nullptr;
    TaskManager* task_mgr_ = nullptr;

    MysqlDbPool* mysql_db_pool_ = nullptr;

    std::mutex mutex_;
    int living_threads_num_ = 0;
};

