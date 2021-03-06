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

#include <libincmonk/Config.h>
#include <libincmonk/Fork.h>
#include <libincmonk/FuzzTrace.h>
#include <libincmonk/FuzzTraceExec.h>
#include <libincmonk/IPASIRSolver.h>
#include <libincmonk/InterspersionSchedulers.h>
#include <libincmonk/Stopwatch.h>

#include <libincmonk/generators/CommunityAttachmentGenerator.h>
#include <libincmonk/generators/MuxGenerator.h>
#include <libincmonk/generators/SimplifiersParadiseGenerator.h>

#include <cstdint>
#include <fstream>
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
      std::cout << "failures: " << m_failures << " crashes: " << m_crashes;
      std::cout << " timeouts: " << m_timeouts << "\n";
      m_stopwatch = Stopwatch{};
    }
    ++m_step;
  }

  void onCrashed() { ++m_crashes; }

  auto getNumCrashes() const noexcept -> uint64_t { return m_crashes; }

  void onFailed() { ++m_failures; }

  auto getNumFailures() const noexcept -> uint64_t { return m_failures; }

  void onTimeout() { ++m_timeouts; }

  auto getNumTimeouts() const noexcept -> uint64_t { return m_timeouts; }

private:
  uint64_t m_step = 0;
  Stopwatch m_stopwatch;

  uint64_t m_crashes = 0;
  uint64_t m_failures = 0;
  uint64_t m_timeouts = 0;
};

void storeCrashTrace(FuzzTrace const& trace, std::string const& fuzzerID, uint32_t runID)
{
  std::stringstream formatter;
  formatter << fuzzerID << "-" << std::setfill('0') << std::setw(6) << runID << "-crashed.mtr";
  storeTrace(trace.begin(), trace.end(), formatter.str());
}

auto supportsHavocing(IPASIRSolverDSO const& dso)
{
  return dso.havocFn != nullptr && dso.havocInitFn != nullptr;
}
}

auto getConfig(FuzzerParams const& params, IPASIRSolverDSO const& ipasirDSO) -> Config
{
  Config cfg = getDefaultConfig(params.seed);
  if (!params.disableHavoc && supportsHavocing(ipasirDSO)) {
    std::cout << "Havoc: enabled\n";
  }
  else {
    cfg.communityAttachmentModelParams.havocSchedule = std::nullopt;
    std::cout << "Havoc: disabled\n";
  }

  if (params.configFile.has_value()) {
    std::ifstream inputFile{*params.configFile};
    if (!inputFile) {
      throw std::runtime_error("could not open the config file");
    }
    return extendConfigViaTOML(cfg, inputFile);
  }
  else {
    return cfg;
  }
}

auto fuzzerMain(FuzzerParams const& params) -> int
{
  using namespace incmonk;

  std::string fuzzerID = params.fuzzerId.empty() ? createFuzzerID() : params.fuzzerId;
  std::cout << "ID: " << fuzzerID << "\n";
  std::cout << "Random seed: " << params.seed << "\n";

  IPASIRSolverDSO ipasirDSO{params.fuzzedLibrary};
  std::unique_ptr<IPASIRSolver> ipasir;

  try {
    ipasir = createIPASIRSolver(ipasirDSO);
  }
  catch (DSOLoadError const& error) {
    std::cerr << "Error: " << error.what() << "\n";
    return EXIT_FAILURE;
  }

  std::unique_ptr<Config> cfg;
  try {
    cfg = std::make_unique<Config>(getConfig(params, ipasirDSO));
  }
  catch (std::runtime_error const& error) {
    std::cerr << "Failed loading the configuration: " << error.what() << "\n";
    return EXIT_FAILURE;
  }

  // clang-format off
  std::vector<MuxGeneratorSpec> generators;
  generators.emplace_back(1.0, createCommunityAttachmentGen(std::move(cfg->communityAttachmentModelParams)));
  generators.emplace_back(1.0, createSimplifiersParadiseGen(std::move(cfg->simplifiersParadiseParams)));
  // clang-format on
  auto randomTraceGen = createMuxGenerator(std::move(generators), params.seed + 100);


  Report report;

  uint64_t runID = 0;
  while (true) {
    report.onBeginRound();

    FuzzTrace trace = randomTraceGen->generate();

    bool crashed = false;
    std::optional<uint64_t> result = 0;
    try {
      result = syncExecInFork(
          [&ipasir, &trace, &runID, &fuzzerID]() {
            auto failure =
                executeTraceWithDump(trace.begin(), trace.end(), *ipasir, fuzzerID, runID);
            return failure.has_value() ? 1 : 2;
          },
          EXIT_SUCCESS,
          params.timeout);
    }
    catch (ChildExecutionFailure const&) {
      report.onCrashed();
      storeCrashTrace(trace, fuzzerID, runID);
      crashed = true;
    }

    if (!result.has_value()) {
      report.onTimeout();
    }
    else if (!crashed && *result != 2) {
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
  std::cout << "\nTimeouts: " << report.getNumTimeouts();
  std::cout << "\nDetected correctness failures: " << report.getNumFailures();
  std::cout << "\nDetected crashes: " << report.getNumCrashes();
  std::cout << "\nGenerated error traces: " << (report.getNumCrashes() + report.getNumFailures())
            << "\n";
  return EXIT_SUCCESS;
}
}
