/*******************************************************
** class name:  SpiderClientInfo
**
** description: 爬虫客户端信息结构体
**
** author: Leopard-C
**
** update: 2020/12/02
*******************************************************/
#pragma once
#include <string>
#include <ctime>
#include "../json/json.h"

struct SpiderClientInfo {
    int id;
    int threads; /* 启动的爬虫线程数 */
    int count;   /* 累计爬取的用户信息数量 */
    time_t join_time;
    double average_speed;    /* 总平均速度 */
    double average_speed_3m; /* 3分钟内平均速度 */
    double average_speed_5m;
    double average_speed_10m;
    double average_speed_30m;
    std::string os;    /* 客户端操作系统 */
    std::string host;  /* 客户端IP */
    bool parseFromJson(Json::Value& data);
};

/* 排序 */
void sortClientsInfoById_ASC(std::vector<SpiderClientInfo>& infos);
void sortClientsInfoById_DESC(std::vector<SpiderClientInfo>& infos);
void sortClientsInfoById(std::vector<SpiderClientInfo>& infos, bool isASC);

void sortClientsInfoByThreads_ASC(std::vector<SpiderClientInfo>& infos);
void sortClientsInfoByThreads_DESC(std::vector<SpiderClientInfo>& infos);
void sortClientsInfoByThreads(std::vector<SpiderClientInfo>& infos, bool isASC);

void sortClientsInfoByCount_ASC(std::vector<SpiderClientInfo>& infos);
void sortClientsInfoByCount_DESC(std::vector<SpiderClientInfo>& infos);
void sortClientsInfoByCount(std::vector<SpiderClientInfo>& infos, bool isASC);

void sortClientsInfoByJoinTime_ASC(std::vector<SpiderClientInfo>& infos);
void sortClientsInfoByJoinTime_DESC(std::vector<SpiderClientInfo>& infos);
void sortClientsInfoByJoinTime(std::vector<SpiderClientInfo>& infos, bool isASC);

void sortClientsInfoByHost_ASC(std::vector<SpiderClientInfo>& infos);
void sortClientsInfoByHost_DESC(std::vector<SpiderClientInfo>& infos);
void sortClientsInfoByHost(std::vector<SpiderClientInfo>& infos, bool isASC);

void sortClientsInfoByOS_ASC(std::vector<SpiderClientInfo>& infos);
void sortClientsInfoByOS_DESC(std::vector<SpiderClientInfo>& infos);
void sortClientsInfoByOS(std::vector<SpiderClientInfo>& infos, bool isASC);

void sortClientsInfoByAvgSpeed_ASC(std::vector<SpiderClientInfo>& infos);
void sortClientsInfoByAvgSpeed_DESC(std::vector<SpiderClientInfo>& infos);
void sortClientsInfoByAvgSpeed(std::vector<SpiderClientInfo>& infos, bool isASC);

void sortClientsInfoByAvgSpeed_3m_ASC(std::vector<SpiderClientInfo>& infos);
void sortClientsInfoByAvgSpeed_3m_DESC(std::vector<SpiderClientInfo>& infos);
void sortClientsInfoByAvgSpeed_3m(std::vector<SpiderClientInfo>& infos, bool isASC);

void sortClientsInfoByAvgSpeed_5m_ASC(std::vector<SpiderClientInfo>& infos);
void sortClientsInfoByAvgSpeed_5m_DESC(std::vector<SpiderClientInfo>& infos);
void sortClientsInfoByAvgSpeed_5m(std::vector<SpiderClientInfo>& infos, bool isASC);

void sortClientsInfoByAvgSpeed_10m_ASC(std::vector<SpiderClientInfo>& infos);
void sortClientsInfoByAvgSpeed_10m_DESC(std::vector<SpiderClientInfo>& infos);
void sortClientsInfoByAvgSpeed_10m(std::vector<SpiderClientInfo>& infos, bool isASC);

void sortClientsInfoByAvgSpeed_30m_ASC(std::vector<SpiderClientInfo>& infos);
void sortClientsInfoByAvgSpeed_30m_DESC(std::vector<SpiderClientInfo>& infos);
void sortClientsInfoByAvgSpeed_30m(std::vector<SpiderClientInfo>& infos, bool isASC);

