#pragma once

/**
\file
Manage the running Kafka producer instances.
Simple load balance over the available producers.
*/

#include <memory>
#include <atomic>
#include <thread>
#include <vector>
#include <map>
#include <forward_list>
#include <string>

#include "tools.h"

#include <librdkafka/rdkafka.h>
#include "fbhelper.h"
#include "fbschemas.h"
#include "KafkaW.h"


namespace BrightnESS {
namespace ForwardEpicsToKafka {
namespace Kafka {

template <typename T> using sptr = std::shared_ptr<T>;

class Instance;

class Topic {
public:
Topic(sptr<Instance> ins, std::string topic_name, int id);
~Topic();

void produce(BrightnESS::FlatBufs::FB_uptr fb);

// Should make a friend method out of this..
void error_from_kafka_callback();

bool healthy();
std::string & topic_name();

std::atomic_bool failure {false};

private:
sptr<Instance> ins;
friend class Producer;
std::string topic_name_;

public:
// For testing:
int id = -1;

private:
KafkaW::Producer::Topic topic;
};


class Instance {
public:
static sptr<Instance> create(KafkaW::BrokerOpt opt);
~Instance();
int load();

// Invoked from callback.  Should make it a friend.
void error_from_kafka_callback();

sptr<Topic> get_or_create_topic(std::string topic_name, int id);

void check_topic_health();
std::atomic_bool error_from_kafka_callback_flag {false};

std::vector<std::weak_ptr<Topic>> topics;

bool instance_failure();

private:
Instance(Instance const &&) = delete;
Instance(KafkaW::BrokerOpt opt);
void init();

KafkaW::BrokerOpt opt;
public:
KafkaW::Producer producer;
private:
std::weak_ptr<Instance> self;

void poll_start();
void poll_run();
void poll_stop();
std::thread poll_thread;
std::atomic_bool do_poll {false};
std::atomic_bool ready_kafka {false};
std::atomic_int id {0};
};


class InstanceSet {
public:
static InstanceSet & Set(KafkaW::BrokerOpt opt);
sptr<Instance> instance();

std::forward_list<sptr<Instance>> instances;

private:
InstanceSet(InstanceSet const &&) = delete;
InstanceSet(KafkaW::BrokerOpt opt);
KafkaW::BrokerOpt opt;
};


}
}
}
