

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

/**
 * \file
 * 
 * \brief Entry point for Incremental Monkey
 */

#include "Fuzz.h"
#include "GenTrace.h"
#include "PrintCPP.h"
#include "PrintICNF.h"
#include "Replay.h"

#include <libincmonk/Config.h>

#include <CLI/CLI.hpp>

#include <optional>
#include <string>

namespace {

class MonkeyCommand {
public:
  // If the command could be executed (ie. was selected by the user),
  // tryExecute returns the command-main()'s return value. Otherwise,
  // it returns nothing.
  virtual auto tryExecute() -> std::optional<int> = 0;

  MonkeyCommand() = default;
  virtual ~MonkeyCommand() = default;

  MonkeyCommand(MonkeyCommand const&) = delete;
  auto operator=(MonkeyCommand const&) -> MonkeyCommand& = delete;
  MonkeyCommand(MonkeyCommand&&) = delete;
  auto operator=(MonkeyCommand &&) -> MonkeyCommand& = delete;
};


class MonkeyFuzzCommand : public MonkeyCommand {
public:
  MonkeyFuzzCommand(CLI::App& app)
  {
    m_subApp = app.add_subcommand("fuzz", "Random-test IPASIR libraries");
    m_subApp->add_option(
        "--id",
        m_fuzzerParams.fuzzerId,
        "Name of the fuzzer instance, included in trace file names (default: random)");
    m_fuzzMaxRoundsOpt = m_subApp->add_option(
        "--rounds", m_fuzzMaxRounds, "Number of rounds to be executed (default: no limit)");
    m_fuzzTimeoutMillisOpt = m_subApp->add_option(
        "--timeout", m_fuzzTimeoutMillis, "Timeout for solver runs (default: no limit)");
    m_subApp->add_flag("--no-havoc", m_fuzzerParams.disableHavoc, "Disable havoc commands");
    m_subApp->add_option(
        "--seed", m_fuzzerParams.seed, "Random number generator seed for problem generators");
    m_fuzzCfgFileOpt =
        m_subApp->add_option("--config",
                             m_fuzzConfigFile,
                             "Problem generator configuration file. See print-default-cfg command");

    m_subApp
        ->add_option(
            "LIB", m_fuzzerParams.fuzzedLibrary, "Shared library file of the IPASIR solver")
        ->required();
  }

  virtual auto tryExecute() -> std::optional<int> override
  {
    if (m_subApp->parsed()) {
      if (!m_fuzzMaxRoundsOpt->empty()) {
        m_fuzzerParams.roundsLimit = m_fuzzMaxRounds;
      }
      if (!m_fuzzTimeoutMillisOpt->empty()) {
        m_fuzzerParams.timeout = std::chrono::milliseconds{m_fuzzTimeoutMillis};
      }
      if (!m_fuzzCfgFileOpt->empty()) {
        m_fuzzerParams.configFile = m_fuzzConfigFile;
      }
      return incmonk::fuzzerMain(m_fuzzerParams);
    }
    else {
      return std::nullopt;
    }
  }

  virtual ~MonkeyFuzzCommand() = default;

private:
  CLI::App* m_subApp = nullptr;
  CLI::Option* m_fuzzMaxRoundsOpt = nullptr;
  CLI::Option* m_fuzzTimeoutMillisOpt = nullptr;
  CLI::Option* m_fuzzCfgFileOpt = nullptr;

  incmonk::FuzzerParams m_fuzzerParams;
  uint64_t m_fuzzMaxRounds = 0;
  uint64_t m_fuzzTimeoutMillis = 0;
  std::filesystem::path m_fuzzConfigFile;
};


class MonkeyReplayCommand : public MonkeyCommand {
public:
  MonkeyReplayCommand(CLI::App& app)
  {
    m_subApp = app.add_subcommand("replay", "Apply failure traces to IPASIR solvers");
    m_subApp
        ->add_option(
            "LIB", m_replayParams.solverLibrary, "Shared library file of the IPASIR solver")
        ->required();
    m_subApp
        ->add_option("TRACE",
                     m_replayParams.traceFile,
                     ".mtr file to apply. If - is specified, the trace is read from the standard "
                     "input instead.")
        ->required();
  }

  virtual auto tryExecute() -> std::optional<int> override
  {
    if (m_subApp->parsed()) {
      return incmonk::replayMain(m_replayParams);
    }
    else {
      return std::nullopt;
    }
  }

  virtual ~MonkeyReplayCommand() = default;

private:
  CLI::App* m_subApp = nullptr;
  incmonk::ReplayParams m_replayParams;
};


class MonkeyPrintCppCommand : public MonkeyCommand {
public:
  MonkeyPrintCppCommand(CLI::App& app)
  {
    m_subApp = app.add_subcommand("print-cpp", "Print traces as C++11 code");
    m_subApp->add_option("--solver-varname",
                         m_printCppParams.solverVarName,
                         "Solver variable name (default: solver)");
    m_subApp->add_option("--function-name",
                         m_printCppParams.funcName,
                         "Function name (default: only a body is printed)");
    m_subApp
        ->add_option("TRACE",
                     m_printCppParams.traceFile,
                     ".mtr file to print. If - is specified, the trace is read from the standard "
                     "input instead.")
        ->required();
  }

