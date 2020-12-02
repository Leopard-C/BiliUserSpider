#include "config.h"
#include "../global.h"
#include "../json/json.h"
#include "../log/logger.h"
#include <fstream>


/* 从Json对象解析 */
bool ClientConfig::parseFromJson(Json::Value& client) {
    if (checkNull(client, "proxy_max_error_times", "mid_max_error_times", "timeout",
                  "main_task_step_times", "sub_task_step", "proxy_sleeptime_map",
                  "interval_proxypool_sync", "bili_api_type"))
    {
        LError("Missing key:client[timeout/sub_task_step/...]");
        return false;
    }
    bili_api_type = getIntVal(client, "bili_api_type");
    main_task_step_times = getIntVal(client, "main_task_step_times");
    sub_task_step = getIntVal(client, "sub_task_step");
    timeout = getIntVal(client, "timeout");
    proxy_max_error_times = getIntVal(client, "proxy_max_error_times");
    mid_max_error_times = getIntVal(client, "mid_max_error_times");
    interval_proxypool_sync = getIntVal(client, "interval_proxypool_sync");
    Json::Value& proxySleeptimeMap = client["proxy_sleeptime_map"];
    int size = proxySleeptimeMap.size();
    for (int i = 0; i < size; ++i) {
        Json::Value& pair = proxySleeptimeMap[i]; 
        if (!pair.isArray() || pair.size() != 2) {
            LError("Invalid proxy_sleeptime_map");
            return false;
        }
        proxy_sleeptime_map.emplace(pair[0].asInt(), pair[1].asInt());
    }
    return true;
}

bool DatabaseConfig::parseFromJson(Json::Value& db) {
    if (checkNull(db, "user", "password", "host", "port", "name")) {
        LError("Missing key:database[host/port/...]");
        return false;
    }
    host = getStrVal(db, "host");
    port = getIntVal(db, "port");
    user = getStrVal(db, "user");
    password = getStrVal(db, "password");
    name = getStrVal(db, "name");
    return true;
}

bool ProxyConfig::parseFromJson(Json::Value& proxy) {
    if (checkNull(proxy, "order_id", "user_id", "interval_query")) {
        LError("Missing key:proxy[order_id/user_id/...]");
        return false;
    }
    order_id = getStrVal(proxy, "order_id");
    user_id = getStrVal(proxy, "user_id");
    interval_query = getIntVal(proxy, "interval_query");
    return true;
}

bool ServerConfig::parseFromJson(Json::Value& svr) {
    if (checkNull(svr, "ip", "port", "thread_pool_size", "main_task_timeout",
                  "client_live_heartbeat", "interval_detect_client_live",
                  "admin_password", "client_token"))
    {
        LError("Missing key:server[ip/port/...]");
        return false;
    }
    Json::Value& ips = svr["ip"];
    int ips_count = ips.size();
    if (ips_count == 0) {
        LError("There should be at least 1 ip address");
        return false;
    }
    for (int i = 0; i < ips_count; ++i) {
        IPs.emplace_back(ips[i].asString());
    }
    port = getIntVal(svr, "port");
    thread_pool_size = getIntVal(svr, "thread_pool_size");
    interval_detect_client_live = getIntVal(svr, "interval_detect_client_live");
    client_live_heartbeat = getIntVal(svr, "client_live_heartbeat");
    main_task_timeout = getIntVal(svr, "main_task_timeout");
    client_token = getStrVal(svr, "client_token");
    admin_password = getStrVal(svr, "admin_password");
    return true;
}


/* 检测配置项是否有效 */
bool ClientConfig::isValid() const {
    return (main_task_step_times > 0 && main_task_step_times <= 5)
        && (sub_task_step > 0 && sub_task_step <= 20)
        && (timeout > 200 && timeout < 20000)  /* 500ms ~ 20s */
        && (proxy_max_error_times > 0 && proxy_max_error_times <= 100)
        && (mid_max_error_times > 0 && mid_max_error_times <= 20)
        && (interval_proxypool_sync > 0 && interval_proxypool_sync <= 60);
}

bool DatabaseConfig::isValid() const {
    return !host.empty() && !user.empty() && !password.empty()
        && port > 0 && port < 65536 && !name.empty();
}

bool ProxyConfig::isValid() const {
    return !order_id.empty() /*&& user_id.empty() */
        && interval_query > 0 && interval_query <= 120;
}

bool ServerConfig::isValid() const {
    return !IPs.empty() && !IPs[0].empty() && port > 0 && port < 65536
        && thread_pool_size > 0 && thread_pool_size <= 128
        && client_live_heartbeat >= 60 && client_live_heartbeat <= 1800 /* 1min ~ 30min */
        && interval_detect_client_live > 0 && interval_detect_client_live <= 60
        && main_task_timeout >= 60 && main_task_timeout <= 1800
        && admin_password.length() >= 8 && client_token.length() >= 8;
}


