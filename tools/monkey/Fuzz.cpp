/* Copyright (c) 2020 Felix Kutzner (github.com/fkutzner)

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.

 Except as contained in this notice, the name(s) of the above copyright holders
 shall not be used in advertising or otherwise to promote the sale, use or
 other dealings in this Software without prior written authorization.

*/

#include "Fuzz.h"

#include <libincmonk/Fork.h>
#include <libincmonk/FuzzTrace.h>
#include <libincmonk/FuzzTraceExec.h>
#include <libincmonk/IPASIRSolver.h>
#include <libincmonk/Stopwatch.h>

#include <libincmonk/generators/GiraldezLevyGenerator.h>

#include <cstdint>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>

namespace incmonk {

namespace {

auto createFuzzerID() -> std::string
{
  std::stringstream formatter;
  formatter << "monkey-";
  std::random_device seedgen;
  std::mt19937 rng{seedgen()};
  std::uniform_int_distribution<uint32_t> idDistr;
  formatter << std::setfill('0') << std::setw(8) << std::hex << idDistr(rng);
  return formatter.str();
}

class Report {
public:
  void onBeginRound()
  {
    if (m_step > 0 && m_step % 100 == 0) {
      auto elapsedTime = m_stopwatch.getElapsedTime<std::chrono::milliseconds>();
      std::cout << "Running at " << 100000.0 / static_cast<double>(elapsedTime.count()) << " x/s ";
      std::cout << "failures: " << m_failures << " crashes: " << m_crashes << "\n";
      m_stopwatch = Stopwatch{};
    }
    ++m_step;
  }

  void onCrashed() { ++m_crashes; }

  auto getNumCrashes() const noexcept -> uint64_t { return m_crashes; }

  void onFailed() { ++m_failures; }

  auto getNumFailures() const noexcept -> uint64_t { return m_failures; }

private:
  uint64_t m_step = 0;
  Stopwatch m_stopwatch;

  uint64_t m_crashes = 0;
  uint64_t m_failures = 0;
};

void storeCrashTrace(FuzzTrace const& trace, std::string const& fuzzerID, uint32_t runID)
{
  std::stringstream formatter;
  formatter << fuzzerID << "-" << std::setfill('0') << std::setw(6) << runID << "-crashed.mtr";
  storeTrace(trace.begin(), trace.end(), formatter.str());
}
}

auto fuzzerMain(FuzzerParams const& params) -> int
{
  using namespace incmonk;

  std::string fuzzerID = params.fuzzerId.empty() ? createFuzzerID() : params.fuzzerId;
  std::cout << "Fuzzer ID: " << fuzzerID << std::endl;

  auto randomSatGen = createGiraldezLevyGen(params.seed);
  IPASIRSolverDSO ipasirDSO{params.fuzzedLibrary};
  auto ipasir = createIPASIRSolver(ipasirDSO);
  Report report;

  uint64_t runID = 0;
  while (true) {
    report.onBeginRound();

    FuzzTrace trace = randomSatGen->generate();

    uint64_t result = 0;
    try {
      result = *syncExecInFork(
          [&ipasir, &trace, &runID, &fuzzerID]() {
            auto failure =
                executeTraceWithDump(trace.begin(), trace.end(), *ipasir, fuzzerID, runID);
            return failure.has_value() ? 1 : 2;
          },
          EXIT_SUCCESS);
    }
    catch (ChildExecutionFailure const&) {
      report.onCrashed();
      storeCrashTrace(trace, fuzzerID, runID);
    }

    if (result != 2) {
      report.onFailed();
      // Child process has written trace
    }

    ++runID;
    if (params.roundsLimit.has_value() && runID >= *params.roundsLimit) {
      break;
    }
  }

  std::cout << "Finished fuzzing.";
  std::cout << "\nExecuted rounds: " << runID;
  std::cout << "\nDetected failures: " << report.getNumFailures();
  std::cout << "\nDetected crashes: " << report.getNumCrashes();
  std::cout << "\nGenerated error traces: " << (report.getNumFailures() + report.getNumFailures())
            << "\n";
  return EXIT_SUCCESS;
}
}
