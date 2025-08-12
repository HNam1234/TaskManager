#include <iostream>
#include "logger.h"

using namespace std;

int main()
{
    treeManagement::LoggingConfig LogConfig;
    LogConfig.filePath = "tree.log";
    LogConfig.maxFileSize = 5;
    LogConfig.maxFiles = 3;
    LogConfig.level = "debug";
    std::vector<treeManagement::SinkType> sinkTypes = {treeManagement::SinkType::Console, treeManagement::SinkType::File};
    LogConfig.sinks = sinkTypes;

    treeManagement::Logger::instance().Init(LogConfig);
    TM_LOG_ERROR("HELLO WORLD DEBUG TEST");
    spdlog::set_level(spdlog::level::debug); // Ensure debug logs show
    spdlog::info("Info log test!");
    spdlog::debug("Debug log test!");

    return 0;
}
