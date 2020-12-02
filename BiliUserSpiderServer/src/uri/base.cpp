#include "base.h"
#include "../server/sim_parser.h"
#include "../json/json.h"
#include "../global.h"

bool checkPassword(Request& req, Response& res) {
    Json::Value root;
    GET_PARAM_STR(pwd);
    if (pwd.empty()) {
        RETURN_MISSING_PARAM(pwd) false;
    }
    else {
        if (pwd == g_svrConfig.admin_password) {
            return true;
        }
        else {
            RETURN_WRONG_PASSWORD() false;
        }
    }
}

bool checkToken(Request& req, Response& res) {
    Json::Value root;
    GET_PARAM_STR(token);
    if (token.empty()) {
        RETURN_MISSING_PARAM(token) false;
    }
    else {
        if (token == g_svrConfig.client_token) {
            return true;
        }
        else {
            RETURN_WRONG_PASSWORD() false;
        }
    }
}

