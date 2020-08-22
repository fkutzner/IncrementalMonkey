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

#include <libincmonk/FuzzTraceExec.h>

#include <libincmonk/FuzzTrace.h>
#include <libincmonk/Oracle.h>

#include <filesystem>
#include <iomanip>
#include <sstream>

namespace incmonk {

namespace {

using Analysis = std::optional<TraceExecutionFailure::Reason>;

auto analyzeResult(FuzzTrace::iterator phaseStart,
                   FuzzTrace::iterator phaseStop,
                   IPASIRSolver& sut,
                   Oracle& oracle) -> Analysis
{
  assert(std::get_if<SolveCmd>(&*phaseStop) != nullptr);

  oracle.solve(phaseStart, phaseStop + 1);

  SolveCmd const& solveCmd = std::get<SolveCmd>(*phaseStop);
  IPASIRSolver::Result lastResult = sut.getLastSolveResult();
  bool const oracleHasVal = solveCmd.expectedResult.has_value();

  auto const incorrect = std::make_optional(TraceExecutionFailure::Reason::INCORRECT_RESULT);
  switch (lastResult) {
  case IPASIRSolver::Result::UNKNOWN:
    return incorrect;
  case IPASIRSolver::Result::ILLEGAL_RESULT:
    return incorrect;
  case IPASIRSolver::Result::SAT:
    return (!oracleHasVal || *solveCmd.expectedResult) ? std::nullopt : incorrect;
  case IPASIRSolver::Result::UNSAT:
    return (!oracleHasVal || !(*solveCmd.expectedResult)) ? std::nullopt : incorrect;
  default:
    return std::nullopt;
  }
}
}

auto executeTrace(FuzzTrace::iterator start, FuzzTrace::iterator stop, IPASIRSolver& sut)
    -> std::optional<TraceExecutionFailure>
{
  std::unique_ptr<Oracle> oracle = createOracle();
  FuzzTrace::iterator cursor = start;

  while (cursor != stop) {
    auto newCursor = applyTrace(cursor, stop, sut);

    FuzzTrace::iterator prevCursor = cursor;
    cursor = start + std::distance(FuzzTrace::const_iterator{start}, newCursor);

    if (cursor != stop) {
      assert(std::get_if<SolveCmd>(&*cursor) != nullptr);

      Analysis analysis = analyzeResult(prevCursor, cursor, sut, *oracle);
      if (analysis.has_value()) {
        return TraceExecutionFailure{*analysis, cursor};
      }

      // Skip current solve cmd on next applyTrace
      ++cursor;
    }
  }

  return std::nullopt;
}


auto createTraceFilename(std::string const& fuzzerID, int run, TraceExecutionFailure::Reason kind)
    -> std::filesystem::path
{
  std::stringstream formatter;
  formatter << fuzzerID << "-" << std::setfill('0') << std::setw(6) << run;
  if (kind == TraceExecutionFailure::Reason::INCORRECT_RESULT) {
    formatter << "-incorrect.mtr";
  }
  else {
    formatter << "-timeout.mtr";
  }
  return formatter.str();
}

auto executeTraceWithDump(FuzzTrace::iterator start,
                          FuzzTrace::iterator stop,
                          IPASIRSolver& target,
                          std::string const& fuzzerID,
                          uint32_t runID) -> std::optional<TraceExecutionFailure>
{
  auto failure = executeTrace(start, stop, target);

  if (failure.has_value()) {
    std::filesystem::path traceFilename = createTraceFilename(fuzzerID, runID, failure->reason);
    auto iterBeyondFailingSolveCmd = ++(failure->solveCmd);
    storeTrace(start, iterBeyondFailingSolveCmd, traceFilename);
  }

  return failure;
}
}