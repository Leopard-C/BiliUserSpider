/*******************************************************
** description:  发送请求，对应的URI为
**                 /config/xxx/set
**                 /config/xxx/get
**
** author: Leopard-C
**
** update: 2020/12/02
*******************************************************/
#pragma once
#include <string>
#include "../config/config.h"

int api_get_client_config(const std::string& server, ClientConfig* clientConfig, std::string* errMsg);

int api_get_database_config(const std::string& server, DatabaseConfig* dbConfig, std::string* errMsg);

