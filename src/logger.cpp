#include "logger.h"
#include "URI.h"
#include <graylog_logger/Log.hpp>
using my_sink_mt = spdlog::sinks::graylog_sink<std::mutex>;

void setUpLogging(const spdlog::level::level_enum &LoggingLevel,
                  const std::string &LogFile, const std::string &GraylogURI) {
  std::vector<spdlog::sink_ptr> Sinks;
  if (!LogFile.empty()) {
    Sinks.push_back(
        std::make_shared<spdlog::sinks::basic_file_sink_mt>(LogFile));
  }
  if (!GraylogURI.empty()) {
    Forwarder::URI TempURI(GraylogURI);
    //    Set up URI interface here
    std::string Host = "localhost";
    int Port = 12201;
    Sinks.push_back(
        std::make_shared<spdlog::sinks::graylog_sink_mt>(Host, Port));
  } else {
    Sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
  }
  auto combined_logger = std::make_shared<spdlog::logger>(
      "ForwarderLogger", begin(Sinks), end(Sinks));
  spdlog::register_logger(combined_logger);
  spdlog::set_level(LoggingLevel);
}

void setUpInitializationLogging() {
  spdlog::set_level(spdlog::level::err);
  spdlog::stdout_color_mt("InitializationLogger");
}
