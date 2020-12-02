#include "api_base.h"
#include "../global.h"


int http_get(ic::Url url, Json::Value* root, std::string* errMsg) {
    int ret = GENERAL_ERROR;
    do {
        url.setParam("token", g_clientToken);
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
