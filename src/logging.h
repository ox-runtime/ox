#pragma once

enum class LogLevel { DEBUG, INFO, LOG_ERROR };

void Log(LogLevel level, const char* message);

// Logging macros
#ifdef NDEBUG
#define LOG_DEBUG(message) ((void)0)
#else
#define LOG_DEBUG(message) Log(LogLevel::DEBUG, message)
#endif

#define LOG_INFO(message) Log(LogLevel::INFO, message)
#define LOG_ERROR(message) Log(LogLevel::LOG_ERROR, message)
