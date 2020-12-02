#include "biliuser_spider.h"
#include "json/json.h"
#include "api/api_client.h"
#include "api/api_config.h"
#include "log/logger.h"
#include "status/status_code.h"
#include "global.h"
#include <thread>
#include <ctime>
#include <fstream>

void spiderThread(BiliUserSpider* spider, int tid);


BiliUserSpider::~BiliUserSpider() {
    /* 等待，直到所有后台线程退出 */
    g_stop(StopMode::Force);
    for (int i = 0; i < 20; ++i) {
        if (living_threads_num_ > 0) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    if (proxy_mgr_) {
        for (int i = 0; i < 20; ++i) {
            if (!proxy_mgr_->isRunning()) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        delete proxy_mgr_;
    }
    if (ua_mgr_) {
        delete ua_mgr_;
        ua_mgr_ = nullptr;
    }
    if (task_mgr_) {
        task_mgr_->flush();
        delete task_mgr_;
        task_mgr_ = nullptr;
    }
    if (mysql_db_pool_) {
        delete mysql_db_pool_;
        mysql_db_pool_ = nullptr;
    }
}


/* 启动爬虫 */
int BiliUserSpider::start() {
    /* 从控制台/或者文件读取简单的配置 */
    if (!readConfig()) {
        LError("Read configuration failed");
        return 1;
    }

    /* 尝试连接服务器 */
    if (!connectToServer()) {
        LError("Connect to server failed");
        return 1;
    }

    /* 输出信息 */
    LInfo("Bili API Type:         {}", g_clientConfig.bili_api_type == 0 ? "Web" : "App");
    LInfo("Main task step times:  {}", g_clientConfig.main_task_step_times);
    LInfo("Subtask step:          {}", g_clientConfig.sub_task_step);
    LInfo("Timeout:               {} ms", g_clientConfig.timeout);
    LInfo("Proxy max error times: {}", g_clientConfig.proxy_max_error_times);
    LInfo("Mid max error times:   {}", g_clientConfig.mid_max_error_times);
    LInfo("Interval sync proxy:   {}", g_clientConfig.interval_proxypool_sync);

    /* 初始化 */
    if (!init()) {
        LError("Init failed");
        return 1;
    }

    /* 加入爬虫队列 */
    if (!clientJoin()) {
        LError("Client join failed");
        return 1;
    }

    LInfo("Wait for starting...");
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    LInfo("Starting...");

    /* 启动所有线程 */
    startAllThreads();

    /* 等待，直到所有后台线程退出 */
    while (living_threads_num_ > 0) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    g_stop(StopMode::Force);

    /* 通知服务器，本客户端退出 */
    if (!clientQuit()) {
        LError("Notify server failed.");
    }

    LInfo("Remain Main Tasks: {}", task_mgr_->numMainTasks());
    LInfo("Remain Sub Tasks: {}", task_mgr_->numSubTasks());
    LInfo("Stopped OK!");
    return 0;
}


/* 读取简单的配置 */
bool BiliUserSpider::readConfig() {
    /* 输入服务器地址 */
    //std::cout << "Server IP: " << std::flush;
    //std::cin >> g_svrAddr;
    //g_serverAddr += ":10032";
    //std::cout << "Threads: " << std::flush;
    //std::cin >> g_numThreads;

    /* 从文件读取 */
    std::ifstream ifs(g_cfgDir + "/spider.json");
    if (!ifs) {
        LError("Open {}/spider.json failed", g_cfgDir);
        return false;
    }
    Json::Value root;
    Json::Reader reader;
    if (!reader.parse(ifs, root, false)) {
        LError("Parse {}/spider.json failed", g_cfgDir);
        return false;
    }
    if (checkNull(root, "server_addr", "threads", "token")) {
        LError("Missing key:server_addr/threads");
        return false;
    }
    g_svrAddr = getStrVal(root, "server_addr");
    g_numThreads = getIntVal(root, "threads");
    g_clientToken = getStrVal(root, "token");
    if (g_numThreads < 1 || g_numThreads > 90) {
        std::cout << "Threads should be [1-90] !";
        return false;
    }
    return true;
}


/* 连接服务器 */
bool BiliUserSpider::connectToServer() {
    std::string errMsg;
    for (int i = 0; i < 5; ++i) {
        int ret = api_connect_to_server(g_svrAddr, &g_clientConfig, &g_dbConfig, &errMsg);
        if (ret == NO_ERROR) {
            return true;
        }
        else {
            LError("{}", errMsg);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    return false;
}


/* 初始化 */
bool BiliUserSpider::init() {
    /* 随机UA管理器 */
    ua_mgr_ = new UserAgentManager();
    if (!ua_mgr_->readConfig(g_cfgDir + "/user-agent.json")) {
        LError("UserAgentManager read configuration failed");
        return false;
    }

    /* Mysql数据库连接池 */
    mysql_db_pool_ = new MysqlDbPool(g_dbConfig.host, g_dbConfig.port, g_dbConfig.user,
        g_dbConfig.password, "bili_user", 3);
    if (mysql_db_pool_->bad()) {
        LError("Create proxy db connector pool failed");
        return false;
    }

    /* 任务分配管理器 */
    task_mgr_ = new TaskManager();
    task_mgr_->start(mysql_db_pool_);

    /* 代理ip管理器 */
    proxy_mgr_ = new ProxyManager();
    proxy_mgr_->start(mysql_db_pool_);

    return true;
}


/* 启动所有线程 */
void BiliUserSpider::startAllThreads() {
    /* 爬虫线程 */
    if (proxy_mgr_ && ua_mgr_ && task_mgr_ && mysql_db_pool_) {
        int tid = 0;  /* 自定义的线程编号，并非系统级别的tid */
        for (int i = 0; i < g_numThreads && !g_shouldStop; ++i) {
            living_threads_num_++;
            std::thread t(spiderThread, this, tid++);
            t.detach();
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        LInfo("Thread {} started", tid);
    }
    else {
        LError("Not setupped!");
    }
}


/* 爬虫线程 */
void spiderThread(BiliUserSpider* spider, int tid) {
    auto taskMgr = spider->task_mgr_;
    auto proxyMgr = spider->proxy_mgr_;
    auto uaMgr = spider->ua_mgr_;

    std::vector<bili::UserInfo> infos;
    std::vector<int> noneMids;
    std::deque<int> errorMids;

    while (1) {
        infos.clear();
        errorMids.clear();
        noneMids.clear();

        /* 1. 申请任务 */
        std::vector<int> mids;
        int taskId = taskMgr->getSubTask(mids);
        
        /* 没有申请到任务 */
        if (taskId == -1) {
            /* 本地任务管理器申请的主任务都已提交，直接退出 */
            if (taskMgr->numMainTasks() == 0) {
                break;
            }
            /* 设置了“快速退出”选项 */
            else if (g_forceStop) {
                break;
            }
            /* 否则，不能退出，因为已经发布出去的任务（其他线程）未必能全部正常完成 */
            /* 等待一段时间，重新申请任务 */
            else {
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
                continue;
            }
        }

        /* 2. 执行任务 */
        for (auto mid : mids) {
            /* 打印进度(总得输出点啥) */
            if (mid % 50 == 0) {
                LInfo("[{0:2d}] {1}", tid, mid);
            }

            /* 开始获取该用户信息, 失败一定次数就视为失败 */
            bili::UserInfo info;
            int hasError = true;
            for (int i = 0; i < g_clientConfig.mid_max_error_times && !g_forceStop; ++i) {
                int ret;
                if (g_clientConfig.bili_api_type == 0) {
                    ret = bili::getUserInfo_Web(mid, g_clientConfig.timeout, proxyMgr, uaMgr, &info);
                }
                else {
                    ret = bili::getUserInfo_App(mid, g_clientConfig.timeout, proxyMgr, uaMgr, &info);
                }
                if (ret == 0) { // 0 => No error, user exist and get info successfully
                    infos.emplace_back(info);
                    hasError = false;
                    break;
                }
                else if (ret == 1) { // 0 => No error, user not exist.
                    noneMids.emplace_back(mid);
                    hasError = false;
                    break;
                }  // -1 => Error
                else {
                    hasError = true;
                    continue;
                }
            }
            if (!g_forceStop && hasError) {
                LError("Error {} times.", g_clientConfig.mid_max_error_times);
                errorMids.emplace_back(mid);
            }

            /* 这里不应该退出, 因为还能和服务器通讯，服务器仍在运行！ */
            //if (g_shouldStop) {
            //    break;
            //}

            /* 强制退出 */
            if (g_forceStop) {
                break;
            }
        }

        /* 强制退出,就不提交该任务了, 任务管理器会自动收回本次任务 */
        if (g_forceStop) {
            break;
        }

        /* 3. 提交任务 */
        taskMgr->commitSubTask(taskId, infos, noneMids, errorMids);
    }

    LInfo("[{:2d}] background thread quit.", tid);

    {
        std::lock_guard<std::mutex> lck(spider->mutex_);
        spider->living_threads_num_--;
    }
}


/* 加入爬虫 */
bool BiliUserSpider::clientJoin() {
    for (int i = 0; i < 5; ++i) {
        std::string errMsg;
        int ret = api_client_join(g_svrAddr, g_clientOS, g_numThreads, &g_clientId, &errMsg);
        if (ret == NO_ERROR) {
            return true;
        }
        else if (ret == SERVER_STOPPED) {
            LError("Server stopped");
            return false;
        }
        else {
            LError("{}", errMsg);
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
    }
    return false;
}

/* 退出爬虫 */
bool BiliUserSpider::clientQuit() {
    for (int i = 0; i < 5; ++i) {
        std::string errMsg;
        int ret = api_client_quit(g_svrAddr, g_clientId, &errMsg);
        if (ret == NO_ERROR) {
            return true;
        }
        else {
            LError("{}", errMsg);
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
    }
    return false;
}

