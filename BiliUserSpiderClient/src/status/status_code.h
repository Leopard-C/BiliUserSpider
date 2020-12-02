#pragma once

enum CM_STATUS {
#ifndef NO_ERROR
    NO_ERROR = 0,
#endif
    GENERAL_ERROR = 1,
    SERVER_STOPPED = 2,
    CLIENT_DISCONNECT = 3,

    MISSING_PARAM = 4,
    INVALID_PARAM = 5,
    WRONG_PASSWORD = 6,

    NO_TASK_AVAIL = 7,

    /* custom */
    REQUEST_ERROR = 100,
    INTERNAL_SERVER_ERROR = 101,
};

