#ifndef _LOGGER_HPP_
#define _LOGGER_HPP_

#include <spdlog/spdlog.h>
#include <memory>

namespace taskManager
{
enum class SinkType
{
    Console,
    File
};

struct LoggingConfig
{
    std::string level;
    std::vector<SinkType> sinks;
    std::string filePath;
    size_t maxFileSize;
    size_t maxFiles;
    std::string pattern;
};

class Logger
{
public:
    static Logger& instance();

    std::shared_ptr<spdlog::logger> get() const { return logger_; }

    void Init(const LoggingConfig& cfg);

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

private:
    Logger() = default;
    ~Logger() = default;

    std::shared_ptr<spdlog::logger> logger_;
    static spdlog::level::level_enum levelFromString(const std::string& s);
};
}

#define TM_LOG_LEVEL(method, ...) \
do { \
        auto _logger = taskManager::Logger::instance().get(); \
        if (_logger) _logger->method(__VA_ARGS__); \
        else std::cerr << "[TM_LOG] Logger not initialized!\n"; \
} while(0)

#define TM_LOG_TRACE(...)    TM_LOG_LEVEL(trace, __VA_ARGS__)
#define TM_LOG_DEBUG(...)    TM_LOG_LEVEL(debug, __VA_ARGS__)
#define TM_LOG_INFO(...)     TM_LOG_LEVEL(info, __VA_ARGS__)
#define TM_LOG_WARN(...)     TM_LOG_LEVEL(warn, __VA_ARGS__)
#define TM_LOG_ERROR(...)    TM_LOG_LEVEL(error, __VA_ARGS__)
#define TM_LOG_CRITICAL(...) TM_LOG_LEVEL(critical, __VA_ARGS__)

#endif // _LOGGER_HPP_
