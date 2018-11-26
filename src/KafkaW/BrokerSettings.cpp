#include "BrokerSettings.h"
#include "logger.h"

namespace KafkaW {

void BrokerSettings::apply(RdKafka::Conf *RdKafkaConfiguration) {
  std::vector<char> ErrorString(256);
  for (const auto &ConfigurationItem : KafkaConfiguration) {
    LOG(Sev::Debug, "set config: {} = {}", ConfigurationItem.first,
        ConfigurationItem.second);
    if (RdKafka::Conf::ConfResult::CONF_OK !=
        RdKafkaConfiguration->set(ConfigurationItem.first.c_str(),
                                  ConfigurationItem.second.c_str(),
                                  ErrorString)) {
      LOG(Sev::Warning, "Failure setting config: {} = {}",
          ConfigurationItem.first, ConfigurationItem.second);
    }
  }
}
}
