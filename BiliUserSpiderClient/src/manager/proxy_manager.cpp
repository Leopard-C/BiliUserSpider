#include "proxy_manager.h"
#include "../log/logger.h"
#include "../iclient/ic/ic.h"
#include "../json/json.h"
#include "../util/singleton.h"
#include "../api/api_proxy.h"
#include "../global.h"
#include <fstream>
#include <thread>

ProxyManager::ProxyManager() {
    LogF("Construct ProxyManager");
}

ProxyManager::~ProxyManager() {
	LogF("Destruct ProxyManager");
}


/* 获取随机代理 */
bool ProxyManager::getRandomProxy(ic::ProxyData& proxyData) {
    size_t size = proxies_.size();
    auto& sleeptime = g_clientConfig.proxy_sleeptime_map;
    for (auto& pair : sleeptime) {
        if (size < pair.first) {
            LInfo("Proxies number < {}, sleepping {}s", pair.first, pair.second);
            int ms_count = pair.second * 10;
            for (int i = 0; i < ms_count && !g_shouldStop; ++i) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            break;
        }
    }
	std::lock_guard<std::mutex> lck(mutex_);
	auto it = proxies_.begin();
	std::advance(it, rand() % proxies_.size());
	proxyData = it->first;
	return true;
}


void ProxyManager::start(MysqlDbPool* mysqlDbPool) {
    LInfo("Start ProxyManager");
    mysql_db_pool_ = mysqlDbPool;
    bg_thread_quit_ = false;
	std::thread t(backgroundThread, this);
	t.detach();
}

void ProxyManager::stop() {
    g_stop(StopMode::Force);
}


/* 代理发生错误一次 */
void ProxyManager::errorProxy(const ic::ProxyData& proxyData) {
	std::lock_guard<std::mutex> lck(mutex_);
	auto it = proxies_.find(proxyData);
	if (it != proxies_.end()) {
		it->second++;
	}
	else {
		proxies_.emplace(proxyData, 1);
	}
}


/* 清除代理 */
void ProxyManager::removeProxy(const ic::ProxyData& proxyData) {
	std::lock_guard<std::mutex> lck(mutex_);
	auto it = proxies_.find(proxyData);
	if (it != proxies_.end()) {
        //LError("Not found proxy in proxies_");
		proxies_.erase(it);
	}
    MysqlInstance mysql(mysql_db_pool_);
    if (mysql.bad()) {
        LError("Mysql instance is bad");
        return;
    }
    std::string errMsg;
    if (api_remove_proxy(mysql, proxyData, &errMsg) != 0) {
        LError("Remove proxy {}:{} failed. {}", proxyData.host, proxyData.port, errMsg);
    }
    else {
        //LInfo("Remove proxy : {}", proxyData.m_host);
    }
}


/*********************************************************************************
 *
 *  helper function for backgrounThreaed()
 *
**********************************************************************************

/* 向数据库报告所有出错代理 */
bool ProxyManager::reportAllErrorProxiesToDb() {
    if (proxies_.empty()) {
        return true;
    }
    MysqlInstance mysql(mysql_db_pool_);
    for (auto it = proxies_.begin(); it != proxies_.end(); ++it) {
        bool flag = false;
        for (int i = 0; i < 10; ++i) {
            std::string errMsg;
            int ret = api_error_proxy(mysql, it->first, it->second, &errMsg);
            if (ret == 0) {
                flag = true;
                break;
            }
            LError("{}", errMsg);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        if (!flag) {
            return false;
        }
    }
    return true;
}

/* 从数据库获取所有代理 */
bool ProxyManager::getAllProxiesFromDb(std::vector<ic::ProxyData>& proxiesData) {
    MysqlInstance mysql(mysql_db_pool_);
    bool flag = false;
    for (int i = 0; i < 10; ++i) {
        std::string errMsg;
        int ret = api_get_all_proxies(mysql, g_clientConfig.proxy_max_error_times, &proxiesData, &errMsg);
        if (ret == 0) {
            flag = true;
            break;
        }
        LError("{}", errMsg);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return flag;
}

/* 从数据库获取总代理数量 */
bool ProxyManager::getNumProxiesFromDb(int* num) {
    MysqlInstance mysql(mysql_db_pool_);
    bool flag = false;
    for (int i = 0; i < 10; ++i) {
        std::string errMsg;
        int ret = api_get_num_proxies(mysql, num, &errMsg);
        if (ret == 0) {
            flag = true;
            break;
        }
        LError("{}", errMsg);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return flag;
}

/* 后台线程 */
void ProxyManager::backgroundThread(ProxyManager* proxySvr) {
	auto& proxies = proxySvr->proxies_;

	while (!g_forceStop) {
		{
			/* 本地缓存 -> 数据库 */
			std::lock_guard<std::mutex> lck(proxySvr->mutex_);
            if (g_forceStop) {
                break;
            }
            if (!proxySvr->reportAllErrorProxiesToDb()) {
                LError("Report errored proxies failed");
                break;
            }

			/* 清空本地缓存 */
			proxies.clear();

			/* 数据库 -> 本地缓存 */
            std::vector<ic::ProxyData> proxiesVec;
            for (int i = 0; i < 10 && !g_forceStop; ++i) {
                /* 获取失败，等待2s */
                if (!proxySvr->getAllProxiesFromDb(proxiesVec)) {
                    for (int k = 0; k < 20 && !g_forceStop; ++k) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    }
                    continue;
                }
                /* 获取代理ip数量为0，等待5s */
                if (proxiesVec.empty()) {
                    for (int k = 0; k < 50 && !g_forceStop; ++k) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    }
                    continue;
                }
                break;
            }

            if (proxiesVec.empty()) {
                LError("Proxy pool is empty. quit");
                break;
            }

            LInfo("ProxyPool Size: {}", proxiesVec.size());

			for (auto& proxyData : proxiesVec) {
				proxies.emplace(proxyData, 0);
			}
		}

		/* 5s 同步一次数据库 */
        int ms_count = g_clientConfig.interval_proxypool_sync * 10;
		for (int i = 0; i < ms_count && !g_forceStop; ++i) {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	}

    LogF("ProxyManager stopped");

    g_stop(StopMode::Force);
	proxySvr->bg_thread_quit_ = true;
}

