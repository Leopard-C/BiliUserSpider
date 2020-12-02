#include "logger.h"
#include "spdlog/sinks/stdout_sinks.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "qtextedit_sink.h"
#include <ctime>

extern std::string g_logsDir;

Logger::Logger() {
    time_t now = time(NULL);
    char tmp[64];
    strftime(tmp, sizeof(tmp), "%Y%m%d_%H%M%S.spdlog", localtime(&now));
    std::string logFileName = g_logsDir + "/" + std::string(tmp);
    std::vector<spdlog::sink_ptr> sinks;
    /* 输出到控制台 */
    sinks.push_back(std::make_shared<spdlog::sinks::stdout_sink_st>());
    /* 输出到QTextEdit控件 */
    sinks.push_back(std::make_shared<spdlog::sinks::qtextedit_sink_mt>(&textedit_target));
    /* 输出到文件 */
    sinks.push_back(std::make_shared<spdlog::sinks::rotating_file_sink_mt>(logFileName, 1025 * 1024 * 5, 10));
    logger = std::make_shared<spdlog::logger>("log", begin(sinks), end(sinks));
    spdlog::register_logger(logger);
    logger->set_pattern("[%H:%M:%S] [%l] %v");

#ifdef _LOG_DETAIL
    logger->set_level(spdlog::level::trace);
    logger->flush_on(spdlog::level::debug);
#else
    logger->set_level(spdlog::level::info);
    logger->flush_on(spdlog::level::err);
#endif

    spdlog::flush_every(std::chrono::seconds(1));
}

