#pragma once
#include <fmt/format.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <string>
#define UNUSED_ARG(x) (void)x;

using SharedLogger = std::shared_ptr<spdlog::logger>;

SharedLogger getLogger();

void setUpInitializationLogging();

void setUpLogging(const spdlog::level::level_enum &LoggingLevel,
                  const std::string &LogFile, const std::string &GraylogURI);