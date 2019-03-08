#pragma once
#include <fmt/format.h>
#include <string>
// clang-format off
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/graylog_sink.h>
// clang-format on
#define UNUSED_ARG(x) (void)x;

void setUpInitializationLogging();

void setUpLogging(const spdlog::level::level_enum &LoggingLevel,
                  const std::string &LogFile, const std::string &GraylogURI);