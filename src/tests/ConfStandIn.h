// SPDX-License-Identifier: BSD-2-Clause
//
// This code has been produced by the European Spallation Source
// and its partner institutes under the BSD 2 Clause License.
//
// See LICENSE.md at the top level for license information.
//
// Screaming Udder!                              https://esss.se

#pragma once
#include <gmock/gmock.h>
#include <librdkafka/rdkafkacpp.h>

class ConfStandIn : public RdKafka::Conf {
public:
  MOCK_METHOD3(set,
               RdKafka::Conf::ConfResult(const std::string &,
                                         const std::string &, std::string &));
  MOCK_METHOD3(set, RdKafka::Conf::ConfResult(const std::string &,
                                              RdKafka::DeliveryReportCb *,
                                              std::string &));
  MOCK_METHOD3(set,
               RdKafka::Conf::ConfResult(const std::string &,
                                         RdKafka::EventCb *, std::string &));
  MOCK_METHOD3(set,
               RdKafka::Conf::ConfResult(const std::string &,
                                         const RdKafka::Conf *, std::string &));
  MOCK_METHOD3(set, RdKafka::Conf::ConfResult(const std::string &,
                                              RdKafka::PartitionerCb *,
                                              std::string &));
  MOCK_METHOD3(set,
               RdKafka::Conf::ConfResult(const std::string &,
                                         RdKafka::PartitionerKeyPointerCb *,
                                         std::string &));
  MOCK_METHOD3(set,
               RdKafka::Conf::ConfResult(const std::string &,
                                         RdKafka::SocketCb *, std::string &));
  MOCK_METHOD3(set,
               RdKafka::Conf::ConfResult(const std::string &, RdKafka::OpenCb *,
                                         std::string &));
  MOCK_METHOD3(set, RdKafka::Conf::ConfResult(const std::string &,
                                              RdKafka::RebalanceCb *,
                                              std::string &));
  MOCK_METHOD3(set, RdKafka::Conf::ConfResult(const std::string &,
                                              RdKafka::OffsetCommitCb *,
                                              std::string &));

  MOCK_CONST_METHOD2(get, RdKafka::Conf::ConfResult(const std::string &,
                                                    std::string &));
  MOCK_CONST_METHOD1(get,
                     RdKafka::Conf::ConfResult(RdKafka::DeliveryReportCb *&));
  MOCK_CONST_METHOD1(get, RdKafka::Conf::ConfResult(RdKafka::EventCb *&));
  MOCK_CONST_METHOD1(get, RdKafka::Conf::ConfResult(RdKafka::PartitionerCb *&));
  MOCK_CONST_METHOD1(
      get, RdKafka::Conf::ConfResult(RdKafka::PartitionerKeyPointerCb *&));
  MOCK_CONST_METHOD1(get, RdKafka::Conf::ConfResult(RdKafka::SocketCb *&));
  MOCK_CONST_METHOD1(get, RdKafka::Conf::ConfResult(RdKafka::OpenCb *&));
  MOCK_CONST_METHOD1(get, RdKafka::Conf::ConfResult(RdKafka::RebalanceCb *&));
  MOCK_CONST_METHOD1(get,
                     RdKafka::Conf::ConfResult(RdKafka::OffsetCommitCb *&));

  MOCK_METHOD3(set,
               RdKafka::Conf::ConfResult(const std::string &,
                                         RdKafka::ConsumeCb *, std::string &));
  MOCK_METHOD0(dump, std::list<std::string> *());
};
