#include "api_config.h"
#include "api_base.h"
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

