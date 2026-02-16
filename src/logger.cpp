#include "heidi-kernel/logger.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace heidi {

Logger::Logger(std::ostream& out, LogLevel min_level)
    : out_(out), min_level_(min_level) {}

void Logger::set_level(LogLevel level) {
    min_level_ = level;
}

LogLevel Logger::level() const {
    return min_level_;
}

void Logger::debug(std::string_view msg) {
    log(LogLevel::Debug, msg);
}

void Logger::info(std::string_view msg) {
    log(LogLevel::Info, msg);
}

void Logger::warn(std::string_view msg) {
    log(LogLevel::Warn, msg);
}

void Logger::error(std::string_view msg) {
    log(LogLevel::Error, msg);
}

std::string_view Logger::level_to_string(LogLevel level) {
    switch (level) {
        case LogLevel::Debug: return "DEBUG";
        case LogLevel::Info: return "INFO";
        case LogLevel::Warn: return "WARN";
        case LogLevel::Error: return "ERROR";
    }
    return "UNKNOWN";
}

void Logger::log(LogLevel level, std::string_view msg) {
    if (level < min_level_) {
        return;
    }

    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    std::tm tm_buf{};
#if defined(_WIN32) || defined(_WIN64)
    localtime_s(&tm_buf, &time);
#else
    localtime_r(&time, &tm_buf);
#endif

    out_ << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S")
         << "." << std::setfill('0') << std::setw(3) << ms.count()
         << " [" << level_to_string(level) << "] "
         << msg << "\n";
}

}
