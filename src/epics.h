#pragma once

#include <atomic>
#include <mutex>
#include <string>

// EPICS v4 stuff
#include <pv/pvData.h>
#include <pv/pvAccess.h>

// For epics::pvAccess::ClientFactory::start()
#include <pv/clientFactory.h>

// EPICS 4 supports access via the channel access protocol as well,
// and we need it because some hardware speaks EPICS base.
#include <pv/caProvider.h>

#include "fbhelper.h"
#include "fbschemas.h"

//#include <pv/channelProviderLocal.h>
//#include <pv/caProvider.h>
//#include <pv/requester.h>

namespace BrightnESS {
namespace ForwardEpicsToKafka {

class TopicMapping;
}
}



namespace BrightnESS {
namespace ForwardEpicsToKafka {
namespace Epics {


class MonitorRequester;


class monitor_lost_exception : public std::exception {
};

class epics_channel_failure : public std::exception {
};

/** \brief
Monitor a channel which currently has to have a certain type and forward changes in value to a callable.

Needs the name of the channel and the callable.
If it detects something wrong, it tries to release all EPICS resources and throws.
*/
class Monitor {
public:
using wptr = std::weak_ptr<Monitor>;

/// Initiate the connection to the channel.  As far as EPICS docs tell, it should not block.
Monitor(TopicMapping * topic_mapping, std::string channel_name);
void init(std::shared_ptr<Monitor> self);
~Monitor();
bool ready();
void stop();

void go_into_failure_mode();

void topic_mapping_gone();

private:
friend class MonitorRequester;
friend class IntrospectField;
friend class StartMonitorChannel;
void emit(BrightnESS::FlatBufs::FB_uptr fb, uint64_t seq);

TopicMapping * topic_mapping;

std::string channel_name;
epics::pvAccess::ChannelProvider::shared_pointer provider;
epics::pvAccess::ChannelRequester::shared_pointer cr;
epics::pvAccess::Channel::shared_pointer ch;
void initiate_connection();
void initiate_value_monitoring();
epics::pvData::MonitorRequester::shared_pointer monr;
epics::pvData::Monitor::shared_pointer mon;

Monitor::wptr self;

std::atomic_bool ready_monitor {false};

std::atomic<int> failure_triggered {0};

std::recursive_mutex m_mutex_emitter;
using RMLG = std::lock_guard<std::recursive_mutex>;

};


}
}
}
