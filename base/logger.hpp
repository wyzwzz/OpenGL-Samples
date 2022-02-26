#pragma once
#include <spdlog/spdlog.h>

#define LOG_INFO(str, ...) spdlog::info(str, ##__VA_ARGS__)
#define LOG_ERROR(str, ...) spdlog::error(str, ##__VA_ARGS__)

