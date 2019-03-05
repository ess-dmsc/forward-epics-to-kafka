#include "logger.h"
#include "URI.h"

void setUpLogging(const spdlog::level::level_enum &LoggingLevel,
                  const std::string &LogFile, const std::string &GraylogURI) {
  std::vector<spdlog::sink_ptr> sinks;
  if (!LogFile.empty()) {
    sinks.push_back(
        std::make_shared<spdlog::sinks::basic_file_sink_mt>(LogFile));
  }
  if (!GraylogURI.empty()) {
    Forwarder::URI TempURI(GraylogURI);
    // Set up URI interface here
    // auto grayloginterface = spdlog::graylog_sink(TempURI.HostPort,
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
