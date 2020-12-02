#include "server/http_server.h"
#include "manager/task_manager.h"
#include "manager/client_manager.h"
#include "manager/proxy_manager.h"
#include "util/singleton.h"
#include "util/path.h"
#include "config/config.h"
#include "log/logger.h"
#include "json/json.h"
#include "global.h"

#include "uri/task.h"
#include "uri/config.h"
#include "uri/server.h"
#include "uri/client.h"

#include <signal.h>
#include <unistd.h>
#include <cstdlib>
#include <fstream>

/* 项目目录 */
std::string g_projDir = util::getProjDir();
/* 源代码目录 */
std::string g_srcDir = g_projDir + "/src";
/* 数据目录 */
std::string g_dataDir = g_projDir + "/data";
/* 配置文件目录 */
std::string g_cfgDir = g_projDir + "/config";
/* 日志目录 */
std::string g_logsDir = g_projDir + "/logs";

/* 各种配置 */
DatabaseConfig g_dbConfig;
ClientConfig   g_clientConfig;
ServerConfig   g_svrConfig;
ProxyConfig    g_proxyConfig;

/* 服务器状态 */
/* 可直接读取，但是应该通过g_start(), g_stop()函数修改 */
bool g_isRunning = false;

/* 启动整个服务器，可多次调用 */
void g_start();

/* 停止整个服务器，可多次调用 */
void g_stop();

/* Ctrl + C 处理函数 */
void catchCtrlC(int code);


/*
 *   主函数
**/
int main(int argc, char* argv[]) {
    srand(time(0));

    signal(SIGINT, catchCtrlC);

    /* 读取配置文件 */
    if (!g_read_config()) {
        LError("Read configuration failed");
        return 1;
    }
    LInfo("Read configuration ok");

    /* 任务管理器 */
    auto taskMgr = Singleton<TaskManager>::GetInstance();
    if (!taskMgr->readData()) {
        return 1;
    }

    /* 服务器对象(单例) */
    auto svr = Singleton<HttpServer>::GetInstance();
    ThreadPool tp;
    tp.set_pool_size(g_svrConfig.thread_pool_size);
    svr->set_thread_pool(&tp);

    // 注册 URI 回调函数
    // 0. 本服务器
    svr->add_mapping("/server/_connect", server_connect, GET_METHOD);
    svr->add_mapping("/server/_status", server_status, GET_METHOD);
    svr->add_mapping("/server/_start", server_start, GET_METHOD);
    svr->add_mapping("/server/_stop", server_stop, GET_METHOD);
    svr->add_mapping("/server/_shutdown", server_shutdown, GET_METHOD);
    svr->add_mapping("/server/_get_clients_info", server_get_clients_info, GET_METHOD);
    svr->add_mapping("/server/_get_task_progress", server_get_task_progress, GET_METHOD);
    svr->add_mapping("/server/_quit_client", server_quit_client, GET_METHOD);
    svr->add_mapping("/server/_set_task_range", server_set_task_range, GET_METHOD);
    // 1. 客户端
    svr->add_mapping("/client/connect", client_connect, GET_METHOD);
    svr->add_mapping("/client/join", client_join, GET_METHOD);
    svr->add_mapping("/client/quit", client_quit, GET_METHOD);
    // 2. 任务
    svr->add_mapping("/task/get", task_get, GET_METHOD);
    svr->add_mapping("/task/commit", task_commit, GET_METHOD);
    // 3. 配置项
    svr->add_mapping("/config/client/set", config_client_set, POST_METHOD);
    svr->add_mapping("/config/client/get", config_client_get, GET_METHOD);
    svr->add_mapping("/config/database/set", config_database_set, POST_METHOD);
    svr->add_mapping("/config/database/get", config_database_get, GET_METHOD);
    svr->add_mapping("/config/proxy/set", config_proxy_set, POST_METHOD);
    svr->add_mapping("/config/proxy/get", config_proxy_get, GET_METHOD);
    svr->add_mapping("/config/server/set", config_server_set, POST_METHOD);
    svr->add_mapping("/config/server/get", config_server_get, GET_METHOD);
    svr->add_mapping("/config/advance/set", config_advance_set, POST_METHOD);
    svr->add_mapping("/config/advance/get", config_advance_get, GET_METHOD);

    for (auto& ip : g_svrConfig.IPs) {
        svr->add_bind_ip(ip);
    }
    svr->set_port(g_svrConfig.port);
    svr->start_sync();

    LInfo("Main function quit");
    return 0;
}


/* Ctrl + C 处理函数 */
void catchCtrlC(int code) {
    g_stop();
    Singleton<ClientManager>::GetInstance()->stop();
    Singleton<ProxyManager>::GetInstance()->stop();
    Singleton<HttpServer>::GetInstance()->stop();
    Singleton<ClientManager>::DesInstance();
    Singleton<ProxyManager>::DesInstance();
    Singleton<HttpServer>::DesInstance();
}

static std::mutex s_server_mutex;

/* 启动整个服务器，可多次调用 */
void g_start() {
    std::lock_guard<std::mutex> lck(s_server_mutex);
    g_isRunning = true;
}

/* 停止整个服务器，可多次调用 */
void g_stop() {
    std::lock_guard<std::mutex> lck(s_server_mutex);
    g_isRunning = false;
}


