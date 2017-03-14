#pragma once
#include <vector>
#include <string>
#include <rapidjson/document.h>
#include "uri.h"
#include "KafkaW.h"
#include "SchemaRegistry.h"

class MainOpt_T;

namespace BrightnESS {
namespace ForwardEpicsToKafka {

using std::string;
using std::vector;

struct MainOpt {
MainOpt();
void set_broker(string broker);
std::string brokers_as_comma_list() const;
uri::URI broker_config {"//localhost:9092/forward_epics_to_kafka_commands"};
vector<uri::URI> brokers;
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

void init_logger();
friend class ::MainOpt_T;
};


std::pair<int, std::unique_ptr<MainOpt>> parse_opt(int argc, char ** argv);

}
}
