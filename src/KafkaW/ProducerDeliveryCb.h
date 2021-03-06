// SPDX-License-Identifier: BSD-2-Clause
//
// This code has been produced by the European Spallation Source
// and its partner institutes under the BSD 2 Clause License.
//
// See LICENSE.md at the top level for license information.
//
// Screaming Udder!                              https://esss.se

#pragma once

#include "../logger.h"
#include "Producer.h"
#include "ProducerMessage.h"
#include "ProducerStats.h"
#include <librdkafka/rdkafkacpp.h>
#include <utility>

namespace KafkaW {

class ProducerDeliveryCb : public RdKafka::DeliveryReportCb {
public:
  explicit ProducerDeliveryCb(ProducerStats &Stats) : Stats(Stats){};

  void dr_cb(RdKafka::Message &Message) override {
    if (Message.err()) {
      Logger->error("ERROR on delivery, topic {}, {} [{}] {}",
                    Message.topic_name(), Message.err(), Message.errstr(),
                    RdKafka::err2str(Message.err()));
      ++Stats.produce_cb_fail;
    } else {
      ++Stats.produce_cb;
    }
    // When produce was called, we gave RdKafka a pointer to our message object
    // This is returned to us here via Message.msg_opaque() so that we can now
    // clean it up
    delete reinterpret_cast<ProducerMessage *>(Message.msg_opaque());
  }

private:
  ProducerStats &Stats;
  SharedLogger Logger = getLogger();
};
} // namespace KafkaW
