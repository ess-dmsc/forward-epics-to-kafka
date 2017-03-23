#include "KafkaOutput.h"
#include "Main.h"
#include "logger.h"

namespace BrightnESS {
namespace ForwardEpicsToKafka {

KafkaOutput::KafkaOutput(KafkaOutput && x) : pt(std::move(x.pt)) {
}

KafkaOutput::KafkaOutput(KafkaW::Producer::Topic && pt) : pt(std::move(pt)) {
}

int KafkaOutput::emit(std::unique_ptr<BrightnESS::FlatBufs::FB> fb) {
	if (!fb) {
		CLOG(8, 1, "KafkaOutput::emit  empty fb");
		return -1024;
	}
	auto m1 = fb->message();
	fb->data = m1.data;
	fb->size = m1.size;
	auto x = pt.produce(*fb.get());
	if (x == 0) {
		fb.release();
		++g__total_msgs_to_kafka;
		g__total_bytes_to_kafka += m1.size;
	}
	return x;
}

}
}
