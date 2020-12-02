#include "path.h"
#include "string_util.h"
#include <limits.h>
#include <cstring>

#ifdef _WIN32
#  include <windows.h>
#  include <io.h>
#  include <direct.h>
#else
#  include <sys/io.h>
#  include <unistd.h>
#  include <sys/stat.h>
#endif

#ifndef PATH_MAX
#  define PATH_MAX 256
#endif

namespace util {

/* 获取可执行文件路径 */
std::string getExePath() {
    char exePathTmp[PATH_MAX] = { 0 };
#ifdef _WIN32
    GetModuleFileNameA(NULL, exePathTmp, PATH_MAX - 1);
#else
    int n = readlink("/proc/self/exe", exePathTmp, PATH_MAX - 1);
#endif
    std::string exePath(exePathTmp);
    return formatDir(exePath);
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

/* 获取项目目录 */
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
    if (path.empty()) {
        return false;
    }
    formatDir(path);
    if (path.back() != '/') {
        path += "/";
    }
    auto pos = path.find("/", 1);
    while (pos != std::string::npos) {
        std::string str = path.substr(0, pos);
#ifdef _WIN32
        if (_access(str.c_str(), 0) != 0) {
            if (_mkdir(str.c_str()) != 0) {
                return false;
            }
        }
#else
        if (access(str.c_str(), R_OK) != 0) {
            if (mkdir(str.c_str(), 0775) != 0) {
                return false;
            }
        }
#endif
        pos = path.find("/", pos + 1);
    }
    return true;
}

/* 格式化目录（Windows平台） */
std::string& formatDir(std::string& path) {
#ifdef _WIN32
    util::replaceAll(path, "\\", "/");
#endif
    return path;
}

}

