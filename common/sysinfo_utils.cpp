#include "sysinfo_utils.hpp"

#include <sys/statvfs.h>
#include <unistd.h>

#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

constexpr const char* UPTIME_PATH       = "/proc/uptime";
constexpr const char* CPU_STAT_PATH     = "/proc/stat";
constexpr const char* MEM_INFO_PATH     = "/proc/meminfo";
constexpr const char* DISK_STATS_PATH   = "/proc/diskstats";
constexpr const char* THERMAL_ZONE_PATH = "/sys/class/thermal/thermal_zone";

uint64_t SysInfoUtils::getUptime() {
    std::ifstream stream(UPTIME_PATH);
    std::string   line;
    if (stream.is_open()) {
        // The line read from /proc/uptime has format:
        // "<uptime_seconds> <idle_seconds>"
        // refer to https://man7.org/linux/man-pages/man5/proc_uptime.5.html
        // so we use istringstream to get the first value
        std::getline(stream, line);
        std::istringstream iss(line);
        double             uptime = 0;
        iss >> uptime;
        return static_cast<uint64_t>(uptime);
    }
    TM_LOG_ERROR("Failed to read uptime from {}", UPTIME_PATH);
    return 0;
}

std::atomic<double> SysInfoUtils::cpu_usage_cached{0.0};

std::shared_ptr<boost::asio::steady_timer> SysInfoUtils::cpu_monitor_timer_ = nullptr;

std::shared_ptr<boost::asio::strand<boost::asio::io_context::executor_type>> SysInfoUtils::cpu_monitor_strand_ =
    nullptr;

std::mutex SysInfoUtils::cpu_monitor_mutex_;

void SysInfoUtils::cpuMonitorLoop(std::shared_ptr<boost::asio::steady_timer> timer, int interval_ms) {
    // Take first CPU snapshot
    auto cpuState1 = readCpuStat();

    // Set timer to run after interval_ms milliseconds
    timer->expires_after(std::chrono::milliseconds(interval_ms));
    timer->async_wait(boost::asio::bind_executor(
        *cpu_monitor_strand_, [timer, interval_ms, cpuState1](const boost::system::error_code& ec) mutable {
        if (ec) return;
        // Take second CPU snapshot
        auto cpuState2 = readCpuStat();
        // Calculate usage based on the two snapshots
        double usage = calcCpuUsage(cpuState1, cpuState2);
        //  Store cpu usage to cpu_usage_cached we can read on another threads safely
        cpu_usage_cached.store(usage, std::memory_order_relaxed);
        // Repeat loop: schedule next timer
        cpuMonitorLoop(timer, interval_ms);
        }));
}

void SysInfoUtils::startAsyncCpuMonitor(boost::asio::io_context& io, int interval_ms) {
    std::lock_guard<std::mutex> lock(cpu_monitor_mutex_);

    if (cpu_monitor_timer_) {
        cpu_monitor_timer_->cancel();
        cpu_monitor_timer_.reset();
    }
    // If the strand is not initialized or the io_context has changed, create a new strand
    if (!cpu_monitor_strand_ || &io != &(cpu_monitor_strand_->context())) {
        cpu_monitor_strand_ =
            std::make_shared<boost::asio::strand<boost::asio::io_context::executor_type>>(io.get_executor());
    }

    cpu_monitor_timer_ = std::make_shared<boost::asio::steady_timer>(io);
    cpuMonitorLoop(cpu_monitor_timer_, interval_ms);
}

void SysInfoUtils::stopAsyncCpuMonitor() {
    std::lock_guard<std::mutex> lock(cpu_monitor_mutex_);

    if (cpu_monitor_timer_) {
        cpu_monitor_timer_->cancel();
        cpu_monitor_timer_.reset();
        cpu_usage_cached.store(0.0, std::memory_order_relaxed);
    }
}

double SysInfoUtils::getCpuUsage() {
    if (!cpu_monitor_timer_) {
        TM_LOG_ERROR("getCpuUsage() called before startAsyncCpuMonitor() -- value is not valid!");
    }
    return std::round(cpu_usage_cached.load(std::memory_order_relaxed) * 10.0) / 10.0;
}

// The line read from /proc/stat has format:
// example: cpu  3357 0 4313 1362393 0 0 0 0 0 0
// Cpu Fields: user, nice, system, idle, iowait, irq, softirq, steal, guest, guest_nice
// For details, see: https://www.kernel.org/doc/html/latest/filesystems/proc.html#stat (section 1.7)

