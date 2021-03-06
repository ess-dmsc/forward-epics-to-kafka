// SPDX-License-Identifier: BSD-2-Clause
//
// This code has been produced by the European Spallation Source
// and its partner institutes under the BSD 2 Clause License.
//
// See LICENSE.md at the top level for license information.
//
// Screaming Udder!                              https://esss.se

#include "MainOpt.h"

#ifdef _MSC_VER
#include <iso646.h>
#else
#include <unistd.h>
#endif
#include "SchemaRegistry.h"
#include "Version.h"
#include "git_commit_current.h"
#include "logger.h"
#include <CLI/CLI.hpp>
#include <fstream>
#include <iostream>
#include <streambuf>

namespace Forwarder {

std::vector<StreamSettings> parseStreamsJson(const std::string &filepath) {
  std::ifstream ifs(filepath);
  if (!ifs.is_open()) {
    getLogger()->error("Could not open JSON file");
  }

  std::stringstream buffer;
  buffer << ifs.rdbuf();

  ConfigParser Config(buffer.str());

  return Config.extractStreamInfo().StreamsInfo;
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
  } catch (std::out_of_range &) {
    // Do nothing
  }
  try {
    int TempLogMessageLevel = std::stoi(LogLevelString.at(0));
    if (TempLogMessageLevel < 0 or TempLogMessageLevel > 5) {
      return false;
    }
    LogLevelResult = spdlog::level::level_enum(TempLogMessageLevel);
  } catch (std::invalid_argument &) {
    return false;
  }

  return true;
}

/// Add a URI valued option to the given App.
CLI::Option *addUriOption(CLI::App &App, std::string const &Name,
                          Forwarder::URI &URIArg,
                          std::string const &Description,
                          bool Defaulted = false) {
  CLI::callback_t Fun = [&URIArg](CLI::results_t Results) {
    try {
      URIArg.parse(Results[0]);
    } catch (std::runtime_error &) {
      return false;
    }
    return true;
  };
  CLI::Option *Opt = App.add_option(Name, Fun, Description, Defaulted);
  Opt->type_name("URI")->type_size(1);
  if (Defaulted) {
    Opt->default_str(URIArg.HostPort + "/" + URIArg.Topic);
  }
  return Opt;
}

CLI::Option *SetKeyValueOptions(CLI::App &App, const std::string &Name,
                                const std::string &Description, bool Defaulted,
                                const CLI::callback_t &Fun) {
  CLI::Option *Opt = App.add_option(Name, Fun, Description, Defaulted);
  const auto RequireEvenNumberOfPairs = -2;
  Opt->type_name("KEY VALUE")->type_size(RequireEvenNumberOfPairs);
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

std::pair<ParseOptRet, std::unique_ptr<MainOpt>> parse_opt(int argc,
                                                           char **argv) {
  std::pair<ParseOptRet, std::unique_ptr<MainOpt>> ret{
      ParseOptRet::Success, std::make_unique<MainOpt>()};
  auto &MainOptions = *ret.second;
  CLI::App App{
      fmt::format("forward-epics-to-kafka-0.1.0 {:.7} (ESS, BrightnESS)\n"
                  "  https://github.com/ess-dmsc/forward-epics-to-kafka\n\n",
                  GIT_COMMIT)};
  App.add_flag("--version", MainOptions.PrintVersion,
               "Print application version and exit");
  App.add_option("--log-file", MainOptions.LogFilename, "Log filename");
  App.add_option("--streams-json", MainOptions.StreamsFile,
                 "Json file for streams to add")
      ->check(CLI::ExistingFile);
  App.add_option("--kafka-gelf", MainOptions.KafkaGELFAddress,
                 "Kafka GELF logging //broker[:port]/topic");
  App.add_option("--graylog-logger-address", MainOptions.GraylogLoggerAddress,
                 "Address for Graylog logging");
  App.add_option("--influx-url", MainOptions.InfluxURI,
                 "Address for Influx logging");
  std::string LogLevelInfoStr =
      R"*(Set log message level. Set to 0 - 5 or one of
  `Trace`, `Debug`, `Info`, `Warning`, `Error`
  or `Critical`. Ex: "-v Debug". Default: `Error`)*";
  App.add_option(
         "-v,--verbosity",
         [&MainOptions, LogLevelInfoStr](std::vector<std::string> Input) {
           return parseLogLevel(Input, MainOptions.LogLevel);
         },
         LogLevelInfoStr)
      ->default_val("Error");
  addUriOption(App, "--config-topic", MainOptions.MainSettings.BrokerConfig,
               "<host[:port]/topic> Kafka host/topic to listen for commands on",
               true)
      ->required();
  addUriOption(App, "--status-topic", MainOptions.MainSettings.StatusReportURI,
               "<host[:port][/topic]> Kafka broker/topic to publish status "
               "updates on");
  App.add_option("--pv-update-period", MainOptions.PeriodMS,
                 "Force forwarding all PVs with this period even if values "
                 "are not updated (ms). 0=Off",
                 true);
  App.add_option("--fake-pv-period", MainOptions.FakePVPeriodMS,
                 "Generates and forwards fake (random "
                 "value) PV updates with the specified period in milliseconds, "
                 "instead of forwarding real "
                 "PV updates from EPICS",
                 true);
  App.add_option("--conversion-threads",
                 MainOptions.MainSettings.ConversionThreads,
                 "Conversion threads", true);
  App.add_option("--conversion-worker-queue-size",
                 MainOptions.MainSettings.ConversionWorkerQueueSize,
                 "Conversion worker queue size", true);
  App.add_option("--main-poll-interval",
                 MainOptions.MainSettings.MainPollInterval,
                 "Main Poll interval", true);
  addKafkaOption(App, "-S,--kafka-config",
                 MainOptions.MainSettings.KafkaConfiguration,
                 "LibRDKafka options");
  App.set_config("-c,--config-file", "", "Read configuration from an ini file",
                 false);

  try {
    App.parse(argc, argv);
  } catch (CLI::CallForHelp const &) {
    ret.first = ParseOptRet::Error;
  } catch (CLI::ParseError const &e) {
    if (!MainOptions.PrintVersion) {
      spdlog::log(spdlog::level::err, "Can not parse command line options: {}",
                  e.what());
      ret.first = ParseOptRet::Error;
    }
  }

  if (MainOptions.PrintVersion) {
    fmt::print("{}\n", Versioning::GetVersion());
    ret.first = ParseOptRet::VersionRequested;
    return ret;
  }

  setUpLogging(MainOptions.LogLevel, MainOptions.LogFilename,
               MainOptions.GraylogLoggerAddress);
  SharedLogger Logger = getLogger();

  if (ret.first == ParseOptRet::Error) {
    std::cout << App.help();
    return ret;
  }
  if (!MainOptions.StreamsFile.empty()) {
    try {
      MainOptions.MainSettings.StreamsInfo =
          parseStreamsJson(MainOptions.StreamsFile);
    } catch (std::exception const &e) {
      Logger->warn("Can not parse configuration file: {}", e.what());
      ret.first = ParseOptRet::Error;
      return ret;
    }
  }
  return ret;
}
} // namespace Forwarder
