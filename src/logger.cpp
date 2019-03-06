#include "logger.h"
#include "URI.h"
#include <graylog_logger/Log.hpp>
using my_sink_mt = spdlog::sinks::graylog_sink<std::mutex>;

void setUpLogging(const spdlog::level::level_enum &LoggingLevel,
                  const std::string &LogFile, const std::string &GraylogURI) {
  std::vector<spdlog::sink_ptr> sinks;
  if (!LogFile.empty()) {
    sinks.push_back(
        std::make_shared<spdlog::sinks::basic_file_sink_mt>(LogFile));
  }
  if (GraylogURI.empty()) {
    Forwarder::URI TempURI(GraylogURI);
    //     Set up URI interface here
    std::string lel = "localhost";
    int a = 9000;

    //    auto Sink = std::make_shared<my_sink_mt>(lel, a);
    //    auto GraylogLogger = std::make_shared<spdlog::logger>("graylog",
    //    Sink);

    sinks.push_back(
        std::make_shared<spdlog::sinks::graylog_sink<std::mutex>>(lel, a));

    //       auto grayloginterface =
    //       spdlog::sinks::graylog_sink<std::mutex>(TempURI.HostPort,
    // TempURI.Topic);
  } else {
    sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
  }
  auto combined_logger = std::make_shared<spdlog::logger>(
      "ForwarderLogger", begin(sinks), end(sinks));
  spdlog::register_logger(combined_logger);
  spdlog::set_level(LoggingLevel);
}

void setUpInitializationLogging() {
  spdlog::set_level(spdlog::level::err);
  spdlog::stdout_color_mt("InitializationLogger");
}
