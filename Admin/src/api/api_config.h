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
int api_set_client_config(const std::string& server, const ClientConfig& clientConfig, std::string* errMsg);

int api_get_database_config(const std::string& server, DatabaseConfig* dbConfig, std::string* errMsg);
int api_set_database_config(const std::string& server, const DatabaseConfig& dbConfig, std::string* errMsg);

int api_get_proxy_config(const std::string& server, ProxyConfig* proxyConfig, std::string* errMsg);
int api_set_proxy_config(const std::string& server, const ProxyConfig& proxyConfig, std::string* errMsg);

int api_get_server_config(const std::string& server, ServerConfig* svrConfig, std::string* errMsg);
int api_set_server_config(const std::string& server, const ServerConfig& svrConfig, std::string* errMsg);

/* 高级设置 */
int api_get_advance_config(const std::string& server, ClientConfig* clientConfig, DatabaseConfig* dbConfig,
    ProxyConfig* proxyConfig, ServerConfig* svrConfig, std::string* errMsg);
int api_set_advance_config(const std::string& server, const ClientConfig& clientConfig, const DatabaseConfig& dbConfig,
    const ProxyConfig& proxyConfig, const ServerConfig& svrConfig, std::string* errMsg);

