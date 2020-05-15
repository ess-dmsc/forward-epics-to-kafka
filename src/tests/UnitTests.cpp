// SPDX-License-Identifier: BSD-2-Clause
//
// This code has been produced by the European Spallation Source
// and its partner institutes under the BSD 2 Clause License.
//
// See LICENSE.md at the top level for license information.
//
// Screaming Udder!                              https://esss.se

#include "CAPathSetup.h"
#include <gtest/gtest.h>
#include <logger.h>

int main(int argc, char **argv) {
  // Set up environment
  try {
    setPathToCaRepeater(argv[0]);
  } catch (std::runtime_error &E) {
    std::cout << "Unable to setup path to caRepeater. The error was: "
              << E.what() << "\n";
    std::cout << "Attempting to continue anyway.\n";
  }

  ::testing::InitGoogleTest(&argc, argv);

  // log errors during tests
  std::string LogFile;
  std::string GraylogURI;
  ::setUpLogging(spdlog::level::err, LogFile, GraylogURI);

  return RUN_ALL_TESTS();
}
