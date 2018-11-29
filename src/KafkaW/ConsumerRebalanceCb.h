#include <exception>
#include <iostream>
#include <librdkafka/rdkafkacpp.h>

class ConsumerRebalanceCb : public RdKafka::RebalanceCb {
private:
  static void
  part_list_print(const std::vector<RdKafka::TopicPartition *> &partitions) {
    for (unsigned int i = 0; i < partitions.size(); i++)
      std::cerr << partitions[i]->topic() << "[" << partitions[i]->partition()
                << "], ";
    std::cerr << "\n";
  }

public:
  void rebalance_cb(RdKafka::KafkaConsumer *consumer, RdKafka::ErrorCode err,
                    std::vector<RdKafka::TopicPartition *> &partitions) {
    std::cout << consumer->name() << err << partitions.size();
    throw std::runtime_error("rebalance cannot be done");

    //        auto self = static_cast<RdKafka::Consumer *>(Opaque);
    //        switch (err) {
    //            case RdKafka::ERR__ASSIGN_PARTITIONS:
    //                LOG(Sev::Debug, "cb_rebalance assign {}",
    //                consumer->name());
    //                if (auto &Callback = self->on_rebalance_start) {
    //                    Callback(PartitionList);
    //                }
    //                print_partition_list(PartitionList);
    //                CallbackERR = rd_kafka_assign(RK, PartitionList);
    //                if (CallbackERR != RD_KAFKA_RESP_ERR_NO_ERROR) {
    //                    LOG(Sev::Notice, "rebalance error: {}  {}",
    //                        rd_kafka_err2name(CallbackERR),
    //                        rd_kafka_err2str(CallbackERR));
    //                }
    //                if (auto &Callback = self->on_rebalance_assign) {
    //                    Callback(PartitionList);
    //                }
    //                break;
    //            case RdKafka::ERR__REVOKE_PARTITIONS:
    //                LOG(Sev::Warning, "cb_rebalance revoke:");
    //                print_partition_list(PartitionList);
    //                CallbackERR = rd_kafka_assign(RK, nullptr);
    //                if (CallbackERR != RD_KAFKA_RESP_ERR_NO_ERROR) {
    //                    LOG(Sev::Warning, "rebalance error: {}  {}",
    //                        rd_kafka_err2name(CallbackERR),
    //                        rd_kafka_err2str(CallbackERR));
    //                }
    //                break;
    //            default:
    //                LOG(Sev::Info, "cb_rebalance failure and revoke: {}",
    //                   RdKafka::err2str(err));
    //                CallbackERR = rd_kafka_assign(RK, nullptr);
    //                if (CallbackERR != RD_KAFKA_RESP_ERR_NO_ERROR) {
    //                    LOG(Sev::Warning, "rebalance error: {}  {}",
    //                        rd_kafka_err2name(CallbackERR),
    //                        rd_kafka_err2str(CallbackERR));
    //                }
    //                break;
    //        }
  }
};