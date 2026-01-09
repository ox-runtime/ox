#pragma once

enum class LogLevel { DEBUG, INFO, LOG_ERROR };

void Log(LogLevel level, const char* message);
