#include "Stream.h"
#include "Converter.h"
#include "KafkaOutput.h"
#include "logger.h"
#include "EpicsClient.h"
#include "epics-to-fb.h"
#include "epics-pvstr.h"
#include "helper.h"

namespace BrightnESS {
namespace ForwardEpicsToKafka {

ConversionPath::ConversionPath(ConversionPath && x) : converter(std::move(x.converter)), kafka_output(std::move(x.kafka_output)) {
}

ConversionPath::ConversionPath(std::shared_ptr<Converter> conv, std::unique_ptr<KafkaOutput> ko) :
		converter(conv),
		kafka_output(std::move(ko))
{ }

ConversionPath::~ConversionPath() {
	LOG(7, "~ConversionPath");
	while (true) {
		auto x = transit.load();
		if (x == 0) break;
		CLOG(7, 1, "~ConversionPath  still has transit {}", transit);
		sleep_ms(1000);
	}
}

int ConversionPath::emit(std::unique_ptr<FlatBufs::EpicsPVUpdate> up) {
	auto fb = converter->convert(*up);
	if (fb == nullptr) {
		CLOG(8, 1, "empty converted flat buffer");
		return 1;
	}
	kafka_output->emit(std::move(fb));
	return 0;
}


static uint16_t _fmt(std::unique_ptr<FlatBufs::EpicsPVUpdate> & x) {
	return (uint16_t)(((uint64_t)x.get())>>0);
}

Stream::Stream(std::shared_ptr<ForwarderInfo> finfo, ChannelInfo channel_info) :
		channel_info(channel_info),
		epics_client(new EpicsClient::EpicsClient(this, finfo, channel_info.channel_name))
{
	emit_queue.formatter = _fmt;
}

Stream::~Stream() {
	CLOG(7, 2, "~Stream");
	stop();
	CLOG(7, 2, "~Stop DONE");
}

int Stream::converter_add(Kafka::InstanceSet & kset, FlatBufs::SchemaRegistry const & schema_registry, std::string schema, uri::URI uri_kafka_output) {
	auto conv = Converter::create(schema_registry, schema);
	auto pt = kset.producer_topic(uri_kafka_output);
	std::unique_ptr<ConversionPath> cp(new ConversionPath( {std::move(conv)}, std::unique_ptr<KafkaOutput>(new KafkaOutput(std::move(pt))) ));
	conversion_paths.push_back(std::move(cp));
	return 0;
}

int Stream::emit(std::unique_ptr<FlatBufs::EpicsPVUpdate> up) {
	CLOG(9, 7, "Stream::emit");
	if (!up) {
		CLOG(7, 1, "empty update?");
		// should never happen, ignore
		return 0;
	}
	if (true) {
		for (int i1 = 0; i1 < 256; ++i1) {
			auto x = emit_queue.push(up);
			if (x == 0) break;
			//sleep_ms(1);
			//if (i1 == 20) {
			{
				//CLOG(9, 1, "buffer full {} times", i1);
				emit_queue.push_enlarge(up);
				break;
			}
		}
		if (up) {
			// here we are, saying goodbye to a good buffer
			up.reset();
			//LOG(4, "loosing buffer");
		}
		//auto s1 = emit_queue.to_vec();
		//LOG(9, "Queue {}\n{}", channel_info.channel_name, s1.data());
	}
	else {
		// Emit directly
		//LOG(9, "Stream::emit  convs: {}", conversion_paths.size());
		for (auto & cp : conversion_paths) {
			cp->emit(std::move(up));
		}
	}
	return 0;
}

int32_t Stream::fill_conversion_work(Ring<std::unique_ptr<ConversionWorkPacket>> & q2, uint32_t max) {
	auto & q1 = emit_queue;
	ulock(q1.mx);
	ulock(q2.mx);
	uint32_t n0 = 0;
	uint32_t n1 = 0;
	uint32_t n2 = q1.size_unsafe();
	uint32_t n3 = std::min(max, q2.capacity_unsafe() - q2.size_unsafe());
	uint32_t ncp = conversion_paths.size();
	std::vector<ConversionWorkPacket*> cwp_last(conversion_paths.size());
	//LOG(8, "Stream::fill_conversion_work {}  {}  {}", n1, n2, n3);
	while (n0 < n2 && n3-n1 >= ncp) {
		//LOG(8, "Stream::fill_conversion_work  loop   {}  {}  {}", n1, n2, n3);
		auto e = q1.pop_unsafe();
		n0 += 1;
		if (e.first != 0) {
			CLOG(8, 1, "empty? should not happen");
			break;
		}
		auto & up = e.second;
		if (!up) {
			LOG(8, "empty epics update");
			continue;
		}
		size_t cpid = 0;
		uint32_t ncp = conversion_paths.size();
		for (auto & cp : conversion_paths) {
			auto p = std::unique_ptr<ConversionWorkPacket>(new ConversionWorkPacket);
			cwp_last[cpid] = p.get();
			p->cp = cp.get();
			if (ncp == 1) {
				// more common case
				p->up = std::move(e.second);
			}
			else {
				p->up = std::unique_ptr<FlatBufs::EpicsPVUpdate>(new FlatBufs::EpicsPVUpdate(*e.second));
			}
			auto x = q2.push_unsafe(p);
			if (x != 0) {
				//CLOG(9, 1, "full? should not happen");
				break;
			}
			cpid += 1;
			n1 += 1;
		}
	}
	if (n1 > 0) {
		for (uint32_t i1 = 0; i1 < cwp_last.size(); ++i1) {
			cwp_last[i1]->stream = this;
			conversion_paths[i1]->transit++;
		}
	}
	return n1;
}

int Stream::stop() {
	epics_client->stop();
	return 0;
}

void Stream::teamid(uint64_t teamid) {
	teamid_ = teamid;
}

}
}
