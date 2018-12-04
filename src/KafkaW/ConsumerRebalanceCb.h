#include <exception>
#include <librdkafka/rdkafkacpp.h>

class ConsumerRebalanceCb : public RdKafka::RebalanceCb {
public:
  void
  rebalance_cb(RdKafka::KafkaConsumer *consumer, RdKafka::ErrorCode err,
               std::vector<RdKafka::TopicPartition *> &partitions) override {
    throw std::runtime_error(fmt::format("{}{}rebalancing cannot be done: {}",
                                         consumer->name(), partitions.size(),
                                         err));
  }
};