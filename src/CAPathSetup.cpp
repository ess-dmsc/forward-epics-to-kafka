// SPDX-License-Identifier: BSD-2-Clause
//
// This code has been produced by the European Spallation Source
// and its partner institutes under the BSD 2 Clause License.
//
// See LICENSE.md at the top level for license information.
//
// Screaming Udder!                              https://esss.se

#include "CAPathSetup.h"
#include <cstdlib>
#include <fstream>

#ifdef _WIN32
#include <direct.h>
#include <stdlib.h>
#else
#include <unistd.h>
#endif

bool fileExists(std::string const &FullPath) {
  std::fstream InFile(FullPath);
  return InFile.good();
}

#ifdef _WIN32
static const std::string caRepeaterName{"caRepeater.exe"};
static const std::string Separator{";"};
int setenv(const char *name, const char *value, int overwrite) {
  int errcode = 0;
  if (!overwrite) {
    size_t envsize = 0;
    errcode = getenv_s(&envsize, NULL, 0, name);
    if (errcode || envsize)
      return errcode;
  }
  return _putenv_s(name, value);
}
#else
static const std::string Separator{":"};
static const std::string caRepeaterName{"caRepeater"};
#endif

void addToPath(std::string const &Path) {
  std::string CurrentPATH{std::getenv("PATH")};
  auto NewPATH = Path + Separator + CurrentPATH;
  setenv("PATH", NewPATH.c_str(), 1);
}

void setPathToCaRepeater(std::string ExecPath) {
  size_t const BufferSize{2048};
  char Buffer[BufferSize];
  if (ExecPath[0] != '/') {
#ifdef _WIN32
    auto ReturnBuffer = _getcwd(Buffer, BufferSize);
#else
    auto ReturnBuffer = getcwd(Buffer, BufferSize);
#endif
    if (ReturnBuffer == nullptr) {
      throw std::runtime_error("Unable to set PATH to caRepeater. Unable to "
                               "get current working directory.");
    }
    std::string WorkingDirectory{ReturnBuffer};
    ExecPath = WorkingDirectory + "/" + ExecPath;
  }
  auto SlashLoc = ExecPath.rfind("/");
  auto ExecDir = ExecPath.substr(0, SlashLoc);
  if (fileExists(ExecDir + "/" + caRepeaterName)) {
    addToPath(ExecDir);
    return;
  }
  SlashLoc = ExecDir.rfind("/");
  auto ExecParentDir = ExecDir.substr(0, SlashLoc);
  if (fileExists(ExecParentDir + "/bin/" + caRepeaterName)) {
    addToPath(ExecParentDir + "/bin");
    return;
  }
  throw std::runtime_error(
      "Unable to set PATH to caRepeater. Unable to find the executable.");
}
