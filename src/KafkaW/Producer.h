#pragma once

#include "BrokerSettings.h"
#include "ConsumerMessage.h"
#include "ProducerStats.h"
#include <atomic>
#include <functional>
#include <librdkafka/rdkafkacpp.h>

namespace KafkaW {

class ProducerTopic;

class ProducerInterface {
public:
  ProducerInterface() = default;
  virtual ~ProducerInterface() = default;

  virtual void poll() = 0;

  virtual uint64_t outputQueueLength() = 0;
  virtual rd_kafka_s *getRdKafkaPtr() const = 0;
};

class Producer : public ProducerInterface {
public:
  typedef ProducerTopic Topic;
  explicit Producer(BrokerSettings ProducerBrokerSettings_);
  Producer(Producer const &) = delete;
  Producer(Producer &&x) noexcept;
  ~Producer() override;
  void poll() override;
  uint64_t outputQueueLength() override;

  /// The message delivered callback for Kafka.
  ///
  /// \param RK The Kafka handle.
  /// \param Message The message
  /// \param Opaque The opaque object.
  static void deliveredCallback(rd_kafka_t *RK,
                                rd_kafka_message_t const *Message,
                                void *Opaque);

  /// The error callback for Kafka.
  ///
  /// \param RK The Kafka handle.
  /// \param Err_i The error code.
  /// \param Message The error string.
  /// \param Opaque The opaque object.
  static void errorCallback(rd_kafka_t *RK, int Err_i, char const *Message,
                            void *Opaque);

  /// The statistics callback for Kafka.
  ///
  /// \param RK The Kafka handle.
  /// \param Json The statistics data in JSON format.
  /// \param json_size The size of the JSON string.
  /// \param Opaque The opaque.
  /// \return The error code.
  static int statsCallback(rd_kafka_t *RK, char *Json, size_t Json_len,
                           void *Opaque);

  /// The log callback for Kafka.
  ///
  /// \param RK The Kafka handle.
  /// \param Level The log level.
  /// \param Fac ?
  /// \param Buf The message buffer.
  static void logCallback(rd_kafka_t const *RK, int Level, char const *Fac,
                          char const *Buf);

  /// The throttle callback for Kafka.
  ///
  /// \param RK The Kafka handle.
  /// \param Name The broker name.
  /// \param Broker_id  The broker id.
  /// \param Throttle_time_ms The throttle time in milliseconds.
  /// \param Opaque The opaque.
  static void throttleCallback(rd_kafka_t *RK, char const *Name,
                               int32_t Broker_id, int Throttle_time_ms,
                               void *Opaque);
  rd_kafka_t *getRdKafkaPtr() const override;
  std::function<void(rd_kafka_message_t const *msg)> on_delivery_ok;
  std::function<void(rd_kafka_message_t const *msg)> on_delivery_failed;
  std::function<void(ProducerInterface *, rd_kafka_resp_err_t)> on_error;
  // Currently it's nice to have access to these two for statistics:
  BrokerSettings ProducerBrokerSettings;
  rd_kafka_t *RdKafkaPtr = nullptr;
  std::atomic<uint64_t> TotalMessagesProduced{0};
  ProducerStats Stats;

private:
  int id = 0;
};
} // namespace KafkaW
