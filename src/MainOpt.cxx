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
#include "logger.h"
#include <CLI/CLI.hpp>
#include <fstream>
#include <iostream>
#include <rapidjson/filereadstream.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/schema.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

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

int MainOpt::parse_json_file(std::string ConfigurationFile) {
  if (ConfigurationFile.empty()) {
    LOG(3, "given config filename is empty");
    return -1;
  }
  this->ConfigurationFile = ConfigurationFile;
  using namespace rapidjson;
  Document schema_;
  try {
    schema_.Parse(blobs::schema_config_global_json,
                  strlen(blobs::schema_config_global_json));
  } catch (...) {
    LOG(3, "schema is not valid!");
    return -2;
  }
  if (schema_.HasParseError()) {
    LOG(3, "could not parse schema");
    return -3;
  }
  SchemaDocument schema(schema_);

  // Parse the JSON configuration and extract parameters.
  // Currently, these parameters take precedence over what is given on the
  // command line.
  parse_document(ConfigurationFile);
  if (json->IsNull()) {
    return -4;
  }
  if (json->HasParseError()) {
    LOG(3, "configuration is not well formed");
    return -5;
  }

  SchemaValidator vali(schema);

  if (!json->Accept(vali)) {
    StringBuffer sb1, sb2;
    vali.GetInvalidSchemaPointer().StringifyUriFragment(sb1);
    vali.GetInvalidDocumentPointer().StringifyUriFragment(sb2);
    LOG(3,
        "command message schema validation:  Invalid schema: {}  keyword: {}",
        sb1.GetString(), vali.GetInvalidSchemaKeyword());
    if (std::string("additionalProperties") == vali.GetInvalidSchemaKeyword()) {
      LOG(3, "Sorry, you have probably specified more properties than what is "
             "allowed by the schema.");
    }
    LOG(3, "configuration is not valid");
    return -6;
  }
  vali.Reset();

  find_broker_config(BrokerConfig);
  find_conversion_threads(conversion_threads);
  find_conversion_worker_queue_size(conversion_worker_queue_size);
  find_main_poll_interval(main_poll_interval);

  find_brokers_config();
  find_status_uri();
  find_broker();
  return 0;
}

void MainOpt::parse_document(const std::string &filepath) {
  std::ifstream ifs(filepath);
  if (!ifs.is_open()) {
    LOG(3, "Could not open JSON file")
  }
  rapidjson::IStreamWrapper isw(ifs);
  json = std::make_shared<rapidjson::Document>();
  json->ParseStream(isw);
}

void MainOpt::find_status_uri() {
  auto itr = json->FindMember("status-uri");
  if (itr != json->MemberEnd() && itr->value.IsString()) {
    uri::URI u1;
    u1.port = 9092;
    u1.parse(itr->value.GetString());
    StatusReportURI = u1;
  }
}

void MainOpt::find_brokers_config() {
  auto kafka = json->FindMember("kafka");
  if (kafka != json->MemberEnd()) {
    auto &brokers = kafka->value;
    if (brokers.IsObject()) {
      auto broker = brokers.FindMember("broker");
      if (broker != json->MemberEnd()) {
        auto &broker_properties = broker->value;
        if (broker_properties.IsObject()) {
          for (auto &broker_property : broker_properties.GetObject()) {
            auto const &name = broker_property.name.GetString();
            if (strncmp("___", name, 3) != 0) {
              if (broker_property.value.IsString()) {
                auto const &value = broker_property.value.GetString();
                LOG(6, "kafka broker config {}: {}", name, value);
                broker_opt.conf_strings[name] = value;
              } else if (broker_property.value.IsInt()) {
                auto const &value = broker_property.value.GetUint();
                LOG(6, "kafka broker config {}: {}", name, value);
                broker_opt.conf_ints[name] = value;
              } else {
                LOG(3, "ERROR can not understand option: {}", name);
              }
            }
          }
        }
      }
    }
  }
}

void MainOpt::find_int(const char *key, int &property) const {
  auto itr = json->FindMember(key);
  if (itr != json->MemberEnd()) {
    if (itr->value.IsInt()) {
      property = itr->value.GetInt();
    }
  }
}

void MainOpt::find_main_poll_interval(int &property) {
  find_int("main-poll-interval", property);
}

void MainOpt::find_conversion_worker_queue_size(uint32_t &property) {
  find_uint32_t("conversion-worker-queue-size", property);
}

void MainOpt::find_uint32_t(const char *key, uint32_t &property) {
  auto itr = json->FindMember(key);
  if (itr != json->MemberEnd()) {
    if (itr->value.IsInt()) {
      property = static_cast<uint32_t>(itr->value.GetInt());
    }
  }
}

void MainOpt::find_conversion_threads(int &property) {
  find_int("conversion-threads", property);
}

void MainOpt::find_broker_config(uri::URI &property) {
  auto itr = json->FindMember("broker-config");
  if (itr != json->MemberEnd()) {
    if (itr->value.IsString()) {
      property = {std::string(itr->value.GetString())};
    }
  }
}

void MainOpt::find_broker() {
  auto itr = json->FindMember("broker");
  if (itr != json->MemberEnd()) {
    if (itr->value.IsString()) {
      set_broker(itr->value.GetString());
    }
  }
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
    if (opt.parse_json_file(opt.ConfigurationFile) != 0) {
      LOG(4, "Can not parse configuration file");
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
