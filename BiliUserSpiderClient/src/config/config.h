/**************************************************************
** class name:  ClientConfig
**              DatabaseConfig
**
** description: 和服务器上相应配置保持一致或者是其子集
**
** author: Leopard-C
**
** update: 2020/12/02
***************************************************************/
#pragma once
#include <string>
#include <vector>
#include <map>
#include "../json/json.h"


/*
 *  客户端配置
**/
struct ClientConfig {
    /* 调用B站API的类型 */
    /*     0: Web            */
    /*   非0: App, 信息更详细 */
    int bili_api_type;

    /* 客户端向服务器申请的任务是 x 的多少倍 */
    /* 其中 x = sub_task_step*threads */
    int main_task_step_times;

    /* 客户端中 爬虫线程每次申请的任务数量(5-10个为宜) */
    int sub_task_step; 

    /* 爬取一个用户的(请求)超时时间(毫秒) */
    int timeout;

    /* 代理IP累计失败多少次就踢出 */
    int proxy_max_error_times;

    /* 爬取一个用户时，最多尝试几次失败才真正视为失败 */
    int mid_max_error_times;

    /* 客户端和服务器同步代理IP的间隔 */
    int interval_proxypool_sync;

    /* 代理IP数量对应的暂停时间 */
    /* 例如: <50, 20>, 表示代理IP数量低于50时，爬虫线程暂停20s */
    std::map<int, int> proxy_sleeptime_map;

    bool parseFromJson(Json::Value& json);
    Json::Value toJson() const;
    bool isValid() const;
};


/*
 *  数据库配置
**/
struct DatabaseConfig {
    int port;
    std::string host;
    std::string user;
    std::string password;
    std::string name;

    bool parseFromJson(Json::Value& json);
    Json::Value toJson() const;
    bool isValid() const;
};

