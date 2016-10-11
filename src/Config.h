#pragma once

#include <memory>
#include <atomic>
#include <thread>
#include <vector>
#include <vector>
#include <forward_list>
#include <string>

#include "tools.h"

#include <librdkafka/rdkafka.h>


namespace BrightnESS {
namespace ForwardEpicsToKafka {
namespace Config {


// Interfaces with the new lib
class Service {
public:
Service();
};



struct KafkaSettings {
KafkaSettings(std::string brokers, std::string topic);
std::string brokers = "localhost:9092";
std::string topic = "configuration.global";
};

/** React on configuration messages */
class Callback {
public:
virtual void operator() (std::string const & msg) = 0;
};

class Listener {
public:
Listener(KafkaSettings settings);
~Listener();
void poll(Callback & cb);
void kafka_connection_information();
private:
rd_kafka_t * rk = nullptr;
//rd_kafka_topic_t * rkt = nullptr;
rd_kafka_topic_partition_list_t * plist = nullptr;
};

}
}
}
