#include "MainOpt.h"

#ifdef _MSC_VER
#include <iso646.h>
#else
#include <unistd.h>
#endif
#include "SchemaRegistry.h"
#include "git_commit_current.h"
#include "helper.h"
#include "logger.h"
#include <CLI/CLI.hpp>
#include <fstream>
#include <iostream>
#include <spdlog/spdlog.h>
#include <streambuf>

namespace Forwarder {

std::string MainOpt::brokers_as_comma_list() const {
  std::string CommaList;
  bool MultipleBrokers = false;
  for (auto &Broker : MainSettings.Brokers) {
    if (MultipleBrokers) {
      CommaList += ",";
    }
    CommaList += Broker.HostPort;
    MultipleBrokers = true;
  }
  return CommaList;
}

bool parseLogLevel(std::vector<std::string> LogLevelString,
                   spdlog::level::level_enum &LogLevelResult) {
  std::map<std::string, spdlog::level::level_enum> LevelMap{
      {"Critical", spdlog::level::critical}, {"Error", spdlog::level::err},
      {"Warning", spdlog::level::warn},      {"Info", spdlog::level::info},
      {"Debug", spdlog::level::debug},       {"Trace", spdlog::level::trace}};

  if (LogLevelString.size() != 1) {
    return false;
  }
  try {
    LogLevelResult = LevelMap.at(LogLevelString.at(0));
    return true;
  } catch (std::out_of_range &e) {
    // Do nothing
  }
  try {
    int TempLogMessageLevel = std::stoi(LogLevelString.at(0));
    if (TempLogMessageLevel < 1 or TempLogMessageLevel > 7) {
      return false;
    }
    LogLevelResult = spdlog::level::level_enum(TempLogMessageLevel);
  } catch (std::invalid_argument &e) {
    return false;
  }
  return true;
}

std::vector<StreamSettings> parseStreamsJson(const std::string &filepath) {
  std::ifstream ifs(filepath);
  if (!ifs.is_open()) {
    spdlog::get("ForwarderLogger")->error("Could not open JSON file");
  }

  std::stringstream buffer;
  buffer << ifs.rdbuf();

  ConfigParser Config(buffer.str());

  return Config.extractStreamInfo().StreamsInfo;
}

/// Add a URI valued option to the given App.
CLI::Option *addUriOption(CLI::App &App, std::string const &Name,
                          Forwarder::URI &URIArg,
                          std::string const &Description,
                          bool Defaulted = false) {
  CLI::callback_t Fun = [&URIArg](CLI::results_t Results) {
    try {
      URIArg.parse(Results[0]);
    } catch (std::runtime_error &E) {
      return false;
    }
    return true;
  };
  CLI::Option *Opt = App.add_option(Name, Fun, Description, Defaulted);
  Opt->set_custom_option("URI", 1);
  if (Defaulted) {
    Opt->set_default_str(URIArg.HostPort + "/" + URIArg.Topic);
  }
  return Opt;
}

CLI::Option *SetKeyValueOptions(CLI::App &App, const std::string &Name,
                                const std::string &Description, bool Defaulted,
                                const CLI::callback_t &Fun) {
  CLI::Option *Opt = App.add_option(Name, Fun, Description, Defaulted);
  const auto RequireEvenNumberOfPairs = -2;
  Opt->set_custom_option("KEY VALUE", RequireEvenNumberOfPairs);
  return Opt;
}

CLI::Option *addKafkaOption(CLI::App &App, std::string const &Name,
                            std::map<std::string, std::string> &ConfigMap,
                            std::string const &Description,
                            bool Defaulted = false) {
  CLI::callback_t Fun = [&ConfigMap](CLI::results_t Results) {
    for (size_t i = 0; i < Results.size() / 2; i++) {
      ConfigMap[Results.at(i * 2)] = Results.at(i * 2 + 1);
    }
    return true;
  };
  return SetKeyValueOptions(App, Name, Description, Defaulted, Fun);
}

CLI::Option *addKafkaOption(CLI::App &App, std::string const &Name,
                            std::map<std::string, int> &ConfigMap,
                            std::string const &Description,
                            bool Defaulted = false) {
  CLI::callback_t Fun = [&ConfigMap](CLI::results_t Results) {
    for (size_t i = 0; i < Results.size() / 2; i++) {
      try {
        ConfigMap[Results.at(i * 2)] = std::stol(Results.at(i * 2 + 1));
      } catch (std::invalid_argument &) {
        throw std::runtime_error(
            fmt::format("Argument {} is not an int", Results.at(i * 2)));
      }
    }
    return true;
  };
  return SetKeyValueOptions(App, Name, Description, Defaulted, Fun);
}

std::pair<int, std::unique_ptr<MainOpt>> parse_opt(int argc, char **argv) {
  std::pair<int, std::unique_ptr<MainOpt>> ret{0, make_unique<MainOpt>()};
  auto &opt = *ret.second;
  CLI::App App{
      fmt::format("forward-epics-to-kafka-0.1.0 {:.7} (ESS, BrightnESS)\n"
                  "  https://github.com/ess-dmsc/forward-epics-to-kafka\n\n",
                  GIT_COMMIT)};
  App.add_option("--log-file", opt.LogFilename, "Log filename");
  App.add_option("--streams-json", opt.StreamsFile,
                 "Json file for streams to add")
      ->check(CLI::ExistingFile);
  App.add_option("--kafka-gelf", opt.KafkaGELFAddress,
                 "Kafka GELF logging //broker[:port]/topic");
  App.add_option("--graylog-logger-address", opt.GraylogLoggerAddress,
                 "Address for Graylog logging");
  App.add_option("--influx-url", opt.InfluxURI, "Address for Influx logging");
  std::string LogLevelInfoStr =
      R"*(Set log message level. Set to 1 - 7 or one of
  `Critical`, `Error`, `Warning`, `Notice`, `Info`,
  or `Debug`. Ex: "-l Notice")*";
  App.add_option("-v,--verbosity",
                 [&opt, LogLevelInfoStr](std::vector<std::string> Input) {
                   return parseLogLevel(Input, opt.LoggingLevel);
                 },
                 LogLevelInfoStr)
      ->set_default_val("Error");
  addOption(App, "--config-topic", opt.MainSettings.BrokerConfig,
            "<//host[:port]/topic> Kafka host/topic to listen for commands on",
            true)
  App.add_option("-v,--verbosity", log_level, "Syslog logging level", true)
      ->check(CLI::Range(1, 7));
  addUriOption(App, "--config-topic", opt.MainSettings.BrokerConfig,
               "<host[:port]/topic> Kafka host/topic to listen for commands on",
               true)
      ->required();
  addUriOption(App, "--status-topic", opt.MainSettings.StatusReportURI,
               "<host[:port][/topic]> Kafka broker/topic to publish status "
               "updates on");
  App.add_option("--pv-update-period", opt.PeriodMS,
                 "Force forwarding all PVs with this period even if values "
                 "are not updated (ms). 0=Off",
                 true);
  App.add_option("--fake-pv-period", opt.FakePVPeriodMS,
                 "Generates and forwards fake (random "
                 "value) PV updates with the specified period in milliseconds, "
                 "instead of forwarding real "
                 "PV updates from EPICS",
                 true);
  App.add_option("--conversion-threads", opt.MainSettings.ConversionThreads,
                 "Conversion threads", true);
  App.add_option("--conversion-worker-queue-size",
                 opt.MainSettings.ConversionWorkerQueueSize,
                 "Conversion worker queue size", true);
  App.add_option("--main-poll-interval", opt.MainSettings.MainPollInterval,
                 "Main Poll interval", true);
  addKafkaOption(App, "-S,--kafka-config", opt.MainSettings.KafkaConfiguration,
                 "LibRDKafka options");
  App.set_config("-c,--config-file", "", "Read configuration from an ini file",
                 false);
  ::setUpInitializationLogging();
  try {
    App.parse(argc, argv);
  } catch (CLI::CallForHelp const &e) {
    ret.first = 1;
  } catch (CLI::ParseError const &e) {
    auto Logger = spdlog::get("InitializationLogger");
    Logger->error("Can not parse command line options: {}", e.what());
    ret.first = 1;
  }
  if (ret.first == 1) {
    std::cout << App.help();
    return ret;
  }
  if (!opt.StreamsFile.empty()) {
    try {
      opt.MainSettings.StreamsInfo = parseStreamsJson(opt.StreamsFile);
    } catch (std::exception const &e) {
      auto Logger = spdlog::get("InitializationLogger");
      Logger->warn("Can not parse configuration file: {}", e.what());
      ret.first = 1;
      return ret;
    }
  }
  return ret;
}

void MainOpt::init_logger() {
  setUpLogging(LoggingLevel, LogFilename, GraylogLoggerAddress);
}
} // namespace Forwarder
