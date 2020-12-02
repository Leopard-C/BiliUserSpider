/**********************************************************************
** description: 从数据库中获取代理IP，或者修改数据库中的代理IP
**
** author: Leopard-C
**
** update: 2020/12/02
***********************************************************************/
#pragma once
#include "../iclient/ic/ic.h"
#include "../database/mysql_instance.h"

int api_get_random_proxy(MysqlInstance& mysql, ic::ProxyData* proxyData, std::string* errMsg);

int api_get_all_proxies(MysqlInstance& mysql, int maxErrorCount, std::vector<ic::ProxyData>* proxies, std::string* errMsg);

int api_error_proxy(MysqlInstance& mysql, const ic::ProxyData& proxyData, int count, std::string* errMsg);

int api_remove_proxy(MysqlInstance& mysql, const ic::ProxyData& proxyData, std::string* errMsg);

int api_get_num_proxies(MysqlInstance& mysql, int* num, std::string* errMsg);

int api_empty_proxy_pool(MysqlInstance& mysql, std::string* errMsg);

