#pragma once

#include <fmt/format.h>
#include <spdlog/spdlog.h>
#include <string>
#define UNUSED_ARG(x) (void)x;

using SharedLogger = std::shared_ptr<spdlog::logger>;

SharedLogger getLogger();

void setUpLogging(const spdlog::level::level_enum &LoggingLevel,
                  const std::string &LogFile, const std::string &GraylogURI);
