#include "task.h"
#include "base.h"
#include "../global.h"
#include "../manager/task_manager.h"
#include "../manager/client_manager.h"
#include "../util/singleton.h"
#include "../config/config.h"

const int MIN_MIDS_COUNT =  200;
const int MAX_MIDS_COUNT = 1000;


/*
 *   功能：客户端向服务器申请任务
 *   URI: /task/get
**/
void task_get(Request& req, Response& res) {
    Json::Value root, data;
    CHECK_STATUS();
    CHECK_PARAM_INT(client_id);
    CHECK_PARAM_INT(commit_all);  /* 累计爬取的用户数量 */
    GET_PARAM_INT(count, MIN_MIDS_COUNT);   /* 申请多少个用户ID的任务 */
    if (count > MAX_MIDS_COUNT) {
        count = MAX_MIDS_COUNT;
    }

    /* 申请任务 */
    auto taskMgr = Singleton<TaskManager>::GetInstance();
    int midStart, midEnd;
    int taskId;
    int ret = taskMgr->getTask(client_id, count, &taskId, &midStart, &midEnd);
    if (ret != NO_ERROR) {
        RETURN_CODE(ret);
    }

    /* 记录心跳检测 */
    int shouldQuit;
    auto clientMgr = Singleton<ClientManager>::GetInstance();
    if (!clientMgr->heartbeat(client_id, commit_all, &shouldQuit)) {
        RETURN_CLINET_DISCONNECT();
    }

    /* data -> task */
    Json::Value taskNode;
    taskNode["start"] = midStart;
    taskNode["end"] = midEnd;
    taskNode["id"] = taskId;
    data["task"] = taskNode;

    /* 顺便更新客户端配置 */
    data["client_config"] = g_clientConfig.toJson();

    /* 是否应该退出
     *   0: 否
     *   1: 平滑退出
     *   2: 强制退出
    **/
    data["should_quit"] = shouldQuit;

    RETURN_OK();
}


/*
 *   功能：客户端向服务器提交任务
 *   URI: /task/commit
**/
void task_commit(Request& req, Response& res) {
    Json::Value root, data;
    CHECK_PARAM_INT(client_id);
    CHECK_PARAM_INT(task_id);
    CHECK_PARAM_INT(commit_all);

    /* 提交任务 */
    auto taskMgr = Singleton<TaskManager>::GetInstance();
    taskMgr->commit(client_id, task_id);

    /* 记录心跳检测 */
    int  shouldQuit;
    auto clientMgr = Singleton<ClientManager>::GetInstance();
    if (!clientMgr->heartbeat(client_id, commit_all, &shouldQuit)) {
        RETURN_CLINET_DISCONNECT();
    }

    /* 这里再检查服务器状态！！！ */
    /* 不能白白浪费了爬取的内容啊 */
    CHECK_STATUS();

    /* 顺便更新客户端配置 */
    data["client_config"] = g_clientConfig.toJson();

    /* 是否应该退出
     *   0: 否
     *   1: 平滑退出
     *   2: 强制退出
    **/
    data["should_quit"] = shouldQuit;

    RETURN_OK();
}

