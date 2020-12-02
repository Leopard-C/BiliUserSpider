/*******************************************************
** description:  发送请求，对应的URI为
**                 /task/xxx
**
** author: Leopard-C
**
** update: 2020/12/02
*******************************************************/
#pragma once
#include <string>
#include <vector>
#include "../config/config.h"
#include "../task/task.h"


int api_get_task(const std::string& server, int clientId, int count, int commitAll,
    MainTask* mainTask, ClientConfig* appConfig, std::string* errMsg);

int api_commit_task(const std::string& server, int clientId, int taskId, int commitAll,
    ClientConfig* clientConfig, std::string* errMsg);


