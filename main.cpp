#include <iostream>
#include "logger.h"

using namespace std;

int main()
{
    treeManagement::LoggingConfig LogConfig;
    LogConfig.filePath = "tree.log";
    LogConfig.maxFileSize = 5;
    LogConfig.pattern = "%Y-%m-%d %H:%M:%S.%e [%^%l%$] %v";
    LogConfig.maxFiles = 3;
    LogConfig.level = "debug";
    std::vector<treeManagement::SinkType> sinkTypes = {treeManagement::SinkType::Console, treeManagement::SinkType::File};
    LogConfig.sinks = sinkTypes;

    treeManagement::Logger::instance().Init(LogConfig);
    TM_LOG_ERROR("HELLO WORLD DEBUG TEST");

    return 0;
}
