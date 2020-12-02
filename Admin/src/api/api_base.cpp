#include "api_base.h"
#include "../global.h"
#include "../log/logger.h"


int http_get(ic::Url url, Json::Value* root, std::string* errMsg) {
    int ret = GENERAL_ERROR;
    do {
        url.setParam("pwd", g_svrPwd);
        ic::Request request(url.toString());
        request.setTimeout(2000);
        auto response = request.perform();
        if (response.getStatus() != ic::Status::SUCCESS ||
            response.getHttpStatus() != ic::http::StatusCode::HTTP_200_OK)
        {
            ret = REQUEST_ERROR;
            *errMsg = "HTTP GET failed";
            break;
        }

        Json::Reader reader;
        if (!reader.parse(response.getData(), *root, false)) {
            ret = INTERNAL_SERVER_ERROR;
            *errMsg = "Parse data returned by server failed";
            break;
        }
        if (checkNull((*root), "code", "msg")) {
            ret = INTERNAL_SERVER_ERROR;
            *errMsg = "Missing key:code/msg";
            break;
        }

        int code = getIntVal((*root), "code");
        if (code != 0) {
            ret = code;
            *errMsg = getStrVal((*root), "msg");
            break;
        }

        ret = NO_ERROR;
    } while (false);

    return ret;
}

int http_post(ic::Url url, Json::Value& jsonData, Json::Value* root, std::string* errMsg) {
    int ret = GENERAL_ERROR;
    do {
        jsonData["pwd"] = g_svrPwd;
        ic::Request request(url.toString());
        request.setMethod(ic::http::Method::POST);
        request.setData(jsonData.toStyledString());
        request.setHeader("Content-Type", "application/json");
        request.setTimeout(2000);
        auto response = request.perform();
        if (response.getStatus() != ic::Status::SUCCESS ||
            response.getHttpStatus() != ic::http::StatusCode::HTTP_200_OK)
        {
            ret = REQUEST_ERROR;
            *errMsg = "HTTP GET failed";
            break;
        }

        Json::Reader reader;
        if (!reader.parse(response.getData(), *root, false)) {
            ret = INTERNAL_SERVER_ERROR;
            *errMsg = "Parse data returned by server failed";
            break;
        }
        if (checkNull((*root), "code", "msg")) {
            ret = INTERNAL_SERVER_ERROR;
            *errMsg = "Missing key:code/msg";
            break;
        }

        int code = getIntVal((*root), "code");
        if (code != 0) {
            ret = code;
            *errMsg = getStrVal((*root), "msg");
            break;
        }

        ret = NO_ERROR;
    } while (false);

    return ret;
}
