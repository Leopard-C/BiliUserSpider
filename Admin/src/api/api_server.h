/*******************************************************
** description:  发送请求，对应的URI为
**                 /server/_xxx
**
** author: Leopard-C
**
** update: 2020/12/02
*******************************************************/
#pragma once
#include "../config/config.h"
#include "../client/spider_client_info.h"
#include <vector>


int api_connect_to_server(const std::string& server, bool* status, std::string* errMsg);

int api_server_status(const std::string& server, bool* status, std::string* errMsg);

int api_start_server(const std::string& server, std::string* errMsg);

int api_stop_server(const std::string& server, std::string* errMsg);

int api_shutdown_server(const std::string& server, std::string* errMsg);

int api_get_clients_info(const std::string& server, std::vector<SpiderClientInfo>* clientsInfo, bool* status, std::string* errMsg);

int api_quit_client(const std::string& server, int clientId, bool isForce, std::string* errMsg);

int api_get_task_progress(const std::string& server, int* midStart, int* midEnd, int* midCurrent, bool* status, std::string* errMsg);

int api_set_task_range(const std::string& server, int midStart, int midEnd, int midCurrent, std::string* errMsg);

