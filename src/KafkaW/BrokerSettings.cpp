#include "BrokerSettings.h"
#include "logger.h"
#include <librdkafka/rdkafka.h>
#include <vector>

namespace KafkaW {

BrokerSettings::BrokerSettings() {
  KafkaConfiguration = {
      {"api.version.request", "true"},
      {"metadata.request.timeout.ms", "2000"}, // 2 Secs
      {"socket.timeout.ms", "2000"},
      {"message.max.bytes", "24117248"}, // 23 MiB
      {"fetch.message.max.bytes", "24117248"},
      {"receive.message.max.bytes", "24117248"},
      {"queue.buffering.max.messages", "100000"},
      {"queue.buffering.max.ms", "50"},
      {"queue.buffering.max.kbytes", "819200"}, // 781.25 MiB
      {"batch.num.messages", "100000"},
      {"coordinator.query.interval.ms", "2000"},
      {"heartbeat.interval.ms", "500"},     // 0.5 Secs
      {"statistics.interval.ms", "600000"}, // 1 Min
  };
}

void BrokerSettings::apply(rd_kafka_conf_t *RdKafkaConfiguration) {
  std::vector<char> ErrorString(256);
  for (const auto &ConfigurationItem : KafkaConfiguration) {
    LOG(Sev::Debug, "set config: {} = {}", ConfigurationItem.first,
        ConfigurationItem.second);
    if (RD_KAFKA_CONF_OK !=
        rd_kafka_conf_set(RdKafkaConfiguration, ConfigurationItem.first.c_str(),
                          ConfigurationItem.second.c_str(), ErrorString.data(),
                          ErrorString.size())) {
      LOG(Sev::Warning, "error setting config: {} = {}",
          ConfigurationItem.first, ConfigurationItem.second);
    }
  }
}
}
