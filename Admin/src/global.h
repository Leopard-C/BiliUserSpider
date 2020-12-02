/*******************************************************
** description: Forward declaration
**
** author: Leopard-C
**
** update: 2020/12/02
*******************************************************/
#pragma once
#include <string>
#include "config/config.h"

struct DatabaseConfig;
struct ClientConfig;
struct ProxyConfig;
struct ServerConfig;
class  MysqlDbPool;
class  BiliUserSpider;

enum class StopMode {
    Smooth = 0,
    Force = 1
};

extern DatabaseConfig g_dbConfig;
extern ClientConfig   g_clientConfig;
extern ProxyConfig    g_proxyConfig;
extern ServerConfig   g_svrConfig;

extern std::string g_projDir;
extern std::string g_srcDir;
extern std::string g_cfgDir;
extern std::string g_logsDir;

extern std::string g_svrAddr;

extern std::string g_svrPwd;

extern bool g_isSvrRunning;
extern bool g_isSvrConnected;

