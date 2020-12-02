/*******************************************************
** description:  发送请求，对应的URI为
**                 /client/xxx
**
** author: Leopard-C
**
** update: 2020/12/02
*******************************************************/
#pragma once
#include <string>
#include <time.h>
#include "../config/config.h"

int api_connect_to_server(const std::string& server, ClientConfig* clientConfig,
    DatabaseConfig* dbConfig, std::string* errMsg);

int api_client_join(const std::string& server, const std::string& os, int threads, int* clientId, std::string* errMsg);

int api_client_quit(const std::string& server, int clientId, std::string* errMsg);


