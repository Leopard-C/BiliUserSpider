#include "user.h"
#include "helper.h"
#include "../log/logger.h"
#include "../json/json.h"
#include "../iclient/ic/ic.h"
#include "../manager/proxy_manager.h"
#include "../manager/user_agent_manager.h"
#include <string>

namespace bili {

/*
 *  1. 调用Web端的接口，获取用户信息
**/
int getUserInfo_Web(int mid, int timeout_ms, ProxyManager* proxyMgr, UserAgentManager* uaMgr, UserInfo* info) {
    ic::Url url("http://api.bilibili.com/x/space/acc/info");
    url.setParam("mid", std::to_string(mid));
    //LTrace("Get Cover of Space(APP): {}", url.toString());

    ic::ProxyData proxyData;
    BILI_RET ret = RET_CRITICAL;

    do {
        if (!proxyMgr->getRandomProxy(proxyData)) {
            ret = RET_CRITICAL;
            break;
        }

        ic::Request request(url.toString());
        request.setVerifySsl(false);
        request.setHeader("referer", "https://space.bilibili.com/" + std::to_string(mid - 1));
        request.setHeader("origin", "https://space.bilibili.com");
        request.setHeader("pragma", "no-cache");
        request.setProxy(proxyData);
        request.setTimeout(timeout_ms);
        uaMgr->setRandomUA(request);
        auto response = request.perform();

        if (response.getStatus() != ic::Status::SUCCESS ||
            response.getHttpStatus() != ic::http::StatusCode::HTTP_200_OK) {
            if (response.getData().size() < 10) {
                ret = RET_ERROR;
                break;
            }
            else {
                //LInfo("{}", response.Data());
                //ret = ERROR;
                //break;
            }
        }


        /* 解析b站API返回的JSON数据 */
        Json::Reader reader;
        Json::Value  bApiRes;
        if (!reader.parse(response.getData(), bApiRes, false)) {
            ret = RET_ERROR;
            break;
        }

        if (checkNull(bApiRes, "code", "message")) {
            LError("Missing key: code/message");
            ret = RET_CRITICAL;
            break;
        }

        int code = getIntVal(bApiRes, "code");
        if (code != 0) {
            /* 触发风控，此代理IP作废 */
            if (code == -412) {
                LError("Bili api return: {}", code);
                ret = RET_RISK_CONTROL;
                break;
            }
            /* 用户信息为空，账号已注销 */
            else if (code == -404) {
                if (mid % 3 == 0) {
                    LogF("-404");
                }
                ret = RET_NOT_FOUND;
                break;
            }
            /* 只有mid=0时会返回该错误代码, 这里视为用户不存在 */
            else if (code == -400) {
                LogF("-400");
                ret = RET_NOT_FOUND;
                break;
            }
            else {
                LError("bili api return: {}, {}", code, getStrVal(bApiRes, "message"));
                ret = RET_ERROR;
                break;
            }
        }

        // won't happen
        if (checkNull(bApiRes, "data")) {
            LError("Missing key: data");
            ret = RET_CRITICAL;
            break;
        }

        Json::Value& bApiResData = bApiRes["data"];
        if (checkNull(bApiResData, "mid", "name", "sex", "face", "sign", "level", "official", "vip")) {
            LError("Missing key: mid/name/sex/face/...");
            ret = RET_CRITICAL;
            break;
        }

        Json::Value& official = bApiResData["official"];
        if (checkNull(official, "type", "desc", "role", "title")) {
            LError("Missing key:type/desc/role/title");
            ret = RET_CRITICAL;
            break;
        }

        Json::Value& vip = bApiResData["vip"];
        if (checkNull(vip, "type", "status", "label")) {
            LError("Missing key:type/status/label");
            ret = RET_CRITICAL;
            break;
        }
        Json::Value& vip_label = vip["label"];
        if (checkNull(vip_label, "text")) {
            LError("Missing key:text");
            ret = RET_CRITICAL;
            break;
        }

        /* 直播间信息 */
        if (!checkNull(bApiResData, "live_room")) {
            Json::Value& live_room = bApiResData["live_room"];
            if (checkNull(live_room, "roomid")) {
                LError("Missing key:roomid");
                ret = RET_CRITICAL;
                break;
            }
            info->lid = getUIntVal(live_room, "roomid");
        }

        /* 可能和我的英文系统有关吧 */
        std::string sex = getStrVal(bApiResData, "sex");
        if (sex == "保密") {
            info->sex = 0;
        }
        else if (sex == "男") {
            info->sex = 2;
        }
        else if (sex == "女") {
            info->sex = 1;
        }
        else {
            info->sex = -1;
        }

        info->mid = mid;
        info->name = getStrVal(bApiResData, "name");
        info->attention = -1;  // <>
        info->fans = -1; // <>
        info->face = getStrVal(bApiResData, "face");
        info->sign = getStrVal(bApiResData, "sign");
        info->is_deleted = -1;  // <>
        info->level = getIntVal(bApiResData, "level");
        info->official_type = getIntVal(official, "type");
        info->official_desc = getStrVal(official, "desc");
        info->official_role = getIntVal(official, "role");
        info->official_title = getStrVal(official, "title");
        info->vip_type = getIntVal(vip, "type");
        info->vip_status = getIntVal(vip, "status");
        info->vip_label = getStrVal(vip_label, "text");

        ret = RET_NO_ERORR;
    } while (false);

    switch (ret) {
    case bili::RET_NO_ERORR:
        return 0;
    case bili::RET_NOT_FOUND:
        return 1;
    case bili::RET_RISK_CONTROL:
        proxyMgr->removeProxy(proxyData);
        return -1;
    case bili::RET_ERROR:
        proxyMgr->errorProxy(proxyData);
    case bili::RET_CRITICAL:
    default:
        return -1;
    }
}


/*
 *  2. 调用APP端的接口，获取用户信息
**/
int getUserInfo_App(int mid, int timeout_ms, ProxyManager* proxyMgr, UserAgentManager* uaMgr, UserInfo* info) {
    ic::Url url("https://app.bilibili.com/x/v2/space");
    url.setParam("vmid", std::to_string(mid));
    url.setParam("ts", std::to_string(time(NULL))); // time point
    std::string sign = bili::getSign(url);
    url.setParam("sign", sign);        // sign for auth
    //LTrace("Get Cover of Space(APP): {}", url.toString());

    ic::ProxyData proxyData;
    BILI_RET ret = RET_CRITICAL;

    do {
        if (!proxyMgr->getRandomProxy(proxyData)) {
            ret = RET_CRITICAL;
            break;
        }

        ic::Request request(url.toString());
        request.setVerifySsl(false);
        request.setHeader("env", "prod");
        request.setHeader("APP-KEY", "android");
        request.setHeader("Host", "app.bilibili.com");
        //uaMgr->setRandomUA(request);
        request.setHeader("User-Agent", "Mozilla/5.0 BiliDroid/6.9.1 (bbcallen@gmail.com) os/android model/"
            "MI 6  mobi_app/android build/6091000 channel/yingyongbao innerVer/6091000 osVer/5.1.1 network/2");
        request.setProxy(proxyData);
        request.setTimeout(timeout_ms);
        auto response = request.perform();

        if (response.getStatus() != ic::Status::SUCCESS ||
            response.getHttpStatus() != ic::http::StatusCode::HTTP_200_OK)
        {
            if (response.getData().size() < 10) {
                ret = RET_ERROR;
                break;
            }
            else {
                //LInfo("{}", response.Data());
                //ret = ERROR;
                //break;
            }
        }


        /* 解析b站API返回的JSON数据 */
        Json::Reader reader;
        Json::Value  bApiRes;
        if (!reader.parse(response.getData(), bApiRes, false)) {
            ret = RET_ERROR;
            break;
        }

        if (checkNull(bApiRes, "code", "message")) {
            LError("Missing key: code/message");
            ret = RET_CRITICAL;
            break;
        }

        int code = getIntVal(bApiRes, "code");
        if (code != 0) {
            /* 触发风控，此代理IP作废 */
            if (code == -412) {
                LError("Bili api return: {}", code);
                ret = RET_RISK_CONTROL;
                break;
            }
            /* 用户信息为空，账号已注销 */
            else if (code == -404) {
                if (mid % 3 == 0) {
                    LogF("-404");
                }
                ret = RET_NOT_FOUND;
                break;
            }
            /* 只有mid=0时会返回该错误代码, 这里视为用户不存在 */
            else if (code == -400) {
                LogF("-400");
                ret = RET_NOT_FOUND;
                break;
            }
            else {
                LError("bili api return: {}, {}", code, getStrVal(bApiRes, "message"));
                ret = RET_ERROR;
                break;
            }
        }

        // won't happen
        if (checkNull(bApiRes, "data")) {
            LError("Missing key: data");
            ret = RET_CRITICAL;
            break;
        }

        Json::Value& bApiResData = bApiRes["data"];
        if (checkNull(bApiResData, "card")) {
            LError("Missing key: card");
            ret = RET_CRITICAL;
            break;
        }

        Json::Value& card = bApiResData["card"];
        if (checkNull(card, "mid", "name", "sex", "face", "sign", "attention", "fans",
            "level_info", "official_verify", "vip", "is_deleted")) {
            LError("Missing key: mid/name/sex/face/...");
            ret = RET_CRITICAL;
            break;
        }

        Json::Value& level_info = card["level_info"];
        if (checkNull(level_info, "current_level")) {
            LError("Missing key:current_level");
            ret = RET_CRITICAL;
            break;
        }

        Json::Value& official_verify = card["official_verify"];
        if (checkNull(official_verify, "type", "desc", "role", "title")) {
            LError("Missing key:type/desc/role/title");
            ret = RET_CRITICAL;
            break;
        }

        Json::Value& vip = card["vip"];
        if (checkNull(vip, "vipType", "vipStatus", "label")) {
            LError("Missing key:vipType/vipStatus/label");
            ret = RET_CRITICAL;
            break;
        }
        Json::Value& vip_label = vip["label"];
        if (checkNull(vip_label, "text")) {
            LError("Missing key:text");
            ret = RET_CRITICAL;
            break;
        }

        /* 直播间信息 */
        info->lid = -1;
        if (!checkNull(bApiResData, "live")) {
            Json::Value& live = bApiResData["live"];
            if (checkNull(live, "roomid")) {
                LError("Missing key:roomid");
                ret = RET_CRITICAL;
                break;
            }
            info->lid = getIntVal(live, "roomid");
        }

        std::string sex = getStrVal(card, "sex");
        if (sex == "保密") {
            info->sex = 0;
        }
        else if (sex == "男") {
            info->sex = 2;
        }
        else if (sex == "女") {
            info->sex = 1;
        }
        else {
            info->sex = -1;
        }

        info->mid = mid;
        info->name = getStrVal(card, "name");
        info->attention = getIntVal(card, "attention");
        info->fans = getIntVal(card, "fans");
        info->face = getStrVal(card, "face");
        info->sign = getStrVal(card, "sign");
        info->is_deleted = getIntVal(card, "is_deleted");
        info->level = getIntVal(level_info, "current_level");
        info->official_type = getIntVal(official_verify, "type");
        info->official_desc = getStrVal(official_verify, "desc");
        info->official_role = getIntVal(official_verify, "role");
        info->official_title = getStrVal(official_verify, "title");
        info->vip_type = getIntVal(vip, "vipType");
        info->vip_status = getIntVal(vip, "vipStatus");
        info->vip_label = getStrVal(vip_label, "text");

        ret = RET_NO_ERORR;
    } while (false);

    switch (ret) {
    case bili::RET_NO_ERORR:
        return 0;
    case bili::RET_NOT_FOUND:
        return 1;
    case bili::RET_RISK_CONTROL:
        proxyMgr->removeProxy(proxyData);
        return -1;
    case bili::RET_ERROR:
        proxyMgr->errorProxy(proxyData);
    case bili::RET_CRITICAL:
    default:
        return -1;
    }
}

} // namespace bili


