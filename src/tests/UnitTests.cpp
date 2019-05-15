#include <gtest/gtest.h>
#include <logger.h>

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);

  // log errors during tests
  std::string LogFile;
  std::string GraylogURI;
  ::setUpLogging(spdlog::level::err, LogFile, GraylogURI);

  return RUN_ALL_TESTS();
}
