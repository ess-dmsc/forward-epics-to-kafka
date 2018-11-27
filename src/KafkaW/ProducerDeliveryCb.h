#pragma once
#include <librdkafka/rdkafkacpp.h>

class ProducerDeliveryCb : public RdKafka::DeliveryReportCb {
public:
  ProducerDeliveryCb(std::function<> onDeliveryOk, std::function<> onDeliveryFailed)
  ProducerDeliveryCb() = default;
  void dr_cb(RdKafka::Message &message) override {
    switch (message.status()) {
    case RdKafka::Message::MSG_STATUS_NOT_PERSISTED:
      // failed
      break;
    case RdKafka::Message::MSG_STATUS_POSSIBLY_PERSISTED:
      break;
    case RdKafka::Message::MSG_STATUS_PERSISTED:
      // passed
      break;
    default:
      break;
    }
  }
};