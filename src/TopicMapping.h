#pragma once

#include <memory>
#include <condition_variable>
#include <string>
#include <chrono>

#include "Kafka.h"

namespace BrightnESS {
namespace ForwardEpicsToKafka {

// Forward declare class
namespace Epics {
class Monitor;
}

namespace Kafka {
class Topic;
}



class TopicMappingSettings {
public:
TopicMappingSettings(std::string channel, std::string topic)
:	channel(channel),
	topic(topic)
{ }

std::string channel;
std::string topic;
};


/** \brief
Represents the mapping between a EPICS process variable and a Kafka topic.
*/

class TopicMapping {
public:
typedef std::string string;
enum class State { INIT, READY, FAILURE };

//TopicMapping(TopicMapping &&) = default;

/// Defines a mapping, but does not yet start the forwarding
TopicMapping(Kafka::InstanceSet & kset, TopicMappingSettings topic_mapping_settings, uint32_t id);
~TopicMapping();

void start_forwarding(Kafka::InstanceSet & kset);
void stop_forwarding();

void emit(double x);

/** Called from watchdog thread, opportunity to check own health status */
void health_selfcheck();
bool healthy() const;

string topic_name() const;
string channel_name() const;

State health_state() const;

/** can be called from any thread */
void go_into_failure_mode();

// for debugging:
uint32_t id;

/// Should return true if we waited long enough so that this zombie can be cleaned up
bool zombie_can_be_cleaned();

private:
TopicMappingSettings topic_mapping_settings;
std::weak_ptr<Kafka::Topic> topic;
std::condition_variable cv1;
std::mutex mu1;
std::unique_ptr<Epics::Monitor> epics_monitor;
//std::atomic<State> state {State::INIT};
std::chrono::system_clock::time_point ts_init;
std::chrono::system_clock::time_point ts_failure;
State state {State::INIT};
};

}
}
