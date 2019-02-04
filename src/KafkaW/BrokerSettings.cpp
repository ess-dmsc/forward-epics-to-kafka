#include "BrokerSettings.h"
#include "KafkaEventCb.h"
#include "logger.h"
#include <librdkafka/rdkafkacpp.h>

namespace KafkaW {

void BrokerSettings::apply(RdKafka::Conf *RdKafkaConfiguration) const {
  std::string ErrorString;
  for (const auto &ConfigurationItem : KafkaConfiguration) {
    LOG(spdlog::level::trace, "set config: {} = {}", ConfigurationItem.first,
        ConfigurationItem.second);
    if (RdKafka::Conf::ConfResult::CONF_OK !=
        RdKafkaConfiguration->set(ConfigurationItem.first,
                                  ConfigurationItem.second, ErrorString)) {
      std::string ThrowMessage =
          fmt::format("Failure setting config: {} = {}",
                      ConfigurationItem.first, ConfigurationItem.second);
      LOG(spdlog::level::warn, ThrowMessage.c_str());
      throw std::runtime_error(ThrowMessage);
    }
  }
}
} // namespace KafkaW
