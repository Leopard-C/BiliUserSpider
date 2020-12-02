#include "util/path.h"
#include "config/config.h"
#include "log/logger.h"
#include "database/mysql_instance.h"
#include "biliuser_spider.h"
#include "global.h"
#include <thread>

#ifdef _WIN32
#  include <windows.h>
#else
#  include <unistd.h>
#  include <signal.h>
#endif


/* 各种目录 */
std::string g_projDir = util::getProjDir();
std::string g_srcDir = g_projDir + "/src";
std::string g_dataDir = g_projDir + "/data";
std::string g_cfgDir = g_projDir + "/config";
std::string g_logsDir = g_projDir + "/logs";

/* 各种配置 */
DatabaseConfig g_dbConfig;
ClientConfig   g_clientConfig;

/* 是否停止爬虫 */
bool g_shouldStop = false; /* 平滑退出，所有主任务都提交才会退出 */
bool g_forceStop = false;  /* 强制退出，所有爬虫线程立即停止，然后退出 */

/* 服务器地址 */
std::string g_svrAddr;

/* 客户端Token, 通信使用 */
std::string g_clientToken;

/* 当前客户端ID */
int g_clientId;

/* 爬虫线程数量 */
int g_numThreads;

/* 客户端对象 */
BiliUserSpider g_biliUserSpider;

/* 客户端系统 Win/Linux */
#ifdef _WIN32
  std::string g_clientOS = "Win";
#else
  std::string g_clientOS = "Linux";
#endif

/* 捕获Ctrl + C */
#ifdef _WIN32
  BOOL ctrlHandler(DWORD fdwCtrlType);
#else
void catchCtrlC(int code);
#endif


/*
 *  主函数
**/
int main(int argc, char *argv[]) {
#ifdef _WIN32
    SetConsoleCtrlHandler((PHANDLER_ROUTINE)ctrlHandler, TRUE);
#else
    signal(SIGINT, catchCtrlC);
#endif

    /* 创建日志文件夹 */
    if (!util::createDir(g_logsDir)) {
        LogF("Make directory %s failed", g_logsDir.c_str());
        return 1;
    }

    LInfo("****************************************************");
    LInfo("**                                                **");
    LInfo("**            Bilibili User Spider                **");
    LInfo("**                                                **");
    LInfo("****************************************************");

    /* 启动爬虫 */
    int ret = g_biliUserSpider.start();

    LInfo("Main function quit.");
    return ret;
}


/* 捕获Ctrl + C */
#ifdef _WIN32
BOOL ctrlHandler(DWORD fdwCtrlType) {
    switch (fdwCtrlType) {
    /* Ctrl C 事件 */
    case CTRL_C_EVENT:
        LError("Ctrl-C event");
        g_stop(StopMode::Smooth);
        LInfo("Exiting, please wait...");
        while (g_biliUserSpider.living_threads_num_ > 0) {
            LogF("Exiting, please wait...");
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        g_stop(StopMode::Force);
        LogF("Exiting, please wait...");
        while (g_biliUserSpider.proxy_mgr_ && g_biliUserSpider.proxy_mgr_->isRunning()) {
            LogF("Exiting, please wait...");
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        std::this_thread::sleep_for(std::chrono::seconds(2));
        break;
    /* 点击了关闭按钮, 强制退出 */
    case CTRL_CLOSE_EVENT:
        LogF("Ctrl-Close event\n");
        g_stop(StopMode::Force);
        LogF("\nExiting, please wait...\n");
        while (g_biliUserSpider.living_threads_num_ > 0) {
            LogF("Exiting, please wait...");
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        while (g_biliUserSpider.proxy_mgr_ && g_biliUserSpider.proxy_mgr_->isRunning()) {
            LogF("Exiting, please wait...");
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        std::this_thread::sleep_for(std::chrono::seconds(2));
        break;
    default:
        LogF("Unknown fdwCtrlType!\n");
        g_stop(StopMode::Smooth);
        LogF("\nExiting, please wait...\n");
        while (g_biliUserSpider.living_threads_num_ > 0) {
            LogF("Exiting, please wait...");
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        g_stop(StopMode::Force);
        while (g_biliUserSpider.proxy_mgr_ && g_biliUserSpider.proxy_mgr_->isRunning()) {
            LogF("Exiting, please wait...");
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        break;
    }
    return TRUE;
}
#else
void catchCtrlC(int code) {
    LError("Ctrl-C event");
    g_stop(StopMode::Smooth);
    LInfo("Exiting, please wait...");
    int count = 180;
    while (--count && g_biliUserSpider.living_threads_num_ > 0) {
        LogF("Exiting, please wait...");
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    g_stop(StopMode::Force);
    LogF("Exiting, please wait...");
    while (g_biliUserSpider.proxy_mgr_ && g_biliUserSpider.proxy_mgr_->isRunning()) {
        LogF("Exiting, please wait...");
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    std::this_thread::sleep_for(std::chrono::seconds(2));
}
#endif


static std::mutex s_server_mutex;

/* 平滑/强制停止客户端，可多次调用 */
void g_stop(StopMode mode) {
    std::lock_guard<std::mutex> lck(s_server_mutex);
    g_shouldStop = true;
    if (mode == StopMode::Force) {
        g_forceStop = true;
    }
}

