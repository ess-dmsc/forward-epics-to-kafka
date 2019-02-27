#pragma once

#include <fmt/format.h>
#include <string>
// clang-format off
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
// clang-format on
#ifdef _MSC_VER

#define LOG(level, fmt, ...)                                                   \
  dwlog(static_cast<int>(level), 0, fmt, __FILE__, __LINE__, __FUNCSIG__,      \
        __VA_ARGS__);
#else

#define LOG(level, fmt, args...)                                               \
  dwlog(static_cast<int>(level), 0, fmt, __FILE__, __LINE__,                   \
        __PRETTY_FUNCTION__, ##args);
#endif

#define UNUSED_ARG(x) (void)x;

void setUpLogging(const spdlog::level::level_enum &LoggingLevel,
                  const std::string &LogFile, const std::string &GraylogURI);