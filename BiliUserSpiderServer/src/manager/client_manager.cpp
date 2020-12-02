#include "client_manager.h"
#include "../config/config.h"
#include "../log/logger.h"
#include "../status/status_code.h"
#include "../global.h"
#include <thread>
#include <chrono>

void backgroundThread(ClientManager* mgr);


/* 当前累计爬虫数量 */
int ClientInfo::count() const {
    return count_snapshot.empty() ? 0 : count_snapshot.back().second;
}


/* 最近多长时间内的爬虫速度 */
/* 默认-1，表示从运行开始到现在的平均爬虫速度 */
double ClientInfo::speed(int last_seconds/* = -1*/) const {
    time_t now = time(NULL);
    size_t size = count_snapshot.size();
    if (size == 0) {
        return .0;
    }
    if (last_seconds == -1) {
        return count_snapshot.back().second / static_cast<double>(now - join_time);
    }
    if (size == 1) {
        return .0;
    }

    for (size_t i = size - 2; i > 0; --i) {  // "i > 0", not ">="
        if (now - count_snapshot[i].first > last_seconds) {
            return (count_snapshot[size-1].second - count_snapshot[i].second) /
                static_cast<double>(count_snapshot[size-1].first - count_snapshot[i].first);
        }
    }

    return (count_snapshot[size-1].second - count_snapshot[0].second) /
        static_cast<double>(count_snapshot[size-1].first - count_snapshot[0].first);
}

ClientManager::~ClientManager() {
    stop();
    int count = 50;
    while (--count && !bg_thread_quit_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    LogF("Destruct ClientManager");
}


void ClientManager::start() {
    if (bg_thread_quit_) {
        bg_thread_quit_ = false;
        std::thread t(backgroundThread, this);
        t.detach();
    }
}

void ClientManager::stop() {
    g_stop();
}


void ClientManager::printClients() const {
    LogF("Living clients: %d", clients_.size());
    for (const auto& client : clients_) {
        LogF("    %3d  %16s  %3d", client.first, client.second.host.c_str(), client.second.threads);
    }
}


/* 接受客户端加入，返回唯一的ID值 */
int ClientManager::join(const std::string& host, const std::string& os, int threads) {
    std::lock_guard<std::mutex> lck(mutex_);
    time_t now = time(NULL);
    ClientInfo client;
    client.host = host;
    client.os = os;
    client.threads = threads;
    client.join_time = now;
    client.last_heartbeat = now;
    clients_.emplace(current_client_id_, client);
    LInfo("New client join: {:>3d}  {:>16}", current_client_id_, host);
    return current_client_id_++;
}


/* 客户端主动停止运行 */
void ClientManager::quit(int clientId) {
    std::lock_guard<std::mutex> lck(mutex_);
    auto findIt = clients_.find(clientId);
    if (findIt == clients_.end()) {
        LCritical("Client id:{} is not in list.", clientId);
    }
    else {
        LInfo("Client disconnect: {:>3d}  {:16}", findIt->first, findIt->second.host);
        clients_.erase(findIt);
    }
}


/* 心跳检测，判断客户端是否在线 */
bool ClientManager::heartbeat(int clientId, int count, int* shouldQuit) {
    std::lock_guard<std::mutex> lck(mutex_);
    auto findIt = clients_.find(clientId);
    if (findIt == clients_.end()) {
        LCritical("Client id:{} is not in list.", clientId);
        return false;
    }
    auto& client = findIt->second;
    *shouldQuit = client.should_quit;
    time_t now = time(NULL);
    client.last_heartbeat = now;
    client.count_snapshot.emplace_back(now, count);
    /* 清除30分钟之间的爬虫数量记录 */
    for (auto it = client.count_snapshot.begin(); it != client.count_snapshot.end(); ) {
        if (now - it->first > 30 * 60) {
            it = client.count_snapshot.erase(it);
        }
        else {
            ++it;
        }
    }
    LInfo("Heartbeat: {:>3d}  {:16}", findIt->first, findIt->second.host);
    return true;
}


/* 获取所有客户端信息 */
void ClientManager::getClientsInfo(Json::Value& data) {
    std::lock_guard<std::mutex> lck(mutex_);
    for (auto& client : clients_) {
        Json::Value clientNode;
        clientNode["id"] = client.first;
        clientNode["os"] = client.second.os;
        clientNode["host"] = client.second.host;
        clientNode["join_time"] = Json::Int64(client.second.join_time);
        clientNode["heartbeat"] = Json::Int64(client.second.last_heartbeat);
        clientNode["threads"] = client.second.threads;
        clientNode["count"] = client.second.count();
        /* 速度放大1000倍，客户端接收后除以1000得到真实速度 */
        /* 加1，防止因为精度问题，导致客户端解析为负数 */
        clientNode["avg"] = Json::Int(client.second.speed(-1) * 1000) + 1;
        clientNode["avg_3m"] = Json::Int(client.second.speed(60*3) * 1000) + 1;
        clientNode["avg_5m"] = Json::Int(client.second.speed(60*5) * 1000) + 1;
        clientNode["avg_10m"] = Json::Int(client.second.speed(60*10) * 1000) + 1;
        clientNode["avg_30m"] = Json::Int(client.second.speed(60*30) * 1000) + 1;
        data.append(clientNode);
    }
}

/* 通知客户端退出 */
/* 客户端下次发送请求时，将收到退出指令 */
/* mode: 1(平滑退出), 2(强制退出) */
void ClientManager::notifyClientToQuit(int clientId, bool isForceStop/* = false*/) {
    std::lock_guard<std::mutex> lck(mutex_);
    auto findIt = clients_.find(clientId);
    if (findIt != clients_.end()) {
        if (isForceStop) {
            findIt->second.should_quit = 2;
        }
        else {
            findIt->second.should_quit = 1;
        }
    }
}


/* 后台线程 */
void backgroundThread(ClientManager* mgr) {
    auto& clients = mgr->clients_;

    while (g_isRunning) {
        {
            std::lock_guard<std::mutex> lck(mgr->mutex_);
            time_t now = time(NULL);
            for (auto it = clients.begin(); it != clients.end(); ) {
                time_t dur = abs(now - it->second.last_heartbeat);
                /* 踢出最小指定时间内没有心跳的的客户端 */
                if (dur > g_svrConfig.client_live_heartbeat) {
                    LWarn("Force client disconnect:  {:>3d}  {:16}", it->first, it->second.host);
                    clients.erase(it++);
                }
                else {
                    ++it;
                }
            }
            mgr->printClients();
        }

        /* 间隔指定时间 检测是否有客户端掉线 */
        int count = g_svrConfig.interval_detect_client_live * 10;
        while (--count && g_isRunning) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    LogF("ClientManager BgThread quit. g_isRunning: %s", g_isRunning ? "true" : "false");
    mgr->bg_thread_quit_ = true;
    mgr->stop();
}

