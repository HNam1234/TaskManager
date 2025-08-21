#include <future>
#include <iostream>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/posix_time_io.hpp>
#include "logger.hpp"
#include "sysinfo_utils.hpp"

using namespace std;

int main()
{
    taskManager::LoggingConfig LogConfig;
    LogConfig.filePath = "tree.log";
    LogConfig.maxFileSize = 5;
    LogConfig.pattern = "%Y-%m-%d %H:%M:%S.%e [%^%l%$] %v";
    LogConfig.maxFiles = 3;
    LogConfig.level = "debug";
    std::vector<taskManager::SinkType> sinkTypes = {taskManager::SinkType::Console, taskManager::SinkType::File};
    LogConfig.sinks = sinkTypes;

    taskManager::Logger::instance().Init(LogConfig);

    boost::asio::io_context io_ctx;
    constexpr int cpuMonitorIntervalMs = 500;
    SysInfoUtils::startAsyncCpuMonitor(io_ctx, cpuMonitorIntervalMs);
    std::thread proof_thread([&io_ctx]() {
            io_ctx.run();
        });
    
    while(1)
    {
        boost::posix_time::ptime now = boost::posix_time::second_clock::local_time();
        const std::string str_time = to_simple_string(now);
        TM_LOG_INFO("NEW SYSTEM INFO AT {}", str_time);
        TM_LOG_INFO("CPU Usage: {}%", SysInfoUtils::getCpuUsage());
        TM_LOG_INFO("Uptime: {} seconds", SysInfoUtils::getUptime());
        auto memStatus = SysInfoUtils::getMemoryStatus();
        if (memStatus.valid) {
            TM_LOG_INFO("Memory Used: {:.2f} GB, Total: {:.2f} GB",
                        memStatus.used, memStatus.total);
        } else {
            TM_LOG_ERROR("Failed to get memory status");
        }
        auto diskStatus = SysInfoUtils::getDiskStatus("/");
        if (diskStatus.valid) {
            TM_LOG_INFO("Disk Used: {:.2f} GB, Free: {:.2f} GB",
                        diskStatus.used, diskStatus.free);
        } else {
            TM_LOG_ERROR("Failed to get disk status");
        }
        double temperature = SysInfoUtils::getTemperature();
        if (temperature > 0.0) {
            TM_LOG_INFO("CPU Temperature: {:.1f} °C", temperature);
        } else {
            TM_LOG_ERROR("Failed to get CPU temperature");
        }
        constexpr int time_sleep_ms = 500;
        std::this_thread::sleep_for(std::chrono::milliseconds(time_sleep_ms));
    }
    proof_thread.join();
    SysInfoUtils::stopAsyncCpuMonitor();    
    return 0;
}
