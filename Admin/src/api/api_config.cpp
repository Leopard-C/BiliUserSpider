#include "api_base.h"
#include "api_config.h"
#include "../global.h"
#include "../iclient/ic/ic.h"
#include "../json/json.h"

/*
 *  功能：与爬虫服务器通信的API(s)
 *  URI: /config/xxx/get
**/


/* 获取client config */
int api_get_client_config(const std::string& server, ClientConfig* clientConfig, std::string* errMsg) {
    int ret = GENERAL_ERROR;
    do {
        Json::Value root;
        ret = http_get(server + "/config/client/get", &root, errMsg);
        if (ret != NO_ERROR) {
            break;
        }

        if (checkNull(root, "data")) {
            ret = INTERNAL_SERVER_ERROR;
            *errMsg = "Missing key:data";
            break;
        }

        if (!clientConfig->parseFromJson(root["data"])) {
            ret = INTERNAL_SERVER_ERROR;
            *errMsg = "parse root[data] failed";
            break;
        }

        ret = NO_ERROR;
    } while (false);

    return ret;
}


/* 设置客户端配置项 */
int api_set_client_config(const std::string& server, const ClientConfig& clientConfig, std::string* errMsg) {
    Json::Value root;
    return http_post(server + "/config/client/set", clientConfig.toJson(), &root, errMsg);
}


/* 获取database config */
int api_get_database_config(const std::string& server, DatabaseConfig* dbConfig, std::string* errMsg) {
    int ret = GENERAL_ERROR;
    do {
        Json::Value root;
        ret = http_get(server + "/config/database/get", &root, errMsg);
        if (ret != NO_ERROR) {
            break;
        }

        if (checkNull(root, "data")) {
            ret = INTERNAL_SERVER_ERROR;
            *errMsg = "Missing key:data";
            break;
        }

        if (dbConfig->parseFromJson(root["data"])) {
            ret = INTERNAL_SERVER_ERROR;
            *errMsg = "parse root[data] failed";
            break;
        }

        ret = NO_ERROR;
    } while (false);

    return ret;
}

/* 设置database config */
int api_set_database_config(const std::string& server, const DatabaseConfig& dbConfig, std::string* errMsg) {
    Json::Value root;
    return http_post(server + "/config/database/set", dbConfig.toJson(), &root, errMsg);
}

/* 获取代理IP配置 */
int api_get_proxy_config(const std::string& server, ProxyConfig* proxyConfig, std::string* errMsg) {
    int ret = GENERAL_ERROR;
    do {
        Json::Value root;
        ret = http_get(server + "/config/proxy/get", &root, errMsg);
        if (ret != NO_ERROR) {
            break;
        }

        if (checkNull(root, "data")) {
            ret = INTERNAL_SERVER_ERROR;
            *errMsg = "Missing key:data";
            break;
        }

        if (proxyConfig->parseFromJson(root["data"])) {
            ret = INTERNAL_SERVER_ERROR;
            *errMsg = "parse root[data] failed";
            break;
        }

        ret = NO_ERROR;
    } while (false);

    return ret;
}

/* 设置代理IP配置项 */
int api_set_proxy_config(const std::string& server, const ProxyConfig& proxyConfig, std::string* errMsg) {
    Json::Value root;
    return http_post(server + "/config/proxy/set", proxyConfig.toJson(), &root, errMsg);
}


/* 获取服务器配置项 */
int api_get_server_config(const std::string& server, ServerConfig* svrConfig, std::string* errMsg) {
    int ret = GENERAL_ERROR;
    do {
        Json::Value root;
        ret = http_get(server + "/config/server/get", &root, errMsg);
        if (ret != NO_ERROR) {
            break;
        }

        if (checkNull(root, "data")) {
            ret = INTERNAL_SERVER_ERROR;
            *errMsg = "Missing key:data";
            break;
        }

        if (svrConfig->parseFromJson(root["data"])) {
            ret = INTERNAL_SERVER_ERROR;
            *errMsg = "parse root[data] failed";
            break;
        }

        ret = NO_ERROR;
    } while (false);

    return ret;
}

/* 设置服务器配置项 */
int api_set_server_config(const std::string& server, const ServerConfig& svrConfig, std::string* errMsg) {
    Json::Value root;
    return http_post(server + "/config/server/set", svrConfig.toJson(), &root, errMsg);
}

int api_get_advance_config(const std::string& server, ClientConfig* clientConfig, DatabaseConfig* dbConfig,
    ProxyConfig* proxyConfig, ServerConfig* svrConfig, std::string* errMsg)
{
    int ret = GENERAL_ERROR;
    do {
        Json::Value root;
        ret = http_get(server + "/config/advance/get", &root, errMsg);
        if (ret != NO_ERROR) {
            break;
        }

        if (checkNull(root, "data")) {
            ret = INTERNAL_SERVER_ERROR;
            *errMsg = "Missing key:data";
            break;
        }

        if (!g_parse_from_json(root["data"], clientConfig, dbConfig, proxyConfig, svrConfig, false)) {
            ret = INTERNAL_SERVER_ERROR;
            *errMsg = "parse root[data] failed";
            break;
        }

        ret = NO_ERROR;
    } while (false);

    return ret;
}

int api_set_advance_config(const std::string& server, const ClientConfig& clientConfig, const DatabaseConfig& dbConfig,
    const ProxyConfig& proxyConfig, const ServerConfig& svrConfig, std::string* errMsg)
{
    Json::Value data;
    data["client"] = clientConfig.toJson();
    data["proxy"] = proxyConfig.toJson();
    data["database"] = dbConfig.toJson();
    data["server"] = svrConfig.toJson();
    Json::Value root;
    return http_post(server + "/config/advance/set", data, &root, errMsg);
}

