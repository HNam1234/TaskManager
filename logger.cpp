#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>

#include "logger.h"

namespace treeManagement
{
Logger& Logger::instance()
{
    static Logger inst;
    return inst;
}

spdlog::level::level_enum Logger::levelFromString(const std::string& logLevel)
{
    std::string normalizedLogLevel = logLevel;
    std::transform(normalizedLogLevel.begin(), normalizedLogLevel.end(), normalizedLogLevel.begin(), ::tolower);
    if (normalizedLogLevel == "trace") return spdlog::level::trace;
    if (normalizedLogLevel == "debug") return spdlog::level::debug;
    if (normalizedLogLevel == "info") return spdlog::level::info;
    if (normalizedLogLevel == "warn" || normalizedLogLevel == "warning") return spdlog::level::warn;
    if (normalizedLogLevel == "error") return spdlog::level::err;
    if (normalizedLogLevel == "critical") return spdlog::level::critical;
    throw std::invalid_argument("Unknown log level: " + logLevel);
}

void Logger::Init(const LoggingConfig& cfg)
{
    std::vector<spdlog::sink_ptr> sinks;

    for (auto type : cfg.sinks) {
        switch (type) {
        case SinkType::Console:
        {
            auto sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            sink->set_pattern(cfg.pattern);
            sinks.push_back(sink);
            break;
        }
        case SinkType::File:
        {
            if (cfg.filePath.empty())
                throw std::invalid_argument("File sink requires non-empty filePath!");
            auto sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                cfg.filePath, cfg.maxFileSize, cfg.maxFiles
                );
            sink->set_pattern(cfg.pattern);
            sinks.push_back(sink);
            break;
        }
        }
    }
    if (sinks.empty())
    {
        throw std::runtime_error("No sinks specified for Logger");
    }

    logger_ = std::make_shared<spdlog::logger>("botanika-agent", sinks.begin(), sinks.end());
    spdlog::register_logger(logger_);
    logger_->set_level(levelFromString(cfg.level));
    logger_->flush_on(spdlog::level::info);
}

}
