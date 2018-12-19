#include "ProducerTopic.h"
#include <vector>

namespace KafkaW {

using std::unique_ptr;
using std::shared_ptr;
using std::array;
using std::vector;
using std::string;
using std::atomic;
using std::move;

ProducerTopic::ProducerTopic(std::shared_ptr<Producer> Producer,
                             std::string Name_)
    : Producer_(Producer), Name(std::move(Name_)) {

  std::string ErrStr;
  auto Config = RdKafka::Conf::create(RdKafka::Conf::CONF_TOPIC);
  RdKafkaTopic =
      RdKafka::Topic::create(Producer_->getRdKafkaPtr(), Name, Config, ErrStr);
  if (RdKafkaTopic == nullptr) {
    LOG(Sev::Error, "could not create Kafka topic: {}", ErrStr);
    throw TopicCreationError();
  }

  LOG(Sev::Debug, "ctor topic: {}", RdKafkaTopic->name());
}

ProducerTopic::ProducerTopic(ProducerTopic &&x) noexcept {
  std::swap(Producer_, x.Producer_);
  std::swap(RdKafkaTopic, x.RdKafkaTopic);
  std::swap(Name, x.Name);
  std::swap(DoCopyMsg, x.DoCopyMsg);
}

struct Msg_ : public ProducerMessage {
  vector<unsigned char> v;
  void finalize() {
    data = v.data();
    size = v.size();
  }
};

int ProducerTopic::produce(unsigned char *MsgData, size_t MsgSize) {
  auto MsgPtr = new Msg_;
  std::copy(MsgData, MsgData + MsgSize, std::back_inserter(MsgPtr->v));
  MsgPtr->finalize();
  unique_ptr<ProducerMessage> Msg(MsgPtr);
  return produce(Msg);
}

int ProducerTopic::produce(unique_ptr<ProducerMessage> &Msg) {
  void const *key = nullptr;
  size_t key_len = 0;
  int msgflags = 0; // 0, RD_KAFKA_MSG_F_COPY, RD_KAFKA_MSG_F_FREE

  auto &ProducerStats = Producer_->Stats;

  switch (this->Producer_->getRdKafkaPtr()->produce(
      RdKafkaTopic, RdKafka::Topic::PARTITION_UA, msgflags, Msg->data,
      Msg->size, key, key_len, Msg.get())) {
  case RdKafka::ERR_NO_ERROR:
    ++ProducerStats.produced;
    ProducerStats.produced_bytes += (uint64_t)Msg->size;
    ++Producer_->TotalMessagesProduced;
    Msg.release();
    return 0;

  case RdKafka::ERR__QUEUE_FULL:
    ++ProducerStats.local_queue_full;
    LOG(Sev::Warning, "Producer queue full, outq: {}",
        Producer_->getRdKafkaPtr()->outq_len());
    break;

  case RdKafka::ERR_MSG_SIZE_TOO_LARGE:
    ++ProducerStats.msg_too_large;
    LOG(Sev::Error, "Message size too large to publish, size: {}", Msg->size);
    break;

  default:
    ++ProducerStats.produce_fail;
    LOG(Sev::Error, "Publishing message on topic \"{}\" failed", RdKafkaTopic->name());
    break;
  }
  return 1;
}

void ProducerTopic::enableCopy() { DoCopyMsg = true; }

std::string ProducerTopic::name() const { return Name; }
}
