#pragma once
#include <vector>
#include <string>
#include <rapidjson/document.h>
#include "KafkaW.h"
#include "SchemaRegistry.h"

namespace BrightnESS {
namespace ForwardEpicsToKafka {

using std::string;
using std::vector;

struct MainOpt {
string broker_configuration_address = "localhost:9092";
string broker_configuration_topic = "configuration.global";
string broker_data_address = "localhost:9092";
string kafka_gelf = "";
string graylog_logger_address = "";
bool help = false;
string log_file;
string config_file;
//std::map<std::string, int> kafka_conf_ints;
int forwarder_ix = 0;
int write_per_message = 0;
uint64_t teamid = 0;
FlatBufs::SchemaRegistry schema_registry;

// When parsing options, we keep the json document because it may also contain
// some topic mappings which are read by the Main.
std::shared_ptr<rapidjson::Document> json = nullptr;
int parse_json_file(string config_file);
KafkaW::BrokerOpt broker_opt;

void init_after_parse();
};


std::pair<int, std::unique_ptr<MainOpt>> parse_opt(int argc, char ** argv);

}
}
