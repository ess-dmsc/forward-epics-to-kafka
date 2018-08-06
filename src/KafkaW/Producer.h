#pragma once

#include "BrokerSettings.h"
#include "Msg.h"
#include <atomic>
#include <functional>
#include <librdkafka/rdkafka.h>

namespace KafkaW {

class ProducerTopic;

class ProducerMsg {
public:
  virtual ~ProducerMsg() = default;
  virtual void deliveryOk();
  virtual void deliveryError();
  uchar *data;
  uint32_t size;
};

struct ProducerStats {
  std::atomic<uint64_t> produced{0};
  std::atomic<uint32_t> produce_fail{0};
  std::atomic<uint32_t> local_queue_full{0};
  std::atomic<uint64_t> produce_cb{0};
  std::atomic<uint64_t> produce_cb_fail{0};
  std::atomic<uint64_t> poll_served{0};
  std::atomic<uint64_t> msg_too_large{0};
  std::atomic<uint64_t> produced_bytes{0};
  std::atomic<uint32_t> out_queue{0};
  ProducerStats() = default;
  ProducerStats(ProducerStats const &);
};

class Producer {
public:
  typedef ProducerTopic Topic;
  typedef ProducerMsg Msg;
  Producer(BrokerSettings ProducerBrokerSettings_);
  Producer(Producer const &) = delete;
  Producer(Producer &&x);
  ~Producer();
  void pollWhileOutputQueueFilled();
  void poll();
  uint64_t totalMessagesProduced();
  uint64_t outputQueueLength();

  /// The message delivered callback for Kafka.
  ///
  /// \param rk The Kafka handle.
  /// \param msg The message
  /// \param opaque The opaque object.
  static void deliveredCallback(rd_kafka_t *rk, rd_kafka_message_t const *msg,
                                void *opaque);

  /// The error callback for Kafka.
  ///
  /// \param rk The Kafka handle (not used).
  /// \param err_i The error code.
  /// \param reason The error string.
  /// \param opaque The opaque object.
  static void errorCallback(rd_kafka_t *rk, int err_i, char const *reason,
                            void *opaque);

  /// The statistics callback for Kafka.
  ///
  /// \param rk The Kafka handle.
  /// \param json The statistics data in JSON format.
  /// \param json_size The size of the JSON string.
  /// \param opaque The opaque.
  /// \return The error code.
  static int statsCallback(rd_kafka_t *rk, char *json, size_t json_len,
                           void *opaque);

  /// The log callback for Kafka.
  ///
  /// \param rk The Kafka handle.
  /// \param level The log level (not used).
  /// \param fac ?
  /// \param buf The message buffer.
  static void logCallback(rd_kafka_t const *rk, int level, char const *fac,
                          char const *buf);

  /// The throttle callback for Kafka.
  ///
  /// \param rk The Kafka handle (not used).
  /// \param broker_name The broker name.
  /// \param broker_id  The broker id.
  /// \param throttle_time_ms The throttle time in milliseconds.
  /// \param opaque The opaque.
  static void throttleCallback(rd_kafka_t *rk, char const *broker_name,
                               int32_t broker_id, int throttle_time_ms,
                               void *opaque);
  rd_kafka_t *getRdKafkaPtr() const;
  std::function<void(rd_kafka_message_t const *msg)> on_delivery_ok;
  std::function<void(rd_kafka_message_t const *msg)> on_delivery_failed;
  std::function<void(Producer *, rd_kafka_resp_err_t)> on_error;
  // Currently it's nice to have access to these two for statistics:
  BrokerSettings ProducerBrokerSettings;
  rd_kafka_t *RdKafkaPtr = nullptr;
  std::atomic<uint64_t> TotalMessagesProduced{0};
  ProducerStats Stats;

private:
  int id = 0;
};
}
