#include "MainOpt.h"

#ifdef _MSC_VER
#include "WinSock2.h"
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
#include <streambuf>

namespace Forwarder {

MainOpt::MainOpt() {
  Hostname.resize(256);
  gethostname(Hostname.data(), Hostname.size());
  if (Hostname.back() != 0) {
    // likely an error
    Hostname.back() = 0;
  }
}

void MainOpt::set_broker(std::string &Broker) {
  ConfigParser::setBrokers(Broker, MainSettings);
}

std::string MainOpt::brokers_as_comma_list() const {
  std::string s1;
  int i1 = 0;
  for (auto &x : MainSettings.Brokers) {
    if (i1) {
      s1 += ",";
    }
    s1 += x.host_port;
    ++i1;
  }
  return s1;
}

void MainOpt::parse_json_file(std::string ConfigurationFile) {
  if (ConfigurationFile.empty()) {
    throw std::runtime_error("Name of configuration file is empty");
  }
  this->ConfigurationFile = ConfigurationFile;

  // Parse the JSON configuration and extract parameters.
  // These parameters take precedence over what is given on the
  // command line.
  MainSettings = parse_document(ConfigurationFile);

  broker_opt.ConfigurationStrings =
      MainSettings.BrokerSettings.ConfigurationStrings;
  broker_opt.ConfigurationIntegers =
      MainSettings.BrokerSettings.ConfigurationIntegers;
}

ConfigSettings MainOpt::parse_document(const std::string &filepath) {
  std::ifstream ifs(filepath);
  if (!ifs.is_open()) {
    LOG(3, "Could not open JSON file")
  }

  std::stringstream buffer;
  buffer << ifs.rdbuf();

  ConfigParser Config;
  Config.setJsonFromString(buffer.str());
  Config.extractConfiguration();

  return Config.extractConfiguration();
}

/// Add a URI valued option to the given App.
static void addOption(CLI::App &App, std::string const &Name,
                      Forwarder::URI &URIArg, std::string const &Description,
                      bool Defaulted = false) {
  CLI::callback_t Fun = [&URIArg](CLI::results_t Results) {
    URIArg.parse(Results[0]);
    return true;
  };
  CLI::Option *Opt = App.add_option(Name, Fun, Description, Defaulted);
  Opt->set_custom_option("URI", 1);
  if (Defaulted) {
    Opt->set_default_str(std::string("//") + URIArg.host_port + "/" +
                         URIArg.topic);
  }
}

std::pair<int, std::unique_ptr<MainOpt>> parse_opt(int argc, char **argv) {
  std::pair<int, std::unique_ptr<MainOpt>> ret{0, make_unique<MainOpt>()};
  auto &opt = *ret.second;
  CLI::App App{
      fmt::format("forward-epics-to-kafka-0.1.0 {:.7} (ESS, BrightnESS)\n"
                  "  https://github.com/ess-dmsc/forward-epics-to-kafka\n\n",
                  GIT_COMMIT)};
  std::string BrokerDataDefault;
  App.add_option("--config-file", opt.ConfigurationFile,
                 "Configuration JSON file");
  App.add_option("--log-file", opt.LogFilename, "Log filename");
  App.add_option("--broker", BrokerDataDefault, "Default broker for data");
  App.add_option("--kafka-gelf", opt.KafkaGELFAddress,
                 "Kafka GELF logging //broker[:port]/topic");
  App.add_option("--graylog-logger-address", opt.GraylogLoggerAddress,
                 "Address for Graylog logging");
  App.add_option("--influx-url", opt.InfluxURI, "Address for Influx logging");
  App.add_option("-v,--verbose", log_level, "Syslog logging level", true)
      ->check(CLI::Range(1, 7));
  addOption(App, "--broker-config", opt.MainSettings.BrokerConfig,
            "<//host[:port]/topic> Kafka host/topic to listen for commands on",
            true);
  addOption(App, "--status-uri", opt.MainSettings.StatusReportURI,
            "<//host[:port][/topic]> Kafka broker/topic to publish status "
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

  try {
    App.parse(argc, argv);
  } catch (CLI::CallForHelp const &e) {
    ret.first = 1;
  } catch (CLI::ParseError const &e) {
    LOG(3, "Can not parse command line options: {}", e.what());
    ret.first = 1;
  }
  if (ret.first == 1) {
    std::cout << App.help();
    return ret;
  }
  if (!opt.ConfigurationFile.empty()) {
    try {
      opt.parse_json_file(opt.ConfigurationFile);
    } catch (std::exception const &e) {
      LOG(4, "Can not parse configuration file: {}", e.what());
      ret.first = 1;
      return ret;
    }
  }
  if (!BrokerDataDefault.empty()) {
    opt.set_broker(BrokerDataDefault);
  }
  return ret;
}

void MainOpt::init_logger() {
  if (!KafkaGELFAddress.empty()) {
    Forwarder::URI uri(KafkaGELFAddress);
    log_kafka_gelf_start(uri.host, uri.topic);
    LOG(3, "Enabled kafka_gelf: //{}/{}", uri.host, uri.topic);
  }
  if (!GraylogLoggerAddress.empty()) {
    fwd_graylog_logger_enable(GraylogLoggerAddress);
  }
}
} // namespace Forwarder
