#include "MainOpt.h"

#ifdef _MSC_VER
#include "WinSock2.h"
#include "wingetopt.h"
#include <iso646.h>
#else
#include <getopt.h>
#include <unistd.h>
#endif
#include "SchemaRegistry.h"
#include "blobs.h"
#include "git_commit_current.h"
#include "helper.h"
#include "logger.h"
#include <iostream>
#include <rapidjson/filereadstream.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/schema.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <rapidjson/istreamwrapper.h>
#include <fstream>

namespace BrightnESS {
namespace ForwardEpicsToKafka {

using std::string;
using std::vector;

MainOpt::MainOpt() {
  hostname.resize(128);
  gethostname(hostname.data(), hostname.size());
  if (hostname.back()) {
    // likely an error
    hostname.back() = 0;
  }
  set_broker("localhost:9092");
}

void MainOpt::set_broker(string broker) {
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

int MainOpt::parse_json_file(string config_file) {
  if (config_file.empty()) {
    LOG(3, "given config filename is empty");
    return -1;
  }
  this->config_file = config_file;
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
  // Currently, these parameters tke precedence over what is given on the
  // command line.
  auto document = parse_document(config_file);
  if (document.IsNull()) {
    return -4;
  }
  if (document.HasParseError()) {
    LOG(3, "configuration is not well formed");
    return -5;
  }

  SchemaValidator vali(schema);

  if (!document.Accept(vali)) {
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
  set_broker(find_broker(document));
  broker_config = find_broker_config(document);
  conversion_threads = find_conversion_threads(document);
  conversion_worker_queue_size = find_conversion_worker_queue_size(document);
  main_poll_interval = find_main_poll_interval(document);
  find_brokers_config(document);
  find_status_uri(document);
  return 0;
}

rapidjson::Document MainOpt::parse_document(const std::string filepath) {
  std::ifstream ifs(filepath);
  if (!ifs.is_open()){
    LOG(3, "Could not open JSON file")
    return nullptr;
  }
  rapidjson::IStreamWrapper isw(ifs);
  rapidjson::Document d;
  d.ParseStream(isw);
  return d;
}

void MainOpt::find_status_uri(rapidjson::Document &document) {
  auto itr = document.FindMember("status-uri");
  if (itr != document.MemberEnd() && itr->value.IsString()) {
    uri::URI u1;
    u1.port = 9092;
    u1.parse(itr->value.GetString());
    status_uri = u1;
  }
  LOG(3, "ERROR could not find status URI in config json")
}

void MainOpt::find_brokers_config(rapidjson::Document &document) {
  auto kafka = document.FindMember("kafka");
  if (kafka != document.MemberEnd()) {
    auto &brokers = kafka->value;
    if (brokers.IsObject()) {
      auto broker = brokers.FindMember("broker");
      if (broker != document.MemberEnd()) {
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

int MainOpt::find_int(rapidjson::Document &document, const char *key) const {
  auto itr = document.FindMember(key);
  if (itr != document.MemberEnd()) {
    if (itr->value.IsInt()) {
      return itr->value.GetInt();
    }
  }
  LOG(3, "ERROR could not find {} in json", key)
  return 0;
}

int MainOpt::find_main_poll_interval(rapidjson::Document &document) {
  return find_int(document, "main-poll-interval");
}

uint32_t MainOpt::find_conversion_worker_queue_size(rapidjson::Document &document) {
  return static_cast<uint32_t>(find_int(document, "conversion-worker-queue-size"));
}

int MainOpt::find_conversion_threads(rapidjson::Document &document) {
  return find_int(document, "conversion-threads");
}

uri::URI MainOpt::find_broker_config(rapidjson::Document &document) {
  auto itr = document.FindMember("broker-config");
  if (itr != document.MemberEnd()) {
    if (itr->value.IsString()) {
      return {std::string(itr->value.GetString())};
    }
  }
  LOG(3, "ERROR could not find broker config in json")
  return uri::URI();
}

std::string MainOpt::find_broker(rapidjson::Document &document) {
  auto itr = document.FindMember("broker");
  if (itr != document.MemberEnd()){
    if (itr->value.IsString()) {
      return std::string(itr->value.GetString());
    }
  }
  LOG(3, "ERROR could not find broker in json")
  return "";
}

std::pair<int, std::unique_ptr<MainOpt>> parse_opt(int argc, char **argv) {
  std::pair<int, std::unique_ptr<MainOpt>> ret{
      0, std::unique_ptr<MainOpt>(new MainOpt)};
  auto &opt = *ret.second;
  int option_index = 0;
  bool getopt_error = false;
  while (true) {
    int c = getopt_long(argc, argv, "hvQ", LONG_OPTIONS, &option_index);
    if (c == -1)
      break;
    if (c == '?') {
      getopt_error = true;
    }
    switch (c) {
    case 'h':
      opt.help = true;
      break;
    case 'v':
      log_level = (std::min)(9, log_level + 1);
      break;
    case 'Q':
      log_level = (std::max)(0, log_level - 1);
      break;
    default:
      // long option without short equivalent:
      parse_long_argument(LONG_OPTIONS[option_index].name, ret, opt);
    }
  }
  if (optind < argc) {
    LOG(6, "Left-over commandline options:");
    for (int i1 = optind; i1 < argc; ++i1) {
      LOG(6, "{:2} {}", i1, argv[i1]);
    }
  }
  if (getopt_error) {
    opt.help = true;
    ret.first = 1;
  }
  if (opt.help) {
    fmt::print("forward-epics-to-kafka-0.1.0 {:.7} (ESS, BrightnESS)\n"
               "  Contact: dominik.werder@psi.ch\n\n",
               GIT_COMMIT);
    fmt::print(MAN_PAGE, opt.brokers_as_comma_list());
    ret.first = 1;
  }
  return ret;
}
void parse_long_argument(const char *lname, std::pair<int, std::unique_ptr<MainOpt>> &ret, MainOpt &opt) {
  if (string("help") == lname) {
        opt.help = true;
      }
  if (string("config-file") == lname) {
        if (opt.parse_json_file(optarg) != 0) {
          opt.help = true;
          ret.first = 1;
        }
      }
  if (string("log-file") == lname) {
        opt.log_file = optarg;
      }
  if (string("broker-config") == lname) {
        uri::URI u1;
        u1.port = 9092;
        u1.parse(optarg);
        opt.broker_config = u1;
      }
  if (string("broker") == lname) {
        opt.set_broker(optarg);
      }
  if (string("kafka-gelf") == lname) {
        opt.kafka_gelf = optarg;
      }
  if (string("graylog-logger-address") == lname) {
        opt.graylog_logger_address = optarg;
      }
  if (string("influx-url") == lname) {
        opt.influx_url = optarg;
      }
  if (string("forwarder-ix") == lname) {
        opt.forwarder_ix = std::stoi(optarg);
      }
  if (string("write-per-message") == lname) {
        opt.write_per_message = std::stoi(optarg);
      }
  if (string("teamid") == lname) {
        opt.teamid = strtoul(optarg, nullptr, 0);
      }
  if (string("status-uri") == lname) {
        uri::URI u1;
        u1.port = 9092;
        u1.parse(optarg);
        opt.status_uri = u1;
      }
}

void MainOpt::init_logger() {
  if (!kafka_gelf.empty()) {
    uri::URI uri(kafka_gelf);
    log_kafka_gelf_start(uri.host, uri.topic);
    LOG(3, "Enabled kafka_gelf: //{}/{}", uri.host, uri.topic);
  }
  if (!graylog_logger_address.empty()) {
    fwd_graylog_logger_enable(graylog_logger_address);
  }
}
}
}
