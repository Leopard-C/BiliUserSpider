#include "string_utils.h"
#include <unistd.h>
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

std::string& trim(std::string& input) {
    if (input.empty()) {
        return input;
    }
    input.erase(input.find_last_not_of(" ") + 1);
    input.erase(0, input.find_first_not_of(" "));
    return input;
}

}

