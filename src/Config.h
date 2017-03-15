#pragma once

#include <memory>
#include <atomic>
#include <vector>
#include <string>
#include "uri.h"
#include "KafkaW.h"

namespace BrightnESS {
namespace ForwardEpicsToKafka {
namespace Config {

using std::string;

/** Interface to react on configuration messages */
class Callback {
public:
virtual void operator() (string const & msg) = 0;
};

struct Listener_impl;

class Listener {
public:
Listener(KafkaW::BrokerOpt bopt, uri::URI uri);
~Listener();
void poll(Callback & cb);
private:
std::unique_ptr<Listener_impl> impl;
};

}
}
}