std::array<unsigned long, CpuFields::CPU_FIELDS_COUNT> SysInfoUtils::readCpuStat() {
    std::ifstream                                          cpuFile(CPU_STAT_PATH);
    std::array<unsigned long, CpuFields::CPU_FIELDS_COUNT> cpuFields{};

    if (!cpuFile.is_open()) {
        TM_LOG_WARN("Failed to open {}", CPU_STAT_PATH);
        return cpuFields;
    }

    std::string line;
    if (!std::getline(cpuFile, line)) {
        TM_LOG_WARN("Failed to read line from {}", CPU_STAT_PATH);
        return cpuFields;
    }

    std::istringstream iss(line);

    std::string cpuLabel;
    iss >> cpuLabel;  // skip the "cpu" label

    int fieldCount = 0;
    for (auto& cpuField : cpuFields) {
        if (!(iss >> cpuField)) break;
        ++fieldCount;
    }
    if (fieldCount < CPU_FIELDS_COUNT) {
        TM_LOG_WARN("Insufficient CPU fields read from /proc/stat: expected {}, got {}",
                     static_cast<int>(CPU_FIELDS_COUNT), fieldCount);
    }
    return cpuFields;
}

// Takes two CPU state snapshots and returns CPU usage percentage between them.
// The main reason we need to capture two CPU states at different times because CPU usage is about how much work was
// done over a period, not just a single moment
double SysInfoUtils::calcCpuUsage(const std::array<unsigned long, CpuFields::CPU_FIELDS_COUNT>& cpu1,
                                  const std::array<unsigned long, CpuFields::CPU_FIELDS_COUNT>& cpu2) {
    unsigned long idle1    = cpu1[CpuFields::IDLE] + cpu1[CpuFields::IOWAIT];
    unsigned long idle2    = cpu2[CpuFields::IDLE] + cpu2[CpuFields::IOWAIT];
    unsigned long nonIdle1 = cpu1[CpuFields::USER] + cpu1[CpuFields::NICE] + cpu1[CpuFields::SYSTEM] +
                             cpu1[CpuFields::IRQ] + cpu1[CpuFields::SOFTIRQ] + cpu1[CpuFields::STEAL];
    unsigned long nonIdle2 = cpu2[CpuFields::USER] + cpu2[CpuFields::NICE] + cpu2[CpuFields::SYSTEM] +
                             cpu2[CpuFields::IRQ] + cpu2[CpuFields::SOFTIRQ] + cpu2[CpuFields::STEAL];

    unsigned long total1 = idle1 + nonIdle1;
    unsigned long total2 = idle2 + nonIdle2;

    if (total2 <= total1 || idle2 < idle1) {
        TM_LOG_WARN("Invalid or overflowed CPU state transition detected - using previous cached value");
        // Return cached CPU usage value if available, or 0 as fallback
        return cpu_usage_cached.load();
    }

    unsigned long totalDelta = total2 - total1;
    unsigned long idleDelta  = idle2 - idle1;

    if (totalDelta == 0) {
        TM_LOG_WARN("CPU state snapshot interval too short or invalid (totalDelta == 0)");
        return 0.0;
    }
    return 100.0 * (totalDelta - idleDelta) / totalDelta;
}

// The lines read from /proc/meminfo have the format:
//   Key:            Value    Unit
// Example:
//   MemTotal:       16277048 kB
//   MemFree:        13540296 kB
//   MemAvailable:   14443896 kB
//   Buffers:          123456 kB
//   Cached:           654321 kB
//
// For details, see: https://www.kernel.org/doc/html/latest/filesystems/proc.html#meminfo

MemoryStatus SysInfoUtils::getMemoryStatus() {
    std::ifstream file(MEM_INFO_PATH);
    std::string   line, key;
    double        memTotal = 0, memAvailable = 0;

    while (std::getline(file, line)) {
        std::istringstream iss(line);
        iss >> key;
        if (key == "MemTotal:") {
            iss >> memTotal;
        } else if (key == "MemAvailable:") {
            iss >> memAvailable;
        }
        if (memTotal && memAvailable) break;
    }

    if (memTotal <= 0) {
        TM_LOG_WARN("Failed to read MemTotal from {}", MEM_INFO_PATH);
        return {0, 0, false};
    }

    constexpr double KBtoGB = 1024.0 * 1024.0;
    double used = (memTotal - memAvailable) / KBtoGB;
    double total = memTotal / KBtoGB;

    return {used, total, true};
}

