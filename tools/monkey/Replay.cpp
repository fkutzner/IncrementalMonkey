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

#include "Replay.h"

#include "Utils.h"

#include <libincmonk/FuzzTrace.h>
#include <libincmonk/FuzzTraceExec.h>
#include <libincmonk/IPASIRSolver.h>

#include <cstdio>
#include <iostream>
#include <stdlib.h>

namespace incmonk {

namespace {
auto failureExitCodeOrAbort(bool doAbort)
{
  if (!doAbort) {
    return EXIT_FAILURE;
  }
  else {
    abort();
  }
}
}

auto replayMain(ReplayParams const& params) -> int
{
  try {
    IPASIRSolverDSO ipasirDSO{params.solverLibrary};
    auto ipasir = createIPASIRSolver(ipasirDSO);
    FuzzTrace toReplay = loadTraceFromFileOrStdin(params.traceFile, params.parsePermissive);

    auto failure = executeTrace(toReplay.begin(), toReplay.end(), *ipasir);

    if (failure.has_value()) {
      std::cout << "Failed: test oracle did not accept result\n";
      return failureExitCodeOrAbort(params.abortOnFailure);
    }
  }
  catch (IOException const& error) {
    std::cerr << "Error: " << error.what() << "\n";
    return failureExitCodeOrAbort(params.abortOnFailure);
  }
  catch (DSOLoadError const& error) {
    std::cerr << "Error: " << error.what() << "\n";
    return failureExitCodeOrAbort(params.abortOnFailure);
  }

  std::cout << "Passed\n";
  return EXIT_SUCCESS;
}
}