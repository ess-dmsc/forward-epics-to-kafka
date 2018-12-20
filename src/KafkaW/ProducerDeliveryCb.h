#pragma once

#include "Producer.h"
#include "ProducerMessage.h"
#include "ProducerStats.h"
#include "logger.h"
#include <librdkafka/rdkafkacpp.h>
#include <utility>

namespace KafkaW {

class ProducerDeliveryCb : public RdKafka::DeliveryReportCb {
public:
  explicit ProducerDeliveryCb(ProducerStats &Stats) : Stats(Stats){};

  void dr_cb(RdKafka::Message &Message) override {
    if (Message.err()) {
      LOG(Sev::Error, "ERROR on delivery, topic {}, {} [{}] {}",
          Message.topic_name(), Message.err(), Message.errstr(),
          RdKafka::err2str(Message.err()));
      ++Stats.produce_cb_fail;
    } else {
      ++Stats.produce_cb;
    }
    // When produce was called, we gave RdKafka a pointer to our message object
    // This is returned to us here via Message.msg_opaque() so that we can now
    // clean it up
    auto MessageToBeCleanedUp = std::unique_ptr<ProducerMessage>(
        reinterpret_cast<ProducerMessage *>(Message.msg_opaque()));
  }

private:
  ProducerStats Stats;
};
}
