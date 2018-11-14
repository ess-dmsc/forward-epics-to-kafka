#pragma once

#include "BrokerSettings.h"
#include "Msg.h"
#include "PollStatus.h"
#include <functional>
#include <librdkafka/rdkafka.h>

namespace KafkaW {

class Inspect;

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
  /// \param rk The Kafka handle.
  /// \param level The log level.
  /// \param fac ?
  /// \param buf The message buffer.
  static void logCallback(rd_kafka_t const *rk, int level, char const *fac,
                          char const *buf);

  /// The statistics callback for Kafka.
  ///
  /// \param rk The Kafka handle.
  /// \param json The statistics data in JSON format.
  /// \param json_size The size of the JSON string.
  /// \param opaque The opaque.
  /// \return The error code.
  static int statsCallback(rd_kafka_t *rk, char *json, size_t json_size,
                           void *opaque);

  /// Error callback method for Kafka.
  ///
  /// \param rk The Kafka handle.
  /// \param err_i The error code.
  /// \param reason The error string.
  /// \param opaque The opaque object.
  static void errorCallback(rd_kafka_t *rk, int err_i, char const *reason,
                            void *opaque);

  /// The rebalance callback for Kafka.
  ///
  /// \param rk The Kafka handle.
  /// \param err The error response.
  /// \param plist The partition list.
  /// \param opaque The opaque object.
  static void rebalanceCallback(rd_kafka_t *rk, rd_kafka_resp_err_t err,
                                rd_kafka_topic_partition_list_t *plist,
                                void *opaque);
  rd_kafka_topic_partition_list_t *PartitionList = nullptr;
  int id = 0;
};
} // namespace KafkaW
