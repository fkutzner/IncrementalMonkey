

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

#include <CLI/CLI.hpp>

#include <string>


auto main(int argc, char** argv) -> int
{
  incmonk::FuzzerParams fuzzerParams;
  uint64_t fuzzMaxRounds = 0;

  CLI::App app;
  CLI::App* fuzzApp = app.add_subcommand("fuzz", "Fuzzer for IPASIR libraries");
  fuzzApp->add_option(
      "--id",
      fuzzerParams.fuzzerId,
      "Name of the fuzzer instance, included in trace file names. (Default: random)");
  CLI::Option* fuzzMaxRoundsOpt = fuzzApp->add_option(
      "--rounds", fuzzMaxRounds, "Number of rounds to be executed. (Default: no limit)");
  fuzzApp->add_option(
      "--seed", fuzzerParams.seed, "Random number generator seed for problem generators");

  fuzzApp
      ->add_option("LIB", fuzzerParams.fuzzedLibrary, "Shared library file of the IPASIR library")
      ->required();

  app.require_subcommand(1);
  CLI11_PARSE(app, argc, argv);

  if (fuzzApp->parsed()) {
    if (!fuzzMaxRoundsOpt->empty()) {
      fuzzerParams.roundsLimit = fuzzMaxRounds;
    }
    return incmonk::fuzzerMain(fuzzerParams);
  }

  return incmonk::fuzzerMain({argv[2], std::nullopt, ""});
}
