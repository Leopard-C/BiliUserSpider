#pragma once
#include <string>

enum CM_STATUS {
    NO_ERROR = 0,
    GENERAL_ERROR = 1,
    SERVER_STOPPED = 2,
    CLIENT_DISCONNECT = 3,

    MISSING_PARAM = 4,
    INVALID_PARAM = 5,
    WRONG_PASSWORD = 6,

    NO_TASK_AVAIL = 7,
};

std::string get_err_msg(int code);