  virtual auto tryExecute() -> std::optional<int> override
  {
    if (m_subApp->parsed()) {
      return incmonk::printCPPMain(m_printCppParams);
    }
    else {
      return std::nullopt;
    }
  }

  virtual ~MonkeyPrintCppCommand() = default;

private:
  CLI::App* m_subApp = nullptr;
  incmonk::PrintCPPParams m_printCppParams;
};


class MonkeyPrintIcnfCommand : public MonkeyCommand {
public:
  MonkeyPrintIcnfCommand(CLI::App& app)
  {
    m_subApp = app.add_subcommand("print-icnf", "Print traces as ICNF instances");
    m_subApp
        ->add_option("TRACE",
                     m_printIcnfParams.traceFile,
                     ".mtr file to print. If - is specified, the trace is read from the standard "
                     "input instead.")
        ->required();
  }

  virtual auto tryExecute() -> std::optional<int> override
  {
    if (m_subApp->parsed()) {
      return incmonk::printICNFMain(m_printIcnfParams);
    }
    else {
      return std::nullopt;
    }
  }

  virtual ~MonkeyPrintIcnfCommand() = default;

private:
  CLI::App* m_subApp = nullptr;
  incmonk::PrintICNFParams m_printIcnfParams;
};


class MonkeyPrintDefaultCfgCommand : public MonkeyCommand {
public:
  MonkeyPrintDefaultCfgCommand(CLI::App& app)
  {
    m_subApp = app.add_subcommand("print-default-cfg", "Print the default configuration");
  }

  virtual auto tryExecute() -> std::optional<int> override
  {
    if (m_subApp->parsed()) {
      std::cout << incmonk::getDefaultConfigTOML() << "\n";
      return EXIT_SUCCESS;
    }
    else {
      return std::nullopt;
    }
  }

  virtual ~MonkeyPrintDefaultCfgCommand() = default;

private:
  CLI::App* m_subApp = nullptr;
  incmonk::PrintICNFParams m_printIcnfParams;
};


class MonkeyGenTraceCommand : public MonkeyCommand {
public:
  MonkeyGenTraceCommand(CLI::App& app)
  {
    m_subApp = app.add_subcommand("gen-trace", "Generate a random trace");
    m_genTraceCfgFileOpt =
        m_subApp->add_option("--config",
                             m_fuzzConfigFile,
                             "Problem generator configuration file. See print-default-cfg command");
    m_subApp->add_option("OUTPUT", m_genTraceParams.outputFile, "Trace filename")->required();
    m_subApp->add_option(
        "--seed", m_genTraceParams.seed, "Random number generator seed for problem generators");
    m_subApp->add_flag("--no-havoc", m_genTraceParams.disableHavoc, "Disable havoc commands");

    m_subApp
        ->add_option("--generator", m_genTraceParams.generator, "Select generator. Default: cam")
        ->transform(CLI::IsMember({"cam", "simp-para"}))
        ->default_val("cam");
  }

  virtual auto tryExecute() -> std::optional<int> override
  {
    if (m_subApp->parsed()) {
      if (!m_genTraceCfgFileOpt->empty()) {
        m_genTraceParams.configFile = m_fuzzConfigFile;
      }
      return incmonk::genTraceMain(m_genTraceParams);
    }
    else {
      return std::nullopt;
    }
  }

  virtual ~MonkeyGenTraceCommand() = default;

private:
  CLI::App* m_subApp = nullptr;
  CLI::Option* m_genTraceCfgFileOpt = nullptr;

  incmonk::GenTraceParams m_genTraceParams;
  std::filesystem::path m_fuzzConfigFile;
};

}

auto main(int argc, char** argv) -> int
{
  std::string const version = INCMONK_VERSION;
  CLI::App app{"A random-testing tool for IPASIR implementations\nVersion " + version, "monkey"};

  std::vector<std::unique_ptr<MonkeyCommand>> commands;
  commands.emplace_back(std::make_unique<MonkeyFuzzCommand>(app));
  commands.emplace_back(std::make_unique<MonkeyGenTraceCommand>(app));
  commands.emplace_back(std::make_unique<MonkeyPrintCppCommand>(app));
  commands.emplace_back(std::make_unique<MonkeyPrintDefaultCfgCommand>(app));
  commands.emplace_back(std::make_unique<MonkeyPrintIcnfCommand>(app));
  commands.emplace_back(std::make_unique<MonkeyReplayCommand>(app));

  app.require_subcommand(1);
  CLI11_PARSE(app, argc, argv);

  for (auto const& cmd : commands) {
    if (std::optional<int> result = cmd->tryExecute(); result.has_value()) {
      return *result;
    }
  }

  // Not reachable: exactly one command returns a non-empty result
  return EXIT_FAILURE;
}
