#pragma once
#include <stdint.h>

#include <boost/asio.hpp>
#include <iostream>

#include "logger.hpp"
struct MemoryStatus {
    double used;
    double total;
    bool   valid;
};

struct DiskStatus {
    double used;
    double free;
    bool   valid;
};

enum CpuFields { USER = 0, NICE, SYSTEM, IDLE, IOWAIT, IRQ, SOFTIRQ, STEAL, GUEST, GUEST_NICE, CPU_FIELDS_COUNT };

class SysInfoUtils {
   public:
    static uint64_t     getUptime();
    static double       getCpuUsage();
    static MemoryStatus getMemoryStatus();
    static DiskStatus   getDiskStatus(const std::string& mount_path);
    static double       getTemperature();

    // Start asynchronous CPU usage monitoring every `sample_ms` milliseconds.
    // run this before getCpuUsage()
    static void startAsyncCpuMonitor(boost::asio::io_context& io, int sample_ms = 200);
    static void stopAsyncCpuMonitor();

   private:
    static void cpuMonitorLoop(std::shared_ptr<boost::asio::steady_timer> timer, int interval_ms);
    static std::array<unsigned long, CpuFields::CPU_FIELDS_COUNT> readCpuStat();
    static double calcCpuUsage(const std::array<unsigned long, CpuFields::CPU_FIELDS_COUNT>& cpu1,
                               const std::array<unsigned long, CpuFields::CPU_FIELDS_COUNT>& cpu2);

    static std::atomic<double>                                                          cpu_usage_cached;
    static std::shared_ptr<boost::asio::steady_timer>                                   cpu_monitor_timer_;
    static std::shared_ptr<boost::asio::strand<boost::asio::io_context::executor_type>> cpu_monitor_strand_;
    static std::mutex                                                                   cpu_monitor_mutex_;
};
