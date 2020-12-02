#include "api_client.h"
#include "api_base.h"
#include "../global.h"
#include "../json/json.h"
#include "../config/config.h"


/*
 *  功能：与爬虫服务器通信的API(s)
 *  URI: /client/xxx
**/


/* 测试与服务器的连通性 */
int api_connect_to_server(const std::string& server, ClientConfig* clientConfig,
    DatabaseConfig* dbConfig, std::string* errMsg)
{
	int ret = GENERAL_ERROR;
	do {
        Json::Value root;
        ret = http_get(server + "/client/connect", &root, errMsg);
        if (ret != NO_ERROR) {
            break;
        }

		if (checkNull(root, "data")) {
            ret = INTERNAL_SERVER_ERROR;
            *errMsg = "Missing key:data";
			break;;
		}

		Json::Value& data = root["data"];
		if (checkNull(data, "client_config", "db_config")) {
            ret = INTERNAL_SERVER_ERROR;
            *errMsg = "Missing key:client/client_config/db_config";
			break;
		}

        ClientConfig clientConfig;
        if (!clientConfig.parseFromJson(data["client_config"])) {
            ret = INTERNAL_SERVER_ERROR;
            *errMsg = "Parse data[client_config] failed";
            break;
        }
        g_clientConfig = clientConfig;

        DatabaseConfig dbConfig;
        if (!dbConfig.parseFromJson(data["db_config"])) {
            ret = INTERNAL_SERVER_ERROR;
            *errMsg = "Parse data[db_config] failed";
            break;
        }
        g_dbConfig = dbConfig;

        ret = NO_ERROR;
	} while (false);

	return ret;
}


/* 客户端加入 */
int api_client_join(const std::string& server, const std::string& os, int threads, int* clientId, std::string* errMsg)
{
	int ret = GENERAL_ERROR;
	do {
        Json::Value root;
        ic::Url url(server + "/client/join");
        url.setParam("os", os);
        url.setParam("threads", threads);
        ret = http_get(url, &root, errMsg);
        if (ret != NO_ERROR) {
            break;
        }

		if (checkNull(root, "data")) {
            ret = INTERNAL_SERVER_ERROR;
            *errMsg = "Missing key:data";
			break;;
		}

		Json::Value& data = root["data"];
		if (checkNull(data, "id")) {
            ret = INTERNAL_SERVER_ERROR;
            *errMsg = "Missing key:data[id]";
			break;
		}

		*clientId = getIntVal(data, "id");
        ret = NO_ERROR;
	} while (false);

	return ret;
}


/* 客户端退出 */
int api_client_quit(const std::string& server, int clientId, std::string* errMsg) {
    Json::Value root;
	ic::Url url(server + "/client/quit");
    url.setParam("id", clientId);
    return http_get(url, &root, errMsg);
}

