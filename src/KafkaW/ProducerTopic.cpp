// SPDX-License-Identifier: BSD-2-Clause
//
// This code has been produced by the European Spallation Source
// and its partner institutes under the BSD 2 Clause License.
//
// See LICENSE.md at the top level for license information.
//
// Screaming Udder!                              https://esss.se

#include "ProducerTopic.h"
#include <vector>

namespace KafkaW {

ProducerTopic::ProducerTopic(std::shared_ptr<Producer> ProducerPtr,
                             std::string TopicName)
    : KafkaProducer(ProducerPtr), Name(std::move(TopicName)) {

  std::string ErrStr;
  RdKafkaTopic = KafkaProducer->createTopic(Name, ErrStr);
  if (RdKafkaTopic == nullptr) {
    Logger->error("could not create Kafka topic: {}", ErrStr);
    throw TopicCreationError();
  }
  Logger->trace("Constructor topic: {}", RdKafkaTopic->name());
}

ProducerTopic::ProducerTopic(ProducerTopic &&x) noexcept {
  std::swap(KafkaProducer, x.KafkaProducer);
  std::swap(RdKafkaTopic, x.RdKafkaTopic);
  std::swap(Name, x.Name);
  std::swap(DoCopyMsg, x.DoCopyMsg);
}

struct Msg_ : public ProducerMessage {
  std::vector<unsigned char> v;
  void finalize() {
    Data = v.data();
    Size = v.size();
  }
};

/// NB this copies the provided data - so use only for low volume publishing
/// \param MsgData Pointer to the data to publish
/// \param MsgSize Size of the data to publish
/// \return 0 if message is successfully passed to RdKafka to be published, 1
/// otherwise
int ProducerTopic::produce(unsigned char *MsgData, size_t MsgSize) {
  auto MsgPtr = new Msg_;
  std::copy(MsgData, MsgData + MsgSize, std::back_inserter(MsgPtr->v));
  MsgPtr->finalize();
  std::unique_ptr<ProducerMessage> Msg(MsgPtr);
  return produce(Msg);
}

int ProducerTopic::produceAndSetKey(unsigned char *MsgData, size_t MsgSize,
                                    const std::string &Key) {
  auto MsgPtr = new Msg_;
  std::copy(MsgData, MsgData + MsgSize, std::back_inserter(MsgPtr->v));
  MsgPtr->finalize();
  std::unique_ptr<ProducerMessage> Msg(MsgPtr);
  Msg->Key = Key;
  return produce(Msg);
}

int ProducerTopic::produce(std::unique_ptr<ProducerMessage> &Msg) {
  void const *key = nullptr;
  size_t key_len = 0;
  // MsgFlags = 0 means that we are responsible for cleaning up the message
  // after it has been sent
  // We do this by providing a pointer to our message object in the produce
  // call, this pointer is returned to us in the delivery callback, at which
  // point we can free the memory
  int MsgFlags = 0;
  auto &ProducerStats = KafkaProducer->Stats;

  if (!Msg->Key.empty()) {
    key = Msg->Key.c_str();
    key_len = Msg->Key.size();
  }

  switch (KafkaProducer->produce(
      RdKafkaTopic.get(), RdKafka::Topic::PARTITION_UA, MsgFlags, Msg->Data,
      Msg->Size, key, key_len, Msg.get())) {
  case RdKafka::ERR_NO_ERROR:
    ++ProducerStats.produced;
    ProducerStats.produced_bytes += static_cast<uint64_t>(Msg->Size);
    ++KafkaProducer->TotalMessagesProduced;
    Msg.release(); // we clean up the message after it has been sent, see
                   // comment by MsgFlags declaration
    return 0;

  case RdKafka::ERR__QUEUE_FULL:
    ++ProducerStats.local_queue_full;
    Logger->warn("Producer queue full, outq: {}",
                 KafkaProducer->outputQueueLength());
    break;

  case RdKafka::ERR_MSG_SIZE_TOO_LARGE:
    ++ProducerStats.msg_too_large;
    Logger->error("Message size too large to publish, size: {}", Msg->Size);
    break;

  default:
    ++ProducerStats.produce_fail;
    Logger->error("Publishing message on topic \"{}\" failed",
                  RdKafkaTopic->name());
    break;
  }
  return 1;
}

std::string ProducerTopic::name() const { return Name; }

std::string ProducerTopic::brokerAddress() const {
  return KafkaProducer->ProducerBrokerSettings.Address;
}
} // namespace KafkaW
