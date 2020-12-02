#include <string>
#include "config/config.h"

struct DatabaseConfig;
struct ClientConfig;
class  MysqlDbPool;
class  BiliUserSpider;

enum class StopMode {
    Smooth = 0,
    Force = 1
};

extern DatabaseConfig g_dbConfig;
extern ClientConfig   g_clientConfig;

extern std::string g_projDir;
extern std::string g_srcDir;
extern std::string g_dataDir;
extern std::string g_cfgDir;
extern std::string g_logsDir;
extern std::string g_statusFile;

extern bool g_shouldStop;
extern bool g_forceStop;

extern int g_numThreads;
extern int g_clientId;

extern std::string g_svrAddr;

extern std::string g_clientToken;

extern std::string g_clientOS;

extern BiliUserSpider g_biliUserSpider;

extern void g_stop(StopMode mode);
extern bool g_read_config();



