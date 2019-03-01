#include "logger.h"
#include "URI.h"

void setUpLogging(const spdlog::level::level_enum &LoggingLevel,
                  const std::string &LogFile, const std::string &GraylogURI) {
  spdlog::set_level(LoggingLevel);
  if (!LogFile.empty()) {
    spdlog::basic_logger_mt("ForwarderLogger", LogFile);
  }
  if (!GraylogURI.empty()) {
    Forwarder::URI TempURI(GraylogURI);
    // Set up URI interface here
    // auto grayloginterface = spdlog::graylog_sink(TempURI.HostPort,
    // TempURI.Topic);
  } else {
    spdlog::stdout_color_mt("ForwarderLogger");
  }
}

void setUpInitializationLogging() {
  spdlog::set_level(spdlog::level::err);
  spdlog::stdout_color_mt("InitializationLogger");
}