/* 输出到Json */
Json::Value ClientConfig::toJson() const {
    Json::Value client;
    client["bili_api_type"] = bili_api_type;
    client["main_task_step_times"] = main_task_step_times;
    client["sub_task_step"] = sub_task_step;
    client["timeout"] = timeout;
    client["proxy_max_error_times"] = proxy_max_error_times;
    client["mid_max_error_times"] = mid_max_error_times;
    client["interval_proxypool_sync"] = interval_proxypool_sync;
    Json::Value proxySleeptimeMapNode;
    for (auto& pair : proxy_sleeptime_map) {
        Json::Value pairNode;
        pairNode.append(pair.first);
        pairNode.append(pair.second);
        proxySleeptimeMapNode.append(pairNode);
    }
    client["proxy_sleeptime_map"] = proxySleeptimeMapNode;
    return client;
}

Json::Value DatabaseConfig::toJson() const {
    Json::Value db;
    db["host"] = host;
    db["port"] = port;
    db["name"] = name;
    db["user"] = user;
    db["password"] = password;
    return db;
}

Json::Value ProxyConfig::toJson() const {
    Json::Value proxy;
    proxy["order_id"] = order_id;
    proxy["user_id"] = user_id;
    proxy["interval_query"] = interval_query;
    return proxy;
}

Json::Value ServerConfig::toJson() const {
    Json::Value svr;
    for (auto& ip : IPs) {
        svr["ip"].append(ip);
    }
    svr["port"] = port;
    svr["admin_password"] = admin_password;
    svr["client_token"] = client_token;
    svr["thread_pool_size"] = thread_pool_size;
    svr["client_live_heartbeat"] = client_live_heartbeat;
    svr["interval_detect_client_live"] = interval_detect_client_live;
    svr["main_task_timeout"] = main_task_timeout;
    return svr;
}

/* 从Json对象解析(所有配置) */
bool g_parse_from_json(Json::Value& root,
                       ClientConfig* clientConfig, DatabaseConfig* dbConfig,
                       ProxyConfig* proxyConfig, ServerConfig* svrConfig,
                       bool checkValid/* = false*/)
{
    if (checkNull(root, "client", "database", "proxy", "server")) {
        LError("Missing key:client/database/...");
        return false;
    }
    if (!(clientConfig->parseFromJson(root["client"])
        && dbConfig->parseFromJson(root["database"])
        && proxyConfig->parseFromJson(root["proxy"])
        && svrConfig->parseFromJson(root["server"])))
    {
        LError("Parse from json failed");
        return false;
    }

    /* 检查配置项是否有效 */
    if (!checkValid) {
        return true;
    }
    if (!clientConfig->isValid()) {
        LError("ClientConfig is invalid");
        return false;
    }
    if (!dbConfig->isValid()) {
        LError("DatabaseConfig is invalid");
        return false;
    }
    if (!proxyConfig->isValid()) {
        LError("ProxyConfig is invalid");
        return false;
    }
    if (!svrConfig->isValid()) {
        LError("ServerConfig is invalid");
        return false;
    }
    return true;
}


/* 重新读取 config/config.json */
bool g_realod_config() {
    std::ifstream ifs(g_cfgDir + "/config.json");
    if (!ifs) {
        LError("Open configuration file failed");
        return false;
    }

    Json::Reader reader;
    Json::Value root;
    if (!reader.parse(ifs, root, false)) {
        LError("Parse configuration file failed");
        return false;
    }

    /* 解析到临时对象中 */
    ClientConfig clientConfig;
    DatabaseConfig dbConfig;
    ProxyConfig proxyConfig;
    ServerConfig svrConfig;
    if (!g_parse_from_json(root, &clientConfig, &dbConfig, &proxyConfig, &svrConfig, true)) {
        return false;
    }

    /* 赋值给全局对象 */
    g_clientConfig = clientConfig;
    g_dbConfig = dbConfig;
    g_proxyConfig = proxyConfig;
    g_svrConfig = svrConfig;
    return true;
}

/* 读取 config/config.json */
bool g_read_config() {
    return g_realod_config();
}

/* 将配置项写入文件 */
bool g_write_config() {
    std::ofstream ofs(g_cfgDir + "/config.json");
    if (!ofs) {
        LError("Open {}/config.json failed", g_cfgDir);
        return false;
    }
    Json::Value root;
    root["database"] = g_dbConfig.toJson();
    root["client"] = g_clientConfig.toJson();
    root["proxy"] = g_proxyConfig.toJson();
    root["server"] = g_svrConfig.toJson();
    ofs << root.toStyledString();
    return true;
}

