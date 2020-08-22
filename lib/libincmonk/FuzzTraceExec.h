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

#pragma once

#include <libincmonk/FuzzTrace.h>
#include <libincmonk/IPASIRSolver.h>

#include <optional>
#include <string>

namespace incmonk {

struct TraceExecutionFailure {
  enum class Reason { INCORRECT_RESULT, TIMEOUT };
  Reason reason;
  FuzzTrace::iterator solveCmd;
};

/**
 * \brief Executes the given trace `[start, stop)` on the solver under test, checking
 *   the results with the test oracle.
 * 
 * \returns on failure: TraceExecutionFailure pointing to the failed solve command,
 *   otherwise nothing. Intedeterminate results are counted as incorrect results.
 */
auto executeTrace(FuzzTrace::iterator start, FuzzTrace::iterator stop, IPASIRSolver& sut)
    -> std::optional<TraceExecutionFailure>;

/**
 * \brief Executes the given trace using `executeTrace()`, writing the trace to disk
 *   on failure.
 * 
 * \param start           Start of the trace to be executed
 * \param stop            Past-the-end iterator of the trace to be executed
 * \param sut             The solver under test
 * \param filenamePrefix  Arbitrary prefix for the trace filename
 * \param runID           The (arbitrary) ID of the execution.
 * 
 * On failure, a file named `filenamePrefix`-`runID`-<X>.mtr is written to the current
 * working directory, with <X> being either `incorrect` or `missingresult`.
 * 
 * \returns see `executeTrace()`
 */
auto executeTraceWithDump(FuzzTrace::iterator start,
                          FuzzTrace::iterator stop,
                          IPASIRSolver& sut,
                          std::string const& filenamePrefix,
                          uint32_t runID) -> std::optional<TraceExecutionFailure>;
}
