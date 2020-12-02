#pragma once
#include <sstream>
#include <vector>
#include <string>

namespace util {
    
template<typename Output, typename Input>
bool convert(const Input& in, Output& out) {
    std::stringstream ss;
    ss << in;
    ss >> out;
    return !ss.fail();
}

template<typename T>
bool split(const std::string& str, const std::string& sep, std::vector<T>& vec) {
    if (std::is_same<T, std::string>::value) {
        if (str.empty()) {
            vec.push_back(str);
            return true;
        }
        std::string::size_type pos1 = 0;
        std::string::size_type pos2 = str.find(sep, pos1);
        std::string::size_type sep_len = sep.length();
        while (pos2 != str.npos) {
            std::string ele = str.substr(pos1, pos2 - pos1);
            vec.push_back(ele);
            pos1 = pos2 + sep_len;
            pos2 = str.find(sep, pos1);
        }
        std::string remain = str.substr(pos1);
        vec.push_back(remain);
    }
    else {
        if (str.empty()) {
            return false;
        }
        std::string::size_type pos1 = 0;
        std::string::size_type pos2 = str.find(sep, pos1);
        std::string::size_type sep_len = sep.length();
        while (pos2 != str.npos) {
            if (pos2 == pos1) {
                return false;
            }
            std::string ele = str.substr(pos1, pos2 - pos1);
            T t;
            if (!convert(ele, t)) {
                return false;
            }
            vec.push_back(t);
            pos1 = pos2 + sep_len;
            pos2 = str.find(sep, pos1);
        }
        std::string remain = str.substr(pos1);
        if (remain.empty()) {
            return false;
        }
        else {
            T t;
            if (!convert(remain, t)) {
                return false;
            }
            vec.push_back(t);
        }
    }
    return true;
}

std::string toupper(const std::string& str);
std::string tolower(const std::string& str);

std::string& trim(std::string& input);

bool isdigit(const char ch);
bool isdigit(const std::string& str);

bool isalnum(const char ch);
bool isalnum(const std::string& str);

inline std::string to_string(const char* str, const std::string& defaultValue = "") {
    return str ? std::string(str) : defaultValue;
}

inline int to_int(const char* str, const int defaultValue) {
    return str ? std::atoi(str) : defaultValue;
}

std::string& replaceAll(std::string& str, const std::string& oriStr, const std::string& repStr);

} // namespace util
