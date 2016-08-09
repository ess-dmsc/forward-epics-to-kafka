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
#include <vector>
#include <forward_list>
#include <string>

#include "tools.h"

#include <librdkafka/rdkafka.h>


namespace BrightnESS {
namespace ForwardEpicsToKafka {
namespace Kafka {

template <typename T> using sptr = std::shared_ptr<T>;

class Instance;

class Topic {
public:
Topic(sptr<Instance> ins, std::string topic_name);
~Topic();

void produce(BufRange buf);

// Should make a friend method out of this..
void error_from_kafka_callback();

bool healthy();
std::string & topic_name();

std::atomic_bool failure {false};

private:
sptr<Instance> ins;
// Used by Producer
rd_kafka_topic_t * rkt = 0;
friend class Producer;
std::string topic_name_;
};


class Instance {
public:
static sptr<Instance> create();

int load();

// Public because we want to delete from shared pointer
~Instance();

// Kafka handle, used by Topic.  Solve differently?
rd_kafka_t * rk = 0;

// Invoked from callback.  Should make it a friend.
void error_from_kafka_callback();

sptr<Topic> get_or_create_topic(std::string topic_name);

void check_topic_health();
std::atomic_bool error_from_kafka_callback_flag {false};

std::vector<std::weak_ptr<Topic>> topics;

bool instance_failure();

private:
// Should prevent all default five
Instance(Instance const &&) = delete;
Instance();
void init();

std::weak_ptr<Instance> self;

char const * brokers = "localhost:9092";

void poll_start();
void poll_run();
void poll_stop();
std::thread poll_thread;
std::atomic_bool do_poll {false};
std::atomic_bool ready_kafka {false};
std::atomic_bool m_instance_failure {false};
std::atomic_int id {0};
};


class InstanceSet {
public:
static InstanceSet & Set();
sptr<Instance> instance();

std::forward_list<sptr<Instance>> instances;

private:
// Prevent ctor
InstanceSet(InstanceSet const &&) = delete;
InstanceSet();
};


typedef Instance KafkaOpaqueType;


}
}
}
