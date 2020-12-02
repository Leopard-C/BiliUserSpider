#pragma once
#include <string>

namespace util {

/* 获取可执行文件路径 */
std::string getExePath();

/* 获取可执行文件目录 */
std::string getExeDir();

/* 获取项目目录 */
std::string getProjDir();

/* 创建多级目录 */
bool createDir(std::string path);

}