DiskStatus SysInfoUtils::getDiskStatus(const std::string& mount_path) {
    // Use statvfs() to get disk (filesystem) information for the "/" (root directory).
    // If fetching fails, return used disk and free disk = 0.
    struct statvfs stat{};
    if (statvfs(mount_path.c_str(), &stat) != 0) {
        TM_LOG_ERROR("Failed to get disk status using statvfs");
        return {0, 0, false};   
    }

    // statvfs fields explaination:
    // stat.f_blocks:  total number of memory blocks in the filesystem
    // stat.f_bavail:  number of free memory blocks user can use
    // stat.f_frsize:  size of one memory block

    const double bytesInGB = 1024.0 * 1024.0 * 1024.0;

    double totalDiskGB = (double)stat.f_blocks * stat.f_frsize / bytesInGB;
    double freeDiskGB  = (double)stat.f_bavail * stat.f_frsize / bytesInGB;

    if (totalDiskGB == 0) {
        TM_LOG_WARN("Total disk size is zero");
        return {0, 0, false};
    }
    double usedDiskGB = totalDiskGB - freeDiskGB;
    return {std::round(usedDiskGB * 10.0) / 10.0, std::round(freeDiskGB * 10.0) / 10.0, true};
}

// The files inside /sys/class/thermal/thermal_zone directories have the format:
//   /sys/class/thermal/thermal_zone0/
//   /sys/class/thermal/thermal_zone1/
//   /sys/class/thermal/thermal_zone2/
//   ...
//
// Each directory contains:
//   type:   sensor type (e.g., "x86_pkg_temp", "coretemp", "cpu-thermal")
//   temp:   temperature in millidegrees Celsius (e.g., "42000" = 42.0°C)
//
// Example:
//   /sys/class/thermal/thermal_zone0/type:  x86_pkg_temp
//   /sys/class/thermal/thermal_zone0/temp:  45000
//
// For details, see:
// https://www.kernel.org/doc/html/latest/driver-api/thermal/sysfs-api.html

double SysInfoUtils::getTemperature() {
    const std::string thermalBase = "/sys/class/thermal/";

    // Dynamically iterate through all directories matching "thermal_zone*"
    for (const auto& entry : std::filesystem::directory_iterator(thermalBase)) {
        if (!entry.is_directory()) continue;
        auto path = entry.path();
        if (path.filename().string().find("thermal_zone") != 0) continue;

        std::string   typePath = (path / "type").string();
        std::ifstream typeFile(typePath);
        std::string   type;
        // Sensor types containing keywords like "cpu", "pkg", "core", or "coretemp" indicate a CPU-related sensor.
        if (typeFile.is_open() && std::getline(typeFile, type)) {
            if (type.find("cpu") != std::string::npos || type.find("pkg") != std::string::npos ||
                type.find("core") != std::string::npos || type.find("coretemp") != std::string::npos) {
                std::string   tempPath = (path / "temp").string();
                std::ifstream tempFile(tempPath);
                std::string   line;
                if (tempFile.is_open() && std::getline(tempFile, line)) {
                    try {
                        // Convert Temperature value from millidegrees Celsius to degrees Celsius
                        double temp = std::stod(line) / 1000.0;

                        constexpr double MIN_VALID_TEMP = 0.0;
                        constexpr double MAX_VALID_TEMP = 120.0;

                        // Temperature should in range (0->120) degrees Celsius
                        if (temp > MIN_VALID_TEMP && temp < MAX_VALID_TEMP)
                        { 
                            return std::round(temp * 10.0) / 10.0;
                        } else {
                            TM_LOG_WARN("Invalid temperature value {} in {}", temp, tempPath);
                        }
                    } catch (const std::exception& e) {
                        TM_LOG_DEBUG("Exception parsing temperature from {}: {}", tempPath, e.what());
                        continue;
                    }
                }
            }
        }
    }
    // If no valid temperature is found after checking all thermal zones, return 0.
    TM_LOG_WARN("No valid CPU temperature sensor found in /sys/class/thermal/thermal_zone*");
    return 0.0;
}
