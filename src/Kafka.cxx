#include "Kafka.h"
#include "logger.h"
#include "local_config.h"

#include <map>
#include <algorithm>
#include <functional>
#include <mutex>
#include <atomic>

namespace BrightnESS {
namespace ForwardEpicsToKafka {
namespace Kafka {

sptr<InstanceSet> InstanceSet::Set(KafkaW::BrokerOpt opt) {
	static std::mutex mx;
	std::unique_lock<std::mutex> lock(mx);
	LOG(4, "Kafka InstanceSet with rdkafka version: {}", rd_kafka_version_str());
	static std::shared_ptr<InstanceSet> kset;
	if (!kset) {
		opt.poll_timeout_ms = 0;
		kset.reset(new InstanceSet(opt));
	}
	return kset;
}

InstanceSet::InstanceSet(KafkaW::BrokerOpt opt) : opt(opt) {
}

static void prod_delivery_ok(rd_kafka_message_t const * msg) {
	if (auto x = msg->_private) {
		auto p = static_cast<KafkaW::Producer::Msg*>(x);
		p->delivery_ok();
		delete p;
	}
}

static void prod_delivery_failed(rd_kafka_message_t const * msg) {
	if (auto x = msg->_private) {
		auto p = static_cast<KafkaW::Producer::Msg*>(x);
		p->delivery_fail();
		// TODO really delete or just re-use?
		// but maybe we want to add information about the failure first?
		delete p;
	}
}


KafkaW::Producer::Topic InstanceSet::producer_topic(uri::URI uri) {
	LOG(7, "InstanceSet::producer_topic  for:  {}, {}", uri.host_port, uri.topic);
	auto host_port = uri.host_port;
	auto it = producers_by_host.find(host_port);
	if (it != producers_by_host.end()) {
		return KafkaW::Producer::Topic(it->second, uri.topic);
	}
	auto bopt = opt;
	bopt.address = host_port;
	auto p = std::make_shared<KafkaW::Producer>(bopt);
	p->on_delivery_ok = prod_delivery_ok;
	p->on_delivery_failed = prod_delivery_failed;
	{
		std::unique_lock<std::mutex> lock(mx_producers_by_host);
		producers_by_host[host_port] = p;
	}
	return KafkaW::Producer::Topic(p, uri.topic);
}


int InstanceSet::poll() {
	std::unique_lock<std::mutex> lock(mx_producers_by_host);
	for (auto m : producers_by_host) {
		auto & p = m.second;
		p->poll();
		LOG(6, "Broker: {}  total: {}  outq: {}", m.first, p->total_produced(), p->outq());
	}
	return 0;
}


}
}
}
