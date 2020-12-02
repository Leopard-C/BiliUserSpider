#include "api_task.h"
#include "api_base.h"
#include "../log/logger.h"
#include "../global.h"
#include "../json/json.h"
#include "../iclient/ic/ic.h"
#include "../config/config.h"
#include "../task/task.h"

extern int g_clientId;


/* 向主服务器申请任务 */
int api_get_task(const std::string& server, int clientId, int count, int commitAll,
    MainTask* mainTask, ClientConfig* clientConfig, std::string* errMsg)
{
	int ret = GENERAL_ERROR;
	do {
        Json::Value root;
        ic::Url url(server + "/task/get");
        url.setParam("count", count);
        url.setParam("client_id", clientId);
        url.setParam("commit_all", commitAll);
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
        if (checkNull(data, "task", "client_config")) {
            ret = INTERNAL_SERVER_ERROR;
            *errMsg = "Missing key:data[task/client_config]";
            break;
        }

        /* data[task] */
        Json::Value& taskNode = data["task"];
        if (checkNull(taskNode, "start", "end", "id")) {
            ret = INTERNAL_SERVER_ERROR;
            *errMsg = "Missing key:data[task][start/end/id]";
            break;
        }
        mainTask->mid_start = getUIntVal(taskNode, "start");
        mainTask->mid_end = getUIntVal(taskNode, "end");
        mainTask->id = getIntVal(taskNode, "id");
        mainTask->completed = 0;

        /* data[client_config] */
        if (!clientConfig->parseFromJson(data["client_config"])) {
            ret = INTERNAL_SERVER_ERROR;
            *errMsg = "Parse data[client_config] failed";
            break;
        }

		ret = NO_ERROR;
	} while (false);

	return ret;
}


/* 向主服务器提交任务,任务完成 */
int api_commit_task(const std::string& server, int clientId, int taskId, int commitAll,
    ClientConfig* clientConfig, std::string* errMsg)
{
    int ret = GENERAL_ERROR;
    do {
        Json::Value root;
        ic::Url url(server + "/task/commit");
        url.setParam("client_id", clientId);
        url.setParam("task_id", taskId);
        url.setParam("commit_all", commitAll);
        ret = http_get(url, &root, errMsg);
        if (ret != NO_ERROR) {
            break;
        }

		if (checkNull(root, "data")) {
            ret = INTERNAL_SERVER_ERROR;
            *errMsg = "Missing key:data";
			break;
		}

        Json::Value& data = root["data"];
        if (checkNull(data, "client_config", "should_quit")) {
            ret = INTERNAL_SERVER_ERROR;
            *errMsg = "Missing key:data[client_config/should_quit/...]";
            break;
        }

        /* data[client_config] */
        if (!clientConfig->parseFromJson(data["client_config"])) {
            ret = INTERNAL_SERVER_ERROR;
            *errMsg = "Parse data[client_config] failed";
            break;
        }

        int shouldQuit = getIntVal(data, "should_quit");
        if (shouldQuit == 2) {
            LWarn("Force stopped by admin");
            g_stop(StopMode::Force);
        }
        else if (shouldQuit == 1) {
            LWarn("Stopped by admin");
            g_stop(StopMode::Smooth);
        }

        ret = NO_ERROR;
    } while (false);

    return ret;
}

