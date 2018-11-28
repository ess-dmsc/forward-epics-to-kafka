#include "BrokerSettings.h"
#include "logger.h"

namespace KafkaW {
std::unique_ptr<RdKafka::Conf> BrokerSettings::apply() {
  std::string ErrorString;
  auto conf = std::unique_ptr<RdKafka::Conf>(
      RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL));
//  conf->set("group.id",
//            fmt::format("forwarder-command-listener--pid{}", getpid()),
//            ErrorString);
  for (const auto &ConfigurationItem : KafkaConfiguration) {
    LOG(Sev::Debug, "set config: {} = {}", ConfigurationItem.first,
        ConfigurationItem.second);
    if (RdKafka::Conf::ConfResult::CONF_OK !=
        conf->set(ConfigurationItem.first, ConfigurationItem.second,
                  ErrorString)) {
      LOG(Sev::Warning, "Failure setting config: {} = {}",
          ConfigurationItem.first, ConfigurationItem.second);
    }
  }
  return conf;
}
}