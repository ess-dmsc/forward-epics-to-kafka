#include "MainOpt.h"

#ifdef _MSC_VER
#include "WinSock2.h"
#include <iso646.h>
#else
#include <unistd.h>
#endif
#include "SchemaRegistry.h"
#include "blobs.h"
#include "git_commit_current.h"
#include "helper.h"
#include "json.h"
#include "logger.h"
#include <CLI/CLI.hpp>
#include <fstream>
#include <iostream>

namespace BrightnESS {
namespace ForwardEpicsToKafka {

MainOpt::MainOpt() {
  Hostname.resize(256);
  gethostname(Hostname.data(), Hostname.size());
  if (Hostname.back() != 0) {
    // likely an error
    Hostname.back() = 0;
  }
  set_broker("localhost:9092");
}

void MainOpt::set_broker(std::string broker) {
  brokers.clear();
  auto a = split(broker, ",");
  for (auto &x : a) {
    uri::URI u1;
    u1.require_host_slashes = false;
    u1.parse(x);
    brokers.push_back(u1);
  }
}

std::string MainOpt::brokers_as_comma_list() const {
  std::string s1;
  int i1 = 0;
  for (auto &x : brokers) {
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
  parse_document(ConfigurationFile);

  findBrokerConfig();
  findConversionThreads();
  findConversionWorkerQueueSize();
  findMainPollInterval();

  auto Settings = extractKafkaBrokerSettingsFromJSON(JSONConfiguration);
  broker_opt.ConfigurationStrings = Settings.ConfigurationStrings;
  broker_opt.ConfigurationIntegers = Settings.ConfigurationIntegers;

  find_status_uri();

  findBroker();
}

void MainOpt::findMainPollInterval() {
  if (auto x = find<int32_t>("main-poll-interval", JSONConfiguration)) {
    main_poll_interval = x.inner();
  }
}

void MainOpt::findConversionWorkerQueueSize() {
  if (auto x =
          find<size_t>("conversion-worker-queue-size", JSONConfiguration)) {
    ConversionWorkerQueueSize = x.inner();
  }
}

void MainOpt::findConversionThreads() {
  if (auto x = find<size_t>("conversion-threads", JSONConfiguration)) {
    ConversionThreads = x.inner();
  }
}

void MainOpt::findBrokerConfig() {
  if (auto x = find<std::string>("broker-config", JSONConfiguration)) {
    BrokerConfig = x.inner();
  }
}

void MainOpt::findBroker() {
  if (auto x = find<std::string>("broker", JSONConfiguration)) {
    set_broker(x.inner());
  }
}

void MainOpt::parse_document(const std::string &filepath) {
  std::ifstream ifs(filepath);
  if (!ifs.is_open()) {
    LOG(3, "Could not open JSON file")
  }
  JSONConfiguration = nlohmann::json();
  ifs >> JSONConfiguration;

  if (JSONConfiguration.is_null()) {
    throw std::runtime_error("Can not parse configuration file as JSON");
  }
}

void MainOpt::find_status_uri() {
  if (auto x = find<std::string>("status-uri", JSONConfiguration)) {
    auto URIString = x.inner();
    uri::URI URI;
    URI.port = 9092;
    URI.parse(URIString);
    StatusReportURI = URI;
  }
}

KafkaBrokerSettings
extractKafkaBrokerSettingsFromJSON(nlohmann::json const &JSONConfiguration) {
  KafkaBrokerSettings Settings;
  using nlohmann::json;
  if (auto KafkaMaybe = find<json>("kafka", JSONConfiguration)) {
    auto Kafka = KafkaMaybe.inner();
    if (auto BrokerMaybe = find<json>("broker", Kafka)) {
      auto Broker = BrokerMaybe.inner();
      for (auto Property = Broker.begin(); Property != Broker.end();
           ++Property) {
        auto const Key = Property.key();
        if (Key.find("___") == 0) {
          // Skip this property
          continue;
        }
        if (Property.value().is_string()) {
          auto Value = Property.value().get<std::string>();
          LOG(6, "kafka broker config {}: {}", Key, Value);
          Settings.ConfigurationStrings[Key] = Value;
        } else if (Property.value().is_number()) {
          auto Value = Property.value().get<int64_t>();
          LOG(6, "kafka broker config {}: {}", Key, Value);
          Settings.ConfigurationIntegers[Key] = Value;
        } else {
          LOG(3, "can not understand option: {}", Key);
        }
      }
    }
  }
  return Settings;
}

/// Add a URI valued option to the given App.

static void addOption(CLI::App &App, std::string const &Name, uri::URI &URIArg,
                      std::string const &Description, bool Defaulted = false) {
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
                  "  Contact: dominik.werder@psi.ch\n\n",
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
  App.add_option("-i,--idle-pv-updates", opt.idle_ms, "number of milliseconds to update idle PVs. 0=Off");
  App.add_option("-v,--verbose", log_level, "Syslog logging level", true)
      ->check(CLI::Range(1, 7));
  addOption(App, "--broker-config", opt.BrokerConfig,
            "<//host[:port]/topic> Kafka host/topic to listen for commands on",
            true);
  addOption(App, "--status-uri", opt.StatusReportURI,
            "<//host[:port][/topic]> Kafka broker/topic to publish status "
            "updates on");

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
    uri::URI uri(KafkaGELFAddress);
    log_kafka_gelf_start(uri.host, uri.topic);
    LOG(3, "Enabled kafka_gelf: //{}/{}", uri.host, uri.topic);
  }
  if (!GraylogLoggerAddress.empty()) {
    fwd_graylog_logger_enable(GraylogLoggerAddress);
  }
}
}
}
