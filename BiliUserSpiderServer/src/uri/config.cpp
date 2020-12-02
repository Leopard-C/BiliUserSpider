#include "config.h"
#include "base.h"
#include "../global.h"
#include "../log/logger.h"
#include "../config/config.h"
#include <fstream>

extern ClientConfig g_clientConfig;
extern DatabaseConfig g_dbConfig;
extern std::string g_cfgDir;


/*
 * 1.1
 *   功能：设置客户端配置项
 *   方法：GET
 *   URI: /config/client/set
**/
void config_client_set(Request& req, Response& res) {
    Json::Value root, data;
    CHECK_PASSWORD();

    Json::Value postData;
    Json::Reader reader;
    if (!reader.parse(*(req.get_body()->get_raw_string()), postData, false)) {
        RETURN_INVALID_PARAM_MSG("Invalid param");
    }

    ClientConfig clientConfig;
    if (clientConfig.parseFromJson(postData) && clientConfig.isValid()) {
        g_clientConfig = clientConfig;
        g_write_config();
        RETURN_OK();
    }
    RETURN_INVALID_PARAM_MSG("Invalid param");
}

/*
 * 1.2
 *   功能：获取客户端配置项
 *   方法：GET
 *   URI: /config/client/get
**/
void config_client_get(Request& req, Response& res) {
    Json::Value root, data;
    CHECK_TOKEN_OR_PASSWORD();
    data = g_clientConfig.toJson();
    RETURN_OK();
}


/*
 * 2.1
 *   功能：设置数据库配置项
 *   方法：GET
 *   URI: /config/database/set
**/
void config_database_set(Request& req, Response& res) {
    Json::Value root, data;
    CHECK_PASSWORD();
    CHECK_PARAM_STR(host);
    CHECK_PARAM_INT(port);
    CHECK_PARAM_STR(user);
    CHECK_PARAM_STR(password);
    CHECK_PARAM_STR(name);
    DatabaseConfig dbConfig;
    dbConfig.host = host;
    dbConfig.port = port;
    dbConfig.user = user;
    dbConfig.password = password;
    dbConfig.name = name;
    if (dbConfig.isValid()) {
        g_dbConfig = dbConfig;
        g_write_config();
        RETURN_OK();
    }
    RETURN_INVALID_PARAM_MSG("Invalid param");
}

/*
 * 2.2
 *   功能：获取数据库配置项
 *   方法：GET
 *   URI: /config/database/get
**/
void config_database_get(Request& req, Response& res) {
    Json::Value root, data;
    CHECK_TOKEN_OR_PASSWORD();
    data = g_dbConfig.toJson();
    RETURN_OK();
}


/*
 * 3.1
 *   功能：设置代理IP配置项
 *   方法：GET
 *   URI: /config/proxy/set
**/
void config_proxy_set(Request& req, Response& res) {
    Json::Value root, data;
    CHECK_PASSWORD();
    CHECK_PARAM_STR(order_id);
    GET_PARAM_STR(user_id);  // 可选参数
    CHECK_PARAM_INT(interval_query);
    ProxyConfig proxyConfig;
    proxyConfig.order_id = order_id;
    proxyConfig.user_id = user_id;
    proxyConfig.interval_query = interval_query;
    if (proxyConfig.isValid()) {
        g_proxyConfig = proxyConfig;
        g_write_config();
        RETURN_OK();
    }
    RETURN_INVALID_PARAM_MSG("Invalid param");
}

/*
 * 3.2
 *   功能：获取代理IP配置项
 *   方法：GET
 *   URI: /config/proxy/get
**/
void config_proxy_get(Request& req, Response& res) {
    Json::Value root, data;
    CHECK_PASSWORD();
    data = g_proxyConfig.toJson();
    RETURN_OK();
}


/*
 * 4.1
 *   功能：设置服务器配置项
 *   方法：GET
 *   URI: /config/server/set
**/
void config_server_set(Request& req, Response& res) {
    Json::Value root, data;
    CHECK_PASSWORD();
    //CHECK_PARAM_STR_ARRAY(ips);
    //CHECK_PARAM_INT(port);
    //CHECK_PARAM_INT(thread_pool_size);
    CHECK_PARAM_INT(client_live_heartbeat);
    CHECK_PARAM_INT(interval_detect_client_live);
    CHECK_PARAM_INT(main_task_timeout);
    ServerConfig svrConfig = g_svrConfig;
    svrConfig.client_live_heartbeat = client_live_heartbeat;
    svrConfig.interval_detect_client_live = interval_detect_client_live;
    svrConfig.main_task_timeout = main_task_timeout;
    if (svrConfig.isValid()) {
        g_svrConfig = svrConfig;
        g_write_config();
        RETURN_OK();
    }
    RETURN_INVALID_PARAM_MSG("Invalid param");
}

/*
 * 4.2
 *   功能：获取服务器配置项
 *   方法：GET
 *   URI: /config/server/get
**/
void config_server_get(Request& req, Response& res) {
    Json::Value root, data;
    CHECK_PASSWORD();
    data = g_svrConfig.toJson();
    RETURN_OK();
}


/*
 * 5.1
 *  功能：高级设置（一次修改所有配置）
 *  方法：POST
 *  类型：application/json
 *  URI: config/advance/set
 */
void config_advance_set(Request& req, Response& res) {
    Json::Value root, data;
    Json::Reader reader;
    CHECK_PASSWORD();
    if (!reader.parse(*(req.get_body()->get_raw_string()), root, false)) {
        LError("Parse post body failed");
        RETURN_ERROR("Invalid post body");
    }

    ClientConfig clientConfig;
    DatabaseConfig dbConfig;
    ProxyConfig proxyConfig;
    ServerConfig svrConfig;
    if (!g_parse_from_json(root, &clientConfig, &dbConfig, &proxyConfig, &svrConfig, true)) {
        LError("Parse post body to config object failed");
        RETURN_ERROR("Invalid post body");
    }

    g_clientConfig = clientConfig;
    g_dbConfig = dbConfig;
    g_proxyConfig = proxyConfig;
    g_svrConfig = svrConfig;
    g_write_config();
    RETURN_OK();
}

/*
 * 5.2
 *  功能：高级设置（一次获取所有配置）
 *  方法：GET
 *  URI: config/advanced/get
 */
void config_advance_get(Request& req, Response& res) {
    Json::Value root, data;
    CHECK_PASSWORD();
    data["client"] = g_clientConfig.toJson();
    data["database"] = g_dbConfig.toJson();
    data["proxy"] = g_proxyConfig.toJson();
    data["server"] = g_svrConfig.toJson();
    RETURN_OK();
}

