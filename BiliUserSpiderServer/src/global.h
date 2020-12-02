/*******************************************************
** description: Forward declaration
**
** author: Leopard-C
**
** update: 2020/12/02
*******************************************************/
#include <string>
#include "config/config.h"

struct DatabaseConfig;
struct ClientConfig;
struct ServerConfig;
struct ProxyConfig;

extern DatabaseConfig g_dbConfig;
extern ClientConfig   g_clientConfig;
extern ServerConfig   g_svrConfig;
extern ProxyConfig    g_proxyConfig;

extern std::string g_projDir;
extern std::string g_srcDir;
extern std::string g_dataDir;
extern std::string g_cfgDir;
extern std::string g_logsDir;
extern std::string g_statusFile;

extern bool g_isRunning;

extern void g_start();
extern void g_stop();
extern bool g_read_config();
extern bool g_reload_config();
extern bool g_write_config();

