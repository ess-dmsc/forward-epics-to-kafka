#include "Config.h"
#include "logger.h"
#include <memory>
#include <mutex>
#include <condition_variable>

namespace BrightnESS {
namespace ForwardEpicsToKafka {
namespace Config {

struct Listener_impl {
std::unique_ptr<KafkaW::Consumer> consumer;
std::mutex mx;
std::condition_variable cv;
int connected = 0;
};

Listener::Listener(KafkaW::BrokerOpt bopt, uri::URI uri) {
	bopt.address = uri.host_port;
	impl.reset(new Listener_impl);
	impl->consumer.reset(new KafkaW::Consumer(bopt));
	auto & consumer = *impl->consumer;
	consumer.on_rebalance_assign = [this](rd_kafka_topic_partition_list_t * plist) {
		{
			std::unique_lock<std::mutex> lock(impl->mx);
			impl->connected = 1;
		}
		impl->cv.notify_all();
	};
	consumer.on_rebalance_assign = {};
	consumer.on_rebalance_start = {};
	consumer.add_topic(uri.topic);
}


Listener::~Listener() {
}


void Listener::poll(Callback & cb) {
	if (auto m = impl->consumer->poll().is_Msg()) {
		cb({(char*)m->data(), m->size()});
	}
}

void Listener::wait_for_connected(std::chrono::milliseconds timeout) {
	std::unique_lock<std::mutex> lock(impl->mx);
	impl->cv.wait_for(lock, timeout, [this]{return impl->connected == 1;});
}


}
}
}
