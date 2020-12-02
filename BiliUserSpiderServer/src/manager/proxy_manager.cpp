#include "proxy_manager.h"
#include "../log/logger.h"
#include "../json/json.h"
#include "../util/string_utils.h"
#include "../iclient/ic/ic.h"
#include "../mysql/mysql_instance.h"
#include "../mysql/mysql_pool.h"
#include "../config/config.h"
#include "../global.h"
#include <jsoncpp/json/json.h>
#include <fstream>
#include <thread>

void backgroundThreadAiErYun(ProxyManager* mgr);
void backgroundThreadQingTing(ProxyManager* mgr);

#define USE_AiErYun


ProxyManager::ProxyManager() {
    LogF("Construct ProxyManager");
}

ProxyManager::~ProxyManager() {
    int count = 50;
#ifdef USE_AiErYun
    while (--count && !bg_thread_AiErYun_quit_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
#else
#   ifdef USE_QingTing
        while (--count && !bg_thread_QingTing_quit_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
#   endif
#endif
    LogF("Destruct ProxyManager");
}

void ProxyManager::start() {
    std::lock_guard<std::mutex> lck(mutex_);
    if (mysql_db_pool_) {
        LWarn("mysql_db_pool_ is not NULL. ProxyManager maybe started already.");
        return;
    }
    mysql_db_pool_ = new MysqlDbPool(g_dbConfig.host, g_dbConfig.port,
                                     g_dbConfig.user, g_dbConfig.password,
                                     g_dbConfig.name, 1);

#ifdef USE_AiErYun
    LInfo("Proxy type: AiErYun");
    if (bg_thread_AiErYun_quit_) {
        bg_thread_AiErYun_quit_ = false;
        std::thread t(backgroundThreadAiErYun, this);
        t.detach();
    }
#else
#   ifdef USE_QingTing
        LInfo("Proxy type: QingTing");
        if (bg_thread_AiErYun_quit_) {
            bg_thread_QingTing_quit_ = false;
            std::thread t(backgroundThreadQingTing, this);
            t.detach();
        }
#   endif
#endif
}

void ProxyManager::stop() {
    g_stop();
    std::lock_guard<std::mutex> lck(mutex_);
    if (mysql_db_pool_) {
        delete mysql_db_pool_;
        mysql_db_pool_ = nullptr;
    }
}


/* 添加新的代理ip到数据库 */
bool ProxyManager::addProxies(const std::vector<ic::ProxyData>& proxies) {
    char* sql = new char[64 + 32 * proxies.size()];
    char* end = sql;
    end += sprintf(end, "INSERT IGNORE INTO proxy (`host`,`port`) VALUE");
    for (auto& proxyData : proxies) {
        end += sprintf(end, "('%s',%u),", proxyData.host.c_str(), proxyData.port);
    }
    *(end-1) = ';';
    *end = '\0';

    /* 尝试5次 */
    bool ret = false;
    for (int i = 0; i < 5; ++i) {
        MysqlInstance mysql(mysql_db_pool_);
        if (mysql.bad() || !mysql.exec(sql)) {
            continue;
        }
        else {
            ret = true;
            break;
        }
    }

    delete[] sql;
    return ret;
}


/* 清除数据库中失败较次数多的代理 */
bool ProxyManager::removeProxies() {
    char sql[64];
    sprintf(sql, "DELETE FROM proxy WHERE `error_count`>%d;", g_clientConfig.proxy_max_error_times);
    bool ret = false;
    for (int i = 0; i < 5; ++i) {
        MysqlInstance mysql(mysql_db_pool_);
        if (mysql.bad() || !mysql.exec(sql)) {
            continue;
        }
        else {
            ret = true;
            break;
        }
    }
    return ret;
}


/****************************************************************************
 *
 *       获取代理
 *          1. 埃尔云代理
 *          2. 蜻蜓代理
 *
****************************************************************************/

/* 埃尔云代理 */
void backgroundThreadAiErYun(ProxyManager* mgr) {
    LogF("BgThread AiErYun started!");
    int errorCount = 0;

    while (g_isRunning) {
        ic::Url url("http://www.lyunplus.com/api/proxy/list");
        url.setParam("orderCode", g_proxyConfig.order_id);
        url.setParam("count", 20);
        url.setParam("format", "json");
        ic::Request request(url.toString());
        request.setVerifySsl(false);
        request.setTimeout(3000);
        auto response = request.perform();
        if (response.getStatus() != ic::Status::SUCCESS ||
            response.getHttpStatus() != ic::http::StatusCode::HTTP_200_OK)
        {
            LError("GET Error: {}", url.toString());
            LError("iclient Status: {}", ic::to_string(response.getStatus()));
            if (errorCount++ > 10) {
                LError("errorCount:{}", errorCount);
                break;
            }
            std::this_thread::sleep_for(std::chrono::seconds(2));
            continue;
        }
        Json::Reader reader;
        Json::Value root;
        if (!reader.parse(response.getData(), root, false)) {
            LError("Parse json failed");
            continue;
        }
        if (checkNull(root, "code")) {
            LError("Missing param:code");
            continue;
        }
        std::string code = getStrVal(root, "code");
        if (code != "0") {
            LError("Code is {}", code);
            if (code == "10130") {
                std::this_thread::sleep_for(std::chrono::seconds(3));
            }
            else {
                if (errorCount++ > 10) {
                    LError("errorCount:{}", errorCount);
                    break;
                }
            }
            continue;
        }
        if (checkNull(root, "data")) {
            LError("Missing param: data");
            errorCount++;
            continue;
        }
        auto& data = root["data"];
        if (data.empty()) {
            LError("No ip returned");
            errorCount++;
            continue;
        }

        int error = false;
        int size = data.size();
        std::vector<ic::ProxyData> proxiesData;
        proxiesData.reserve(size);
        for (int i = 0; i < size; ++i) {
            auto& proxy = data[i];
            if (checkNull(proxy, "port", "ip")) {
                LError("Missing param: port/ip");
                error = true;
                break;
            }
            ic::ProxyData proxyData;
            proxyData.host = getStrVal(proxy, "ip");
            std::string port_str = getStrVal(proxy, "port");
            proxyData.port = std::atoi(port_str.c_str());
            //LogF("%s:%u", host.c_str(), port);
            proxiesData.emplace_back(proxyData);
        }
        LogF("-----------------------------");
        if (error) {
            errorCount++;
            continue;
        }

        {
            std::lock_guard<std::mutex> lck(mgr->mutex_);
            if (!mgr->addProxies(proxiesData)) {
                LError("Add proxies failed");
                break;
            }
            LogF("Add %d proxies", proxiesData.size());
            if (!mgr->removeProxies()) {
                LError("Remove proxies failed");
                break;
            }
            LogF("Remove proxies ok");
        }

        errorCount = 0;

        /* 间隔指定时间调用代理IP服务商的API */
        int count = g_proxyConfig.interval_query * 10;
        for (int i = 0; i < count && g_isRunning; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    LogF("ProxyManager BgThread AiErYun quit. g_isRunning: {}", g_isRunning);
    mgr->bg_thread_AiErYun_quit_ = true;
    mgr->stop();
}


/* 蜻蜓代理 */
void backgroundThreadQingTing(ProxyManager* mgr) {
    LogF("BgThread QingTing started!");
    int errorCount = 0;

    while (g_isRunning) {
        ic::Url url("https://proxyapi.horocn.com/api/v2/proxies");
        url.setParam("order_id", g_proxyConfig.order_id);
        url.setParam("num", "10");
        url.setParam("format", "json");
        url.setParam("line_separator", "win");
        url.setParam("can_repeat", "yes");
        url.setParam("loc_name", "中国");
        url.setParam("user_token", g_proxyConfig.user_id);

        ic::Request request(url.toString());
        request.setVerifySsl(false);
        request.setTimeout(3000);

        auto response = request.perform();
        if (response.getStatus() != ic::Status::SUCCESS ||
            response.getHttpStatus() != ic::http::StatusCode::HTTP_200_OK)
        {
            LError("GET Error: {}", url.toString());
            LError("iclient Status: {}", ic::to_string(response.getStatus()));
            if (errorCount++ > 10) {
                LError("errorCount:{}", errorCount);
                break;
            }
            std::this_thread::sleep_for(std::chrono::seconds(2));
            continue;
        }

        Json::Reader reader;
        Json::Value root;
        if (!reader.parse(response.getData(), root, false)) {
            LError("Parse json failed");
            continue;
        }

        if (checkNull(root, "code", "msg")) {
            LError("Missing param:code/msg");
            continue;
        }
        bool code = getBoolVal(root, "code");
        if (code != 0) {
            LError("Code is {}", code);
            if (errorCount++ > 10) {
                LError("errorCount: {}", errorCount);
                break;
            }
            continue;
        }
        if (checkNull(root, "data")) {
            LError("Missing param: data");
            errorCount++;
            continue;
        }
        auto& data = root["data"];
        if (data.empty()) {
            LError("Get IP dataset empty");
            errorCount++;
            continue;
        }

        int error = false;
        int size = data.size();
        std::vector<ic::ProxyData> proxiesData;
        proxiesData.reserve(size);
        for (int i = 0; i < size; ++i) {
            auto& proxy = data[i];
            if (checkNull(proxy, "host", "port")) {
                LError("Missing param: host/port");
                error = true;
                break;
            }
            ic::ProxyData proxyData;
            proxyData.host = getStrVal(proxy, "host");
            std::string port_str = getStrVal(proxy, "port");
            proxyData.port = std::atoi(port_str.c_str());
            //LogF("%s:%u", host.c_str(), port);
            proxiesData.emplace_back(proxyData);
        }
        LogF("-----------------------------");
        if (error) {
            errorCount++;
            continue;
        }

        {
            std::lock_guard<std::mutex> lck(mgr->mutex_);
            if (!mgr->addProxies(proxiesData)) {
                LError("Add proxies failed");
                break;
            }
            LogF("Add %d proxies", proxiesData.size());
            if (!mgr->removeProxies()) {
                LError("Remove proxies failed");
                break;
            }
            LogF("Remove proxies ok");
        }

        errorCount = 0;

        /* 间隔指定时间调用代理IP服务商的API */
        int count = g_proxyConfig.interval_query * 10;
        for (int i = 0; i < count && g_isRunning; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    LogF("ProxyManager BgThread QingTing quit. g_isRunning: %s", g_isRunning ? "true" : "false");
    mgr->bg_thread_QingTing_quit_ = true;
    mgr->stop();
}

