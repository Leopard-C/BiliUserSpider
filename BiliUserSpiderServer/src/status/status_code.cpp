#include "status_code.h"

std::string get_err_msg(int code) {
    switch (code) {
    case NO_ERROR:           return "OK";
    case SERVER_STOPPED:     return "Server stopped";
    case CLIENT_DISCONNECT:  return "Client was disconnected";
    case MISSING_PARAM:      return "Missing param";
    case INVALID_PARAM:      return "Invalid param";
    case WRONG_PASSWORD:     return "Wrong password";
    case NO_TASK_AVAIL:      return "No task to assign";
    case GENERAL_ERROR:
    default:                 return "Error";
    }
}

