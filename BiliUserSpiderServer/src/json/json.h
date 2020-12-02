/*******************************************************
** namespace:   Json
**
** description: helper function for using Jsoncpp
**
** author: Leopard-C
**
** update: 2020/12/02
*******************************************************/
#pragma once
#include <jsoncpp/json/json.h>

#define getStrVal(parent, key) parent[key].asString()
#define getIntVal(parent, key) parent[key].asInt()
#define getUIntVal(parent, key) parent[key].asUInt()
#define getBoolVal(parent, key) parent[key].asBool()
#define getDoublelVal(parent, key) parent[key].asDouble()
#define getInt64Val(parent, key) parent[key].asInt64()

#define getStrValEx(parent, key)  std::string  key = parent[#key].asString()
#define getIntValEx(parent, key)  int key = parent[#key].asInt()
#define getUIntValEx(parent, key) unsigned int key = parent[#key].asUInt()
#define getDoublelValEx(parent, key) double key = parent[#key].asDouble()
#define getBoolValEx(parent, key) bool key = parent[#key].asBool();
#define getInt64ValEx(parent, key) int64_t key = parent[#key].asInt64();

namespace Json {

inline bool checkNull(Json::Value& parent) {
    return false;
}

template<typename... TArgs>
bool checkNull(Json::Value& parent, const std::string& key, const TArgs&... keys) {
    return parent[key].isNull() || checkNull(parent, keys...);
}

}

