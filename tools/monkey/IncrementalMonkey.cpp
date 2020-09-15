

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

#include <string>


auto main(int argc, char** argv) -> int
{
  incmonk::FuzzerParams fuzzerParams;
  incmonk::ReplayParams replayParams;
  incmonk::PrintCPPParams printParams;
  incmonk::PrintICNFParams printICNFParams;
  incmonk::GenTraceParams genTraceParams;

  uint64_t fuzzMaxRounds = 0;
  uint64_t fuzzTimeoutMillis = 0;
  std::filesystem::path fuzzConfigFile;

  std::string const version = INCMONK_VERSION;
  CLI::App app{"A random-testing tool for IPASIR implementations\nVersion " + version, "monkey"};
  CLI::App* fuzzApp = app.add_subcommand("fuzz", "Random-test IPASIR libraries");
  fuzzApp->add_option(
      "--id",
      fuzzerParams.fuzzerId,
      "Name of the fuzzer instance, included in trace file names (default: random)");
  CLI::Option* fuzzMaxRoundsOpt = fuzzApp->add_option(
      "--rounds", fuzzMaxRounds, "Number of rounds to be executed (default: no limit)");
  CLI::Option* fuzzTimeoutMillisOpt = fuzzApp->add_option(
      "--timeout", fuzzTimeoutMillis, "Timeout for solver runs (default: no limit)");
  fuzzApp->add_flag("--no-havoc", fuzzerParams.disableHavoc, "Disable havoc commands");
  fuzzApp->add_option(
      "--seed", fuzzerParams.seed, "Random number generator seed for problem generators");
  CLI::Option* fuzzCfgFileOpt =
      fuzzApp->add_option("--config",
                          fuzzConfigFile,
                          "Problem generator configuration file. See print-default-cfg command");

  fuzzApp->add_option("LIB", fuzzerParams.fuzzedLibrary, "Shared library file of the IPASIR solver")
      ->required();


  CLI::App* replayApp = app.add_subcommand("replay", "Apply failure traces to IPASIR solvers");
  replayApp
      ->add_option("LIB", replayParams.solverLibrary, "Shared library file of the IPASIR solver")
      ->required();
  replayApp
      ->add_option("TRACE",
                   replayParams.traceFile,
                   ".mtr file to apply. If - is specified, the trace is read from the standard "
                   "input instead.")
      ->required();


  CLI::App* printCPPApp = app.add_subcommand("print-cpp", "Print traces as C++11 code");
  printCPPApp->add_option(
      "--solver-varname", printParams.solverVarName, "Solver variable name (default: solver)");
  printCPPApp->add_option(
      "--function-name", printParams.funcName, "Function name (default: only a body is printed)");
  printCPPApp
      ->add_option("TRACE",
                   printParams.traceFile,
                   ".mtr file to print. If - is specified, the trace is read from the standard "
                   "input instead.")
      ->required();

  CLI::App* printIcnfApp = app.add_subcommand("print-icnf", "Print traces as ICNF instances");
  printIcnfApp
      ->add_option("TRACE",
                   printICNFParams.traceFile,
                   ".mtr file to print. If - is specified, the trace is read from the standard "
                   "input instead.")
      ->required();

  CLI::App* printDefaultCfgApp =
      app.add_subcommand("print-default-cfg", "Print the default configuration");


  CLI::App* genTraceApp = app.add_subcommand("gen-trace", "Generate a random trace");
  CLI::Option* genTraceCfgFileOpt = genTraceApp->add_option(
      "--config",
      fuzzConfigFile,
      "Problem generator configuration file. See print-default-cfg command");
  genTraceApp->add_option("OUTPUT", genTraceParams.outputFile, "Trace filename")->required();
  genTraceApp->add_option(
      "--seed", genTraceParams.seed, "Random number generator seed for problem generators");
  genTraceApp->add_flag("--no-havoc", genTraceParams.disableHavoc, "Disable havoc commands");

  genTraceApp->add_option("--generator", genTraceParams.generator, "Select generator. Default: cam")
      ->transform(CLI::IsMember({"cam", "simp-para"}))
      ->default_val("cam");


  app.require_subcommand(1);
  CLI11_PARSE(app, argc, argv);

  if (fuzzApp->parsed()) {
    if (!fuzzMaxRoundsOpt->empty()) {
      fuzzerParams.roundsLimit = fuzzMaxRounds;
    }
    if (!fuzzTimeoutMillisOpt->empty()) {
      fuzzerParams.timeout = std::chrono::milliseconds{fuzzTimeoutMillis};
    }
    if (!fuzzCfgFileOpt->empty()) {
      fuzzerParams.configFile = fuzzConfigFile;
    }
    return incmonk::fuzzerMain(fuzzerParams);
  }
  else if (replayApp->parsed()) {
    return incmonk::replayMain(replayParams);
  }
  else if (printCPPApp->parsed()) {
    return incmonk::printCPPMain(printParams);
  }
  else if (printIcnfApp->parsed()) {
    return incmonk::printICNFMain(printICNFParams);
  }
  else if (printDefaultCfgApp->parsed()) {
    std::cout << incmonk::getDefaultConfigTOML() << "\n";
    return EXIT_SUCCESS;
  }
  else if (genTraceApp->parsed()) {
    if (!genTraceCfgFileOpt->empty()) {
      genTraceParams.configFile = fuzzConfigFile;
    }
    return incmonk::genTraceMain(genTraceParams);
  }

  // Not reachable
  return EXIT_FAILURE;
}
