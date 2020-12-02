#include "config.h"
#include "../global.h"
#include "../json/json.h"
#include "../log/logger.h"
#include <fstream>


/* 从Json对象解析 */
bool ClientConfig::parseFromJson(Json::Value& client) {
    if (checkNull(client, "proxy_max_error_times", "mid_max_error_times", "timeout",
        "main_task_step_times", "sub_task_step", "proxy_sleeptime_map",
        "interval_proxypool_sync"))
    {
        LError("Missing key:client[timeout/...]");
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
