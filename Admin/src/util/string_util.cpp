#include "string_util.h"
#include <chrono>
#include <cstring>
#include <cassert>

namespace util {

std::string toupper(const std::string& str) {
    std::string newStr = str;
    for (auto& c : newStr) {
        if (c >= 'a' && c <= 'z') {
            c -= 32;
        }
    }
    return newStr;
}

std::string tolower(const std::string& str) {
    std::string newStr = str;
    for (auto& c : newStr) {
        if (c >= 'A' && c <= 'Z') {
            c += 32;
        }
    }
    return newStr;
}


bool isdigit(const char ch) {
    return ch >= '0' && ch <= '9';
}

bool isdigit(const std::string& str) {
    for (auto c : str) {
        if (c < '0' || c > '9') {
            return false;
        }
    }
    return true;
}

bool isalnum(const char ch) {
    return (ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z');
}

bool isalnum(const std::string& str) {
    for (auto c : str) {
        if (!isalnum(c)) {
            return false;
        }
    }
    return true;
}

std::string& replaceAll(std::string& str, const std::string& oriStr, const std::string& repStr) {
    std::string::size_type oriStrLen = oriStr.length();
    std::string::size_type repStrLen = repStr.length();
    std::string::size_type pos = str.find(oriStr);
    while (pos != std::string::npos) {
        str.replace(pos, oriStrLen, repStr);
        pos = str.find(oriStr, pos + repStrLen);
    }
    return str;
}


/* 格式化时间显示，将时间间隔（秒为单位）转为 xxH xxMin xxSec */
std::string formatTimeSpan(time_t t) {
    time_t day = t / 86400;
    t %= 86400;
    time_t hour = t / 3600;
    t %= 3600;
    time_t min = t / 60;
    t %= 60;
    time_t sec = t;
    std::string time_str;
    time_str.reserve(32);
    /* 可以用sprintf实现，但是time_t在32位和64位不一样，还要区分一下 */
    /* 这里用std::to_string更好一些，虽然麻烦些,效率也可能低一些 */
    if (day > 0) {
        if (day < 10) {
            time_str += "0";
        }
        time_str += std::to_string(day) + "d ";
    }
    if (hour > 0) {
        if (hour < 10) {
            time_str += "0";
        }
        time_str += std::to_string(hour) + "h ";
    }
    if (min > 0) {
        if (min < 10) {
            time_str += "0";
        }
        time_str += std::to_string(min) + "m ";
    }
    if (sec < 10) {
        time_str += "0";
    }
    time_str += std::to_string(sec) + "s";
    return std::string(time_str);
}

std::string& trim(std::string& input) {
    if (input.empty()) {
        return input;
    }
    input.erase(input.find_last_not_of(" ") + 1);
    input.erase(0, input.find_first_not_of(" "));
    return input;
}

}

