#include "client.h"
#include "base.h"
#include "../util/singleton.h"
#include "../manager/client_manager.h"
#include "../config/config.h"
#include "../json/json.h"
#include "../task/task.h"
#include "../global.h"
#include <map>


/*
 *   功能：客户端测试连接服务器
 *   方法：GET
 *   URI: /client/connect
**/
void client_connect(Request& req, Response& res) {
    Json::Value root, data;
    CHECK_TOKEN();
    CHECK_STATUS();
    data["client_config"] = g_clientConfig.toJson();
    data["db_config"] = g_dbConfig.toJson();
    RETURN_OK();
}


/*
 *   功能：客户端加入爬虫队列, 返回客户端ID
 *   方法：GET
 *   URI: /client/join
**/
void client_join(Request& req, Response& res) {
    Json::Value root, data;
    CHECK_TOKEN();
    CHECK_STATUS();
    CHECK_PARAM_STR(os);
    CHECK_PARAM_INT(threads);
    std::string ip = req.get_real_ip();
    if (ip.empty()) {
        ip = *(req.get_client_ip());
    }
    auto clientMgr = Singleton<ClientManager>::GetInstance();
    data["id"] = clientMgr->join(ip, os, threads);
    RETURN_OK();
}


/*
 *   功能：客户端退出
 *   方法：GET
 *   URI: /client/quit
**/
void client_quit(Request& req, Response& res) {
    Json::Value root, data;
    CHECK_TOKEN();
    CHECK_PARAM_INT(id);
    auto clientMgr = Singleton<ClientManager>::GetInstance();
    clientMgr->quit(id);
    RETURN_OK();
}

