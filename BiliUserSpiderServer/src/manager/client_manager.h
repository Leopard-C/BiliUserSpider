/*****************************************************************************
** class name:  ClientManager
**
** description: 客户端管理器，负责处理客户端的连接、心跳检测、退出等
**
** author: Leopard-C
**
** update: 2020/12/02
*****************************************************************************/
#pragma once
#include <deque>
#include <map>
#include <mutex>
#include "../json/json.h"


struct ClientInfo {
    /* 客户端ID（全局唯一） */
    int id;

    /* 客户端启动的爬虫线程数量 */
    int threads;

    /* 加入爬虫队列时间 */
    time_t join_time;

    /* 上一次心跳（申请/提交任务） */
    time_t last_heartbeat;

    /* 客户端IP */
    std::string host;

    /* 客户端主机的OS: Linux/Win */
    std::string os;

    /* 爬虫数量 历史快照（30分钟内） */
    std::deque<std::pair<time_t, int>> count_snapshot;

    /* 当前累计爬虫数量 */
    int count() const;

    /* 过去多少秒的平均爬虫速度 */
    /*   默认-1，表示从运行开始到现在的平均爬虫速度 */
    double speed(int last_seconds = -1) const;

    /*
     * 是否应该退出
     *   0: 否
     *   1: 平滑退出
     *   2: 强制退出
    **/
    int should_quit = 0; 
};


class ClientManager {
public:
    ~ClientManager();

    void start();
    void stop();

    /* 接受客户端加入，返回唯一的ID值 */
    int join(const std::string& host, const std::string& os, int threads);

    /* 客户端断退出 */
    void quit(int clientId);

    /* 心跳检测，判断客户端是否在线 */
    bool heartbeat(int clientId, int count, int* shouldQuit);

    /* 获取所有客户端信息 */
    void getClientsInfo(Json::Value& data);

    /* 输出正在运行的客户端信息 */
    void printClients() const;

public:
    /* 通知客户端退出 */
    /* 客户端下次发送请求时，将收到退出指令 */
    /* mode: 1(平滑退出), 2(强制退出) */
    void notifyClientToQuit(int clientId, bool isForceStop = false);

public:
    friend void backgroundThread(ClientManager* mgr);

private:
    int current_client_id_ = 0;
    bool bg_thread_quit_ = true;
    std::mutex mutex_;
    std::map<int, ClientInfo> clients_;
};

