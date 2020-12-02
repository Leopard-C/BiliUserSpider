/**************************************************************
** class name:  ClientConfig
**              DatabaseConfig
**              ProxyConfig
**              ServerConfig
**
** description: 和服务器上相应配置保持一致
**
** author: Leopard-C
**
** update: 2020/12/02
***************************************************************/
#pragma once
#include <string>
#include <vector>
#include "../json/json.h"


/*
 *  客户端配置
**/
struct ClientConfig {
    /* 调用B站API的类型 */
    /*     0: Web             */
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


/*
 * 代理IP配置
 *  并非每个代理IP服务商都有以下配置项
 */
struct ProxyConfig {
    /* 代理IP服务商订单号 */
    std::string order_id;

    /* 用户标识(可选) */
    std::string user_id;

    /* 最小提取间隔 */
    int interval_query;

    bool parseFromJson(Json::Value& json);
    Json::Value toJson() const;
    bool isValid() const;
};


/*
 *  服务器配置
**/
struct ServerConfig {
    /* 绑定的ip */
    /* 只能从本地文件读取 */
    std::vector<std::string> IPs;

    /* 绑定的端口 */
    /*  只能从服务器本地文件读取 */
    int port;
    
    /* 线程池大小 */
    /*  只能从服务器本地文件读取 */
    int thread_pool_size;
    
    /* 验证 管理员密码 */
    /*  只能从服务器本地文件读取 */
    std::string admin_password;

    /* 验证 客户端Token */
    /*  只能从服务器本地文件读取 */
    std::string client_token;

    /* 多少秒内有心跳才能存活 */
    int client_live_heartbeat;

    /* 检测客户端是否在线的间隔(秒) */
    int interval_detect_client_live;

    /* 任务存活时间（从申请到提交的最长时间）,超时回收,重新分配 */
    int main_task_timeout;

    bool parseFromJson(Json::Value& json);
    Json::Value toJson() const;
    bool isValid() const;
};


/* 从Json对象解析(所有配置) */
bool g_parse_from_json(Json::Value& root,
                       ClientConfig* clientConfig, DatabaseConfig* dbConfig,
                       ProxyConfig* proxyConfig, ServerConfig* svrConfig,
                       bool checkValid = false);

