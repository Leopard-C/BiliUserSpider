/********************************************************************************
**
**    功能: 封装spdlog (写入文件 + 控制台输出)
**    调用: LTrace, LDebug, LInfo, LWarn, LError, LCritical
**    目录: 由全局变量 g_logsDir 决定
**    文件: data_time.spdlog
**
**    其他: LogF() 封装的printf，自动添加 '\n' 换行
**          Log(x) ==> std::cout << (x) << std::endl
**
**  宏开关: _LOG_DETAIL  影响日志级别(以及输出的详细程度)      <===========
**
**    注意: 不要在全局变量的析构函数中调用 LInfo、LError等日志输出类，
**          否则程序可能崩溃。
**          应该用LogF、Log替代
**          因为不同文件中的全局(静态)变量的析构次序不能保证
**          如果Logger的实例已经析构，再调用LInfo等就会崩溃
**
**  author: Leopard-C
**
**  update: 2020/12/02
**
********************************************************************************/
#pragma once
#define SPDLOG_COMPILED_LIB 

#include "spdlog/spdlog.h"
#include "spdlog/logger.h"


class Logger {
public:
    Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    ~Logger() { spdlog::drop_all(); }
public:
    static Logger& GetInstance() {
        static Logger m_instance;
        return m_instance;
    }
    std::shared_ptr<spdlog::logger> GetLogger() {
        return logger;
    }

public:
private:
    std::shared_ptr<spdlog::logger> logger;
};

#ifdef _WIN32
#    define __FILENAME__ (strrchr(__FILE__, '\\') ? (strrchr(__FILE__, '\\') + 1):__FILE__)
#else
#    define __FILENAME__ (strrchr(__FILE__, '/') ? (strrchr(__FILE__, '/') + 1):__FILE__)
#endif

#ifndef suffixsth
#  define suffixsth(msg) std::string(msg).append(" <")\
     .append(__FILENAME__).append("> <").append(__func__)\
     .append("> <").append(std::to_string(__LINE__))\
     .append(">").c_str()
#endif // suffix


// output to file
#ifdef _LOG_DETAIL 
#  define LTrace(msg,...) \
     Logger::GetInstance().GetLogger()->trace(suffixsth(msg),  ##__VA_ARGS__)
#  define LDebug(msg,...) \
     Logger::GetInstance().GetLogger()->debug(suffixsth(msg),  ##__VA_ARGS__)
#  define LInfo(msg,...) \
     Logger::GetInstance().GetLogger()->info(suffixsth(msg),  ##__VA_ARGS__)
#  define LWarn(msg,...) \
     Logger::GetInstance().GetLogger()->warn(suffixsth(msg),  ##__VA_ARGS__)
#  define LError(msg,...) \
     Logger::GetInstance().GetLogger()->error(suffixsth(msg),  ##__VA_ARGS__)
#  define LCritical(msg,...) \
     Logger::GetInstance().GetLogger()->critical(suffixsth(msg),  ##__VA_ARGS__)
#else
#  define LTrace(...) Logger::GetInstance().GetLogger()->trace(__VA_ARGS__)
#  define LDebug(...) Logger::GetInstance().GetLogger()->debug(__VA_ARGS__)
#  define LInfo(...) Logger::GetInstance().GetLogger()->info(__VA_ARGS__)
#  define LWarn(...) Logger::GetInstance().GetLogger()->warn(__VA_ARGS__)
#  define LError(msg,...) \
     Logger::GetInstance().GetLogger()->error(suffixsth(msg),  ##__VA_ARGS__)
#  define LCritical(msg,...) \
     Logger::GetInstance().GetLogger()->critical(suffixsth(msg),  ##__VA_ARGS__)
//#  define LError(...) Logger::GetInstance().GetLogger()->error(__VA_ARGS__)
//#  define LCritical(...) Logger::GetInstance().GetLogger()->critical(__VA_ARGS__)
#endif


// output to console
#include <iostream>
#define Log(x) std::cout << x << std::endl
#define LogF(fmt, ...) printf(fmt"\n", ##__VA_ARGS__)
#define LogEx(fmt, ...) printf("%s(%d)-<%s>: "##fmt, __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)

