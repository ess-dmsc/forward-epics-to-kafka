#include "tests.h"
#include "../MainOpt.h"
#include <gtest/gtest.h>

namespace BrightnESS {
namespace ForwardEpicsToKafka {
namespace tests {
MainOpt *Tests::main_opt = nullptr;
}
}
}

int main(int argc, char **argv) {

  ::testing::InitGoogleTest(&argc, argv);
  std::string f = ::testing::GTEST_FLAG(filter);
  if (f.find("Remote") == std::string::npos) {
    f = f + std::string(":-*Remote*");
  }
  ::testing::GTEST_FLAG(filter) = f;

  auto po = BrightnESS::ForwardEpicsToKafka::parse_opt(argc, argv);
  if (po.first) {
    return 1;
  }
  auto opt = std::move(po.second);
  BrightnESS::ForwardEpicsToKafka::tests::Tests::main_opt = opt.get();
  return RUN_ALL_TESTS();
}
