#include "logging.h"

#include <ctime>
#include <iomanip>
#include <iostream>
#include <mutex>

// Set log level based on build type
#ifdef NDEBUG
static const LogLevel g_log_level = LogLevel::INFO;
#else
static const LogLevel g_log_level = LogLevel::DEBUG;
#endif

static std::mutex g_log_mutex;

void Log(LogLevel level, const char* message) {
    if (level < g_log_level) {
        return;
    }

    std::lock_guard<std::mutex> lock(g_log_mutex);

    // Get timestamp
    auto now = std::time(nullptr);
    auto tm = *std::localtime(&now);

    // Print log level
    const char* level_str = "";
    switch (level) {
        case LogLevel::DEBUG:
            level_str = "DEBUG";
            break;
        case LogLevel::INFO:
            level_str = "INFO";
            break;
        case LogLevel::LOG_ERROR:
            level_str = "ERROR";
            break;
    }

    std::cerr << "[" << std::put_time(&tm, "%H:%M:%S") << "] "
              << "[" << level_str << "] " << message << std::endl;
}
