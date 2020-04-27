// SPDX-License-Identifier: BSD-2-Clause
//
// This code has been produced by the European Spallation Source
// and its partner institutes under the BSD 2 Clause License.
//
// See LICENSE.md at the top level for license information.
//
// Screaming Udder!                              https://esss.se

#include <cstdlib>
#include <fstream>
#include <gtest/gtest.h>
#include <logger.h>
#include <sys/syslimits.h>

bool fileExists(std::string const &FullPath) {
  std::fstream InFile(FullPath);
  return InFile.good();
}

void addToPath(std::string const &Path) {
  std::string CurrentPATH{std::getenv("PATH")};
  auto NewPATH = Path + ":" + CurrentPATH;
  setenv("PATH", NewPATH.c_str(), 1);
}

void setPathToCaRepeater(std::string ExecPath) {
  size_t const BufferSize{PATH_MAX};
  char Buffer[BufferSize];
  if (ExecPath[0] != '/') {
    auto ReturnBuffer = getcwd(Buffer, BufferSize);
    if (ReturnBuffer == nullptr) {
      std::cout << "Unable to set PATH to caRepeater.\n";
      return;
    }
    std::string WorkingDirectory{ReturnBuffer};
    ExecPath = WorkingDirectory + "/" + ExecPath;
  }
  auto SlashLoc = ExecPath.rfind("/");
  auto ExecDir = ExecPath.substr(0, SlashLoc);
  if (fileExists(ExecDir + "/caRepeater")) {
    addToPath(ExecDir);
    return;
  }
  SlashLoc = ExecDir.rfind("/");
  auto ExecParentDir = ExecDir.substr(0, SlashLoc);
  if (fileExists(ExecParentDir + "/bin/caRepeater")) {
    addToPath(ExecParentDir + "/bin");
    return;
  }
  std::cout << "Unable to set PATH to caRepeater.\n";
}

int main(int argc, char **argv) {
  // Set up environment
  setPathToCaRepeater(argv[0]);

  ::testing::InitGoogleTest(&argc, argv);

  // log errors during tests
  std::string LogFile;
  std::string GraylogURI;
  ::setUpLogging(spdlog::level::err, LogFile, GraylogURI);

  return RUN_ALL_TESTS();
}
