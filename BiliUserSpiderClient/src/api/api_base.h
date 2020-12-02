/*******************************************************
** description:  发起GET请求
**
** author: Leopard-C
**
** update: 2020/12/02
*******************************************************/
#pragma once
#include <string>
#include "../iclient/ic/ic.h"
#include "../json/json.h"
#include "../status/status_code.h"

int http_get(ic::Url url, Json::Value* root, std::string* errMsg);

