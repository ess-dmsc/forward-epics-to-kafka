#include "Config.h"
#include "logger.h"

namespace BrightnESS {
namespace ForwardEpicsToKafka {
namespace Config {

struct Listener_impl {
KafkaW::Consumer consumer;
};


Listener::Listener(KafkaW::BrokerOpt bopt, uri::URI uri) {
	bopt.address = uri.host_port;
	impl.reset(new Listener_impl {bopt});
	impl->consumer.add_topic(uri.topic);
}


Listener::~Listener() {
}


void Listener::poll(Callback & cb) {
	for (int i1 = 0; i1 < 10; ++i1) {
		if (auto m = impl->consumer.poll().is_Msg()) {
			LOG(0, "CONFIG MESSAGE");
			cb({(char*)m->data(), m->size()});
		}
	}
}


}
}
}
