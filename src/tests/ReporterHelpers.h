// SPDX-License-Identifier: BSD-2-Clause
//
// This code has been produced by the European Spallation Source
// and its partner institutes under the BSD 2 Clause License.
//
// See LICENSE.md at the top level for license information.
//
// Screaming Udder!                              https://esss.se

#pragma once

#include <atomic>
#include <cstdint>

void waitForReportIterations(uint32_t NumberOfIterationsToWaitFor,
                             std::atomic<uint32_t> &ReportIterations);
