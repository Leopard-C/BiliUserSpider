#pragma once
#include "../util/string_utils.h"
#include "../server/sim_parser.h"
#include "../status/status_code.h"

extern bool g_isRunning;

#define RETURN() \
    res.set_body(root);\
    return

#define RETURN_CODE_MSG(_code,_msg) \
    root["code"]=_code;\
    root["msg"]=_msg;\
    RETURN()

#define RETURN_CODE(_code) \
    RETURN_CODE_MSG(_code,get_err_msg(_code))

#define RETURN_OK() \
    if (!data.isNull()){\
        root["data"]=data;\
    }\
    RETURN_CODE_MSG(NO_ERROR, "OK")

#define RETURN_ERROR(_msg) \
    RETURN_CODE_MSG(GENERAL_ERROR,_msg)

#define RETURN_MISSING_PARAM(_param) \
    RETURN_CODE_MSG(MISSING_PARAM, "Missing param:"#_param)
#define RETURN_MISSING_PARAM_MSG(_msg) \
    RETURN_CODE_MSG(MISSING_PARAM, _msg)

#define RETURN_INVALID_PARAM(_param) \
    RETURN_CODE_MSG(INVALID_PARAM, "Invalid param:"#_param)
#define RETURN_INVALID_PARAM_MSG(_msg) \
    RETURN_CODE_MSG(INVALID_PARAM, _msg)

#define RETURN_SERVER_STOPPED() \
    RETURN_CODE(SERVER_STOPPED)

#define RETURN_WRONG_PASSWORD() \
    RETURN_CODE(WRONG_PASSWORD)

#define RETURN_CLINET_DISCONNECT() \
    RETURN_CODE(CLIENT_DISCONNECT)

/******************************************************
 *
 *     Check Server Status
 *
******************************************************/

#define CHECK_STATUS() \
    if (!g_isRunning) { RETURN_SERVER_STOPPED(); }


/******************************************************
 *
 *     Check param  (Must be exist)
 *
******************************************************/

/* 管理员 */
#define CHECK_PASSWORD() \
    if (!checkPassword(req,res)) return

/* 爬虫客户端 */
#define CHECK_TOKEN() \
    if (!checkToken(req,res)) return
#define CHECK_TOKEN_OR_PASSWORD() \
    if (!checkToken(req,res) || !checkPassword(req,res)) return

#define CHECK_PARAM(_param_) \
    std::string _param_=req.get_param(#_param_);\
    if(_param_.empty()){\
        RETURN_MISSING_PARAM(_param_);\
    }

#define CHECK_PARAM_TYPE(_param_, type) \
    std::string _param_##_=req.get_param(#_param_);\
    if(_param_##_.empty()) {\
        RETURN_MISSING_PARAM(_param_);\
    }\
    type _param_;\
    if (!util::convert(_param_##_, _param_)){\
        RETURN_INVALID_PARAM(_param_);\
    }

#define CHECK_PARAM_STR(_param_) \
    std::string _param_=req.get_param(#_param_);\
    if(_param_.empty()){\
        RETURN_MISSING_PARAM(_param_);\
    }
#define CHECK_PARAM_INT(_param_)     CHECK_PARAM_TYPE(_param_, int)
#define CHECK_PARAM_UINT(_param_)    CHECK_PARAM_TYPE(_param_, unsigned int)
#define CHECK_PARAM_LONG(_param_)    CHECK_PARAM_TYPE(_param_, long)
#define CHECK_PARAM_INT64(_param_)   CHECK_PARAM_TYPE(_param_, int64_t)
#define CHECK_PARAM_DOUBLE(_param_)  CHECK_PARAM_TYPE(_param_, double)


#define CHECK_PARAM_ARRAY_TYPE(_param_, type) \
    std::string _param_##_=req.get_param(#_param_);\
    if(_param_##_.empty()) {\
        RETURN_MISSING_PARAM(_param_);\
    }\
    std::vector<type> _param_;\
    if (!util::split(_param_##_, _param_)){\
        RETURN_INVALID_PARAM(_param_);\
    }

#define CHECK_PARAM_INT_ARRAY(_param_)     CHECK_PARAM_ARRAY_TYPE(_param_,int)
#define CHECK_PARAM_LONG_ARRAY(_param_)    CHECK_PARAM_ARRAY_TYPE(_param_,long)
#define CHECK_PARAM_INT64_ARRAY(_param_)   CHECK_PARAM_ARRAY_TYPE(_param_,int64_t)
#define CHECK_PARAM_DOUBLE_ARRAY(_param_)  CHECK_PARAM_ARRAY_TYPE(_param_,double)
#define CHECK_PARAM_STR_ARRAY(_param_)     CHECK_PARAM_ARRAY_TYPE(_param_,std::string)


/************************************************************
 *
 *     Get param  (if not exist, return default value)
 *
************************************************************/

#define GET_PARAM(_param_) \
    std::string _param_=req.get_param(#_param_)

#define GET_PARAM_TYPE(_param_, type, _default) \
    std::string _param_##_=req.get_param(#_param_);\
    type _param_;\
    if(_param_##_.empty()) {_param_=_default;}\
    else if (!util::convert(_param_##_, _param_)){RETURN_INVALID_PARAM(_param_);}

#define GET_PARAM_STR(_param_) \
    std::string _param_=req.get_param(#_param_);
#define GET_PARAM_INT(_param_,_default)     GET_PARAM_TYPE(_param_,int, _default)
#define GET_PARAM_INT64(_param_,_default)   GET_PARAM_TYPE(_param_,int64_t,_default)
#define GET_PARAM_DOUBLE(_param_,_default)  GET_PARAM_TYPE(_param_,double,_default)

#define GET_PARAM_ARRAY_TYPE(_param_, type) \
    std::string _param_##_=req.get_param(#_param_);\
    std::vector<type> _param_;\
    if(!_param_##_.empty()&&!util::split(_param_##_, _param_))\
    {RETURN_INVALID_PARAM(_param_);}\

#define GET_PARAM_INT_ARRAY(_param_)     GET_PARAM_ARRAY_TYPE(_param_,int)
#define GET_PARAM_INT64_ARRAY(_param_)   GET_PARAM_ARRAY_TYPE(_param_,int64_t)
#define GET_PARAM_DOUBLE_ARRAY(_param_)  GET_PARAM_ARRAY_TYPE(_param_,double)
#define GET_PARAM_STR_ARRAY(_param_)     GET_PARAM_ARRAY_TYPE(_param_,std::string)


bool checkPassword(Request& req, Response& res);

bool checkToken(Request& req, Response& res);

