#include "logger.h"
#include <gtest/gtest.h>

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  ::setUpLogging(spdlog::level::off, "", "");
  spdlog::stdout_color_mt("TestLogger");
  return RUN_ALL_TESTS();
}
