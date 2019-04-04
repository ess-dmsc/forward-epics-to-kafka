#include "logger.h"
#include "URI.h"
using my_sink_mt = spdlog::sinks::graylog_sink<std::mutex>;

SharedLogger getLogger() { return spdlog::get("ForwarderLogger"); }

void setUpLogging(const spdlog::level::level_enum &LoggingLevel,
                  const std::string &LogFile, const std::string &GraylogURI) {
  std::vector<spdlog::sink_ptr> Sinks;
  if (!LogFile.empty()) {
    auto FileSink =
        std::make_shared<spdlog::sinks::basic_file_sink_mt>(LogFile);
    FileSink->set_pattern("[%Y-%m-%d %H:%M:%S.%f] [%l] [processID: %P]: %v");
    Sinks.push_back(FileSink);
  }
  if (!GraylogURI.empty()) {
#ifdef HAVE_GRAYLOG_LOGGER
    Forwarder::URI TempURI(GraylogURI);
    auto GraylogSink = std::make_shared<spdlog::sinks::graylog_sink_mt>(
        LoggingLevel, TempURI.HostPort.substr(0, TempURI.HostPort.find(":")),
        TempURI.Port);
    GraylogSink->set_pattern("[%l] [processID: %P]: %v");
    Sinks.push_back(GraylogSink);
#else
    spdlog::log(
        spdlog::level::err,
        "ERROR not compiled with support for graylog_logger. Would have used{}",
        GraylogURI);
#endif
  }
  Sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
  auto combined_logger = std::make_shared<spdlog::logger>(
      "ForwarderLogger", begin(Sinks), end(Sinks));
  spdlog::register_logger(combined_logger);
  spdlog::set_level(LoggingLevel);
}

void setUpInitializationLogging() {
  spdlog::set_level(spdlog::level::err);
  spdlog::stdout_color_mt("InitializationLogger");
}
