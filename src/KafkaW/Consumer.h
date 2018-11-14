#pragma once

#include "BrokerSettings.h"
#include "Msg.h"
#include "PollStatus.h"
#include <functional>
#include <librdkafka/rdkafka.h>

namespace KafkaW {

class ConsumerInterface {
public:
  ConsumerInterface() = default;
  virtual ~ConsumerInterface() = default;
  virtual void addTopic(std::string Topic) = 0;
  virtual void dumpCurrentSubscription() = 0;
  virtual PollStatus poll() = 0;
};

class Consumer : public ConsumerInterface {
public:
  explicit Consumer(BrokerSettings opt);
  Consumer(Consumer &&) = delete;
  Consumer(Consumer const &) = delete;
  ~Consumer();
  void init();
  void addTopic(std::string Topic) override;
  void dumpCurrentSubscription() override;
  PollStatus poll() override;
  std::function<void(rd_kafka_topic_partition_list_t *plist)>
      on_rebalance_assign;
  std::function<void(rd_kafka_topic_partition_list_t *plist)>
      on_rebalance_start;
  rd_kafka_t *RdKafka = nullptr;

private:
  BrokerSettings ConsumerBrokerSettings;

  /// The log callback for Kafka.
  ///
  /// \param RK The Kafka handle.
  /// \param Level The log level.
  /// \param Fac ?
  /// \param Buf The message buffer.
  static void logCallback(rd_kafka_t const *RK, int Level, char const *Fac,
                          char const *Buf);

  /// The statistics callback for Kafka.
  ///
  /// \param RK The Kafka handle.
  /// \param Json The statistics data in JSON format.
  /// \param Json_size The size of the JSON string.
  /// \param Opaque The opaque.
  /// \return The error code.
  static int statsCallback(rd_kafka_t *RK, char *Json, size_t Json_size,
                           void *Opaque);

  /// Error callback method for Kafka.
  ///
  /// \param RK The Kafka handle.
  /// \param Err_i The error code.
  /// \param Reason The error string.
  /// \param Opaque The opaque object.
  static void errorCallback(rd_kafka_t *RK, int Err_i, char const *Reason,
                            void *Opaque);

  /// The rebalance callback for Kafka.
  ///
  /// \param RK The Kafka handle.
  /// \param ERR The error response.
  /// \param PList The partition list.
  /// \param Opaque The opaque object.
  static void rebalanceCallback(rd_kafka_t *RK, rd_kafka_resp_err_t ERR,
                                rd_kafka_topic_partition_list_t *PList,
                                void *Opaque);
  rd_kafka_topic_partition_list_t *PartitionList = nullptr;
  int id = 0;
};
} // namespace KafkaW
