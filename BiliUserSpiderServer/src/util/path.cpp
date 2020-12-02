#include "path.h"
#include <unistd.h>
#include <limits.h>
#include <cstring>
#include <sys/stat.h>
#include <sys/io.h>

namespace util {

/* 获取可执行文件路径 */
std::string getExePath() {
    char exePath[PATH_MAX] = { 0 };
    int n = readlink("/proc/self/exe", exePath, PATH_MAX);
    return std::string(exePath);
}

/* 获取可执行文件目录 */
std::string getExeDir() {
    std::string exePath = getExePath();
    auto pos = exePath.find_last_of("/");
    if (pos == std::string::npos) {
        return std::string();
    }
    else {
        return exePath.substr(0, pos);
    }
}

/* 获取项目目录                  */
std::string getProjDir() {
    std::string exeDir = getExeDir();
    auto pos = exeDir.find_last_of("/");
    if (pos == std::string::npos) {
        return std::string();
    }
    else {
        return exeDir.substr(0, pos);
    }
}

/* 创建多级目录 */
bool createDir(std::string path) {
    if (path.back() != '/') {
        path += "/";
    }
    auto pos = path.find("/", 1);
    while (pos != std::string::npos) {
        std::string str = path.substr(0, pos);
        if (access(str.c_str(), R_OK) != 0) {
            if (mkdir(str.c_str(), 0775) != 0) {
                return false;
            }
        }
        pos = path.find("/", pos + 1);
    }
    return true;
}

} // namespace util

