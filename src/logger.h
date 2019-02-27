#pragma once

#include <fmt/format.h>
#include <string>
// clang-format off
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
// clang-format on
#ifdef _MSC_VER

// The levels used in the LOG macro are defined in the spdlog::level namespace
// in spdlog.h
#define LOG(level, fmt, ...)                                                   \
  { spdlog::get("filewriterlogger")->log(level, fmt, __VA_ARGS__); }
#else
#define LOG(level, fmt, args...)                                               \
  { spdlog::get("ForwarderLogger")->log(level, fmt, ##args); }
#endif
#define UNUSED_ARG(x) (void)x;

void setUpLogging(const spdlog::level::level_enum &LoggingLevel,
                  const std::string &LogFile, const std::string &GraylogURI);