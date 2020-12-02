#include "api_base.h"
#include "api_server.h"
#include "../global.h"
#include <ctime>


/* 测试与服务器的连通性 */
int api_connect_to_server(const std::string& server, bool* status, std::string* errMsg) {
    Json::Value root;
	int ret = GENERAL_ERROR;
    do {
        ret = http_get(server + "/server/_connect", &root, errMsg);
        if (ret != NO_ERROR) {
            break;
        }

        if (checkNull(root, "status")) {
            ret = INTERNAL_SERVER_ERROR;
            *errMsg = "Missing key:status";
            break;
        }
        *status = getBoolVal(root, "status");

    } while (false);

    return ret;
}


/* 服务器状态 */
int api_server_status(const std::string& server, bool* status, std::string* errMsg) {
    Json::Value root;
	int ret = GENERAL_ERROR;
    do {
        ret = http_get(server + "/server/_status", &root, errMsg);
        if (ret != NO_ERROR) {
            break;
        }

        if (checkNull(root, "status")) {
            ret = INTERNAL_SERVER_ERROR;
            *errMsg = "Missing key:status";
            break;
        }
        *status = getBoolVal(root, "status");

    } while (false);

    return ret;
}


/* 开启服务器 */
int api_start_server(const std::string& server, std::string* errMsg) {
    Json::Value root;
    return http_get(server + "/server/_start", &root, errMsg);
}


/* 关闭服务器 */
int api_stop_server(const std::string& server, std::string* errMsg) {
    Json::Value root;
    return http_get(server + "/server/_stop", &root, errMsg);
}

/* 关闭服务器（real stop） */
int api_shutdown_server(const std::string& server, std::string* errMsg) {
    Json::Value root;
    return http_get(server + "/server/_shutdown", &root, errMsg);
}


/* 获取所有连接的客户端信息 */
int api_get_clients_info(const std::string& server, std::vector<SpiderClientInfo>* clientsInfo,
    bool* status, std::string* errMsg)
{
	int ret = GENERAL_ERROR;
	do {
        Json::Value root;
        ret = http_get(server + "/server/_get_clients_info", &root, errMsg);
        if (ret != NO_ERROR) {
            break;
        }

        if (checkNull(root, "status")) {
            ret = INTERNAL_SERVER_ERROR;
            *errMsg = "Missing key:status";
            break;
        }
        *status = getBoolVal(root, "status");

        if (!checkNull(root, "data") && root["data"].size() > 0) {
            Json::Value& data = root["data"];
            bool error = false;
            int size = data.size();
            for (int i = 0; i < size; ++i) {
                SpiderClientInfo info;
                if (!info.parseFromJson(data[i])) {
                    ret = INTERNAL_SERVER_ERROR;
                    *errMsg = "Parse client info from json failed";
                    break;
                }
                clientsInfo->emplace_back(info);
            }
            if (error) {
                break;
            }
        }

		ret = NO_ERROR;
	} while (false);

	return ret;
}

/* 通知客户端退出 */
int api_quit_client(const std::string& server, int clientId, bool isForce, std::string* errMsg) {
    Json::Value root;
    ic::Url url(server + "/server/_quit_client");
    url.setParam("id", clientId);
    url.setParam("force", static_cast<int>(isForce));
    return http_get(url, &root, errMsg);
}

/* 任务完成进度 */
int api_get_task_progress(const std::string& server, int* midStart, int* midEnd, int* midCurrent, bool* status, std::string* errMsg) {
	int ret = GENERAL_ERROR;
	do {
        Json::Value root;
        ret = http_get(server + "/server/_get_task_progress", &root, errMsg);
        if (ret != NO_ERROR) {
            break;
        }

        if (checkNull(root, "status", "data")) {
            ret = INTERNAL_SERVER_ERROR;
            *errMsg = "Missing key:status/data";
            break;
        }
        *status = getBoolVal(root, "status");

        Json::Value& data = root["data"];
        if (checkNull(data, "mid_start", "mid_end", "mid_current")) {
            ret = INTERNAL_SERVER_ERROR;
            *errMsg = "Missing key:data[mid_start/mid_end/mid_current]";
            break;
        }

        *midStart = getIntVal(data, "mid_start");
        *midEnd = getIntVal(data, "mid_end");
        *midCurrent = getIntVal(data, "mid_current");

		ret = NO_ERROR;
	} while (false);

	return ret;
}

/* 设置任务范围 */
int api_set_task_range(const std::string& server, int midStart, int midEnd, int midCurrent, std::string* errMsg) {
    Json::Value root;
    ic::Url url(server + "/server/_set_task_range");
    url.setParam("start", midStart);
    url.setParam("end", midEnd);
    url.setParam("current", midCurrent);
    return http_get(url, &root, errMsg);
}

