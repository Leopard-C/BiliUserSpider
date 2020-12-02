#include "server.h"
#include "base.h"
#include "../util/singleton.h"
#include "../server/http_server.h"
#include "../manager/client_manager.h"
#include "../manager/proxy_manager.h"
#include "../manager/task_manager.h"
#include "../mysql/mysql_pool.h"
#include "../config/config.h"
#include "../global.h"



/*
 *   用户：管理员
 *   功能：测试连接
 *   方法：GET
 *   URI: /server/_connect
**/
void server_connect(Request& req, Response& res) {
    Json::Value root, data;
    CHECK_PASSWORD();
    root["status"] = g_isRunning;
    RETURN_OK();
}


/*
 *   用户：管理员
 *   功能：获取服务器状态(程序级别的运行 + 逻辑级别的运行)
 *   方法：GET
 *   URI: /server/_status
**/
void server_status(Request& req, Response& res) {
    Json::Value root, data;
    CHECK_PASSWORD();
    root["status"] = g_isRunning;
    RETURN_OK();
}


/*
 *   用户：管理员
 *   功能：启动服务器(逻辑级别)
 *   方法：GET
 *   URI: /server/_start
**/
void server_start(Request& req, Response& res) {
    Json::Value root, data;
    CHECK_PASSWORD();
    if (g_proxyConfig.order_id.empty()) {
        RETURN_ERROR("Please set order_id of proxy first");
    }
    if (g_isRunning) {
        RETURN_OK();
    }

    g_start();

    auto proxyMgr = Singleton<ProxyManager>::GetInstance();
    proxyMgr->start();

    auto clientMgr = Singleton<ClientManager>::GetInstance();
    clientMgr->start();

    LInfo("Server started");
    RETURN_OK();
}


/*
 *   用户：管理员
 *   功能：停止服务器(逻辑级别)
 *   方法：GET
 *   URI: /server/_stop
**/
void server_stop(Request& req, Response& res) {
    Json::Value root, data;
    CHECK_PASSWORD();
    g_stop();
    LInfo("Server stopped");
    RETURN_OK();
}


/*
 *   用户：管理员
 *   功能：关闭服务器(程序级别)
 *   方法：GET
 *   URI: /server/_shutdown
**/
void server_shutdown(Request& req, Response& res) {
    Json::Value root, data;
    CHECK_PASSWORD();
    g_stop();
    auto svr = Singleton<HttpServer>::GetInstance();
    svr->stop();
    LInfo("Server shutdown");
    RETURN_OK();
}


/*
 *   用户：管理员
 *   功能：关闭服务器(程序级别)
 *   方法：GET
 *   URI: /server/_get_quit_client;
**/
void server_get_clients_info(Request& req, Response& res) {
    Json::Value root, data;
    CHECK_PASSWORD();
    auto clientMgr = Singleton<ClientManager>::GetInstance();
    clientMgr->getClientsInfo(data);
    root["status"] = g_isRunning;
    RETURN_OK();
}



/*
 *   用户：管理员
 *   功能：通知客户端退出
 *   方法：GET
 *   URI: /server/_quit_client;
**/
void server_quit_client(Request& req, Response& res) {
    Json::Value root, data;
    CHECK_PASSWORD();
    CHECK_PARAM_INT(id);
    CHECK_PARAM_INT(force);
    auto clientMgr = Singleton<ClientManager>::GetInstance();
    clientMgr->notifyClientToQuit(id, static_cast<bool>(force));
    RETURN_OK();
}


/*
 *   用户：管理员
 *   功能：获取任务进度
 *   方法：GET
 *   URI: /server/_get_task_progress;
**/
void server_get_task_progress(Request& req, Response& res) {
    Json::Value root, data;
    CHECK_PASSWORD();
    auto taskMgr = Singleton<TaskManager>::GetInstance();
    int midStart, midEnd, midCurrent;
    taskMgr->getProgress(&midStart, &midEnd, &midCurrent);
    data["mid_start"] = midStart;
    data["mid_end"] = midEnd;
    data["mid_current"] = midCurrent;
    root["status"] = g_isRunning;
    RETURN_OK();
}

/*
 *   用户：管理员
 *   功能：修改任务范围
 *   方法：GET
 *   URI: /server/_set_task_range
**/
void server_set_task_range(Request& req, Response& res) {
    Json::Value root, data;
    CHECK_PASSWORD();
    CHECK_PARAM_INT(start);
    CHECK_PARAM_INT(end);
    CHECK_PARAM_INT(current);
    auto taskMgr = Singleton<TaskManager>::GetInstance();
    taskMgr->setRange(start, end, current);
    RETURN_OK();
}


