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


namespace BrightnESS {
namespace ForwardEpicsToKafka {
namespace Kafka {

template <typename T> using sptr = std::shared_ptr<T>;

class Instance;

class Topic {
public:
Topic(sptr<Instance> ins, std::string topic_name, int id);
~Topic();

void produce(BrightnESS::FlatBufs::FB_uptr fb, uint64_t seq, uint64_t ts);

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

public:
// For testing:
int id = -1;
};


class Instance {
public:
static sptr<Instance> create(std::string brokers, std::map<std::string, int> conf_ints);

int load();

// Public because we want to delete from shared pointer
~Instance();

// Kafka handle, used by Topic.  Solve differently?
rd_kafka_t * rk = 0;

// Invoked from callback.  Should make it a friend.
void error_from_kafka_callback();

sptr<Topic> get_or_create_topic(std::string topic_name, int id);

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

std::string brokers = "localhost:9092";

void poll_start();
void poll_run();
void poll_stop();
std::thread poll_thread;
std::atomic_bool do_poll {false};
std::atomic_bool ready_kafka {false};
std::atomic_int id {0};
std::map<std::string, int> conf_ints;
static void cb_delivered(rd_kafka_t * rk, rd_kafka_message_t const * msg, void * opaque);
static void cb_error(rd_kafka_t * rk, int err_i, char const * msg, void * opaque);
static void cb_log(rd_kafka_t const * rk, int level, char const * fac, char const * buf);
static int cb_stats(rd_kafka_t * rk, char * json, size_t json_len, void * opaque);
};


class InstanceSet {
public:
static InstanceSet & Set(std::string brokers, std::map<std::string, int> conf_ints);
sptr<Instance> instance();

std::forward_list<sptr<Instance>> instances;

private:
// Prevent ctor
InstanceSet(InstanceSet const &&) = delete;
InstanceSet(std::string brokers, std::map<std::string, int> conf_ints);
std::string brokers;
std::map<std::string, int> conf_ints;
};


}
}
}
