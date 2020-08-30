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

#include <cassert>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <unordered_set>

namespace incmonk {

namespace {

using Analysis = std::optional<TraceExecutionFailure::Reason>;

auto analyzeSatResult(FuzzTrace::iterator phaseStop, IPASIRSolver& sut, Oracle& oracle) -> Analysis
{
  std::vector<CNFLit> assumptions = oracle.getCurrentAssumptions();

  // Check if all assumptions are heeded:
  bool assumptionFailure = false;
  for (auto assumption : assumptions) {
    TBool const assumptionVal = sut.getValue(assumption);
    if (assumptionVal != t_true) {
      assumptionFailure = true;
      break;
    }
  }

  SolveCmd& solveCmd = std::get<SolveCmd>(*phaseStop);
  if (!assumptionFailure) {
    // Check if the solver actually computed a model:
    CNFLit maxLit = oracle.getMaxSeenLit();
    std::vector<CNFLit> model;
    model.reserve(maxLit);
    for (CNFLit lit = 1; lit <= maxLit; ++lit) {
      TBool val = sut.getValue(lit);
      if (val != t_indet) {
        model.push_back(lit * (val == t_true ? 1 : -1));
      }
    }

    // TODO: check the clauses occurring in the trace
    //   when there are variables without assignment
    TBool probeResult = oracle.probe(model);
    if (probeResult != t_false) {
      solveCmd.expectedResult = true;
      oracle.clearAssumptions();
      return std::nullopt;
    }
  }

  // The model is invalid. Check if this is actually a SAT/UNSAT flip:
  oracle.solve(phaseStop, phaseStop + 1);
  if (!solveCmd.expectedResult.has_value()) {
    return std::nullopt;
  }

  if (*(solveCmd.expectedResult)) {
    return TraceExecutionFailure::Reason::INVALID_MODEL;
  }
  else {
    return TraceExecutionFailure::Reason::INCORRECT_RESULT;
  }
}

auto analyzeUnsatResult(FuzzTrace::iterator phaseStop, IPASIRSolver& sut, Oracle& oracle)
    -> Analysis
{
  std::vector<CNFLit> assumptions = oracle.getCurrentAssumptions();

  std::vector<CNFLit> failed;
  for (CNFLit assumption : assumptions) {
    if (sut.isFailed(assumption)) {
      failed.push_back(assumption);
    }
  }

  SolveCmd& solveCmd = std::get<SolveCmd>(*phaseStop);
  TBool probeResult = oracle.probe(failed);
  if (probeResult != t_true) {
    solveCmd.expectedResult = false;
    oracle.clearAssumptions();
    return std::nullopt;
  }

  oracle.solve(phaseStop, phaseStop + 1);
  if (!solveCmd.expectedResult.has_value()) {
    return std::nullopt;
  }

  if (*(solveCmd.expectedResult) == false) {
    return TraceExecutionFailure::Reason::INVALID_FAILED;
  }
  else {
    return TraceExecutionFailure::Reason::INCORRECT_RESULT;
  }
}

auto analyzeResult(FuzzTrace::iterator phaseStart,
                   FuzzTrace::iterator phaseStop,
                   IPASIRSolver& sut,
                   Oracle& oracle) -> Analysis
{
  assert(std::get_if<SolveCmd>(&*phaseStop) != nullptr);

  IPASIRSolver::Result lastResult = sut.getLastSolveResult();
  if (lastResult == IPASIRSolver::Result::UNKNOWN ||
      lastResult == IPASIRSolver::Result::ILLEGAL_RESULT) {
    return std::make_optional(TraceExecutionFailure::Reason::INVALID_RESULT);
  }

  // stop just before the solve call
  oracle.solve(phaseStart, phaseStop);

  if (lastResult == IPASIRSolver::Result::SAT) {
    return analyzeSatResult(phaseStop, sut, oracle);
  }
  else {
    assert(lastResult == IPASIRSolver::Result::UNSAT);
    return analyzeUnsatResult(phaseStop, sut, oracle);
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

  switch (kind) {
  case TraceExecutionFailure::Reason::INCORRECT_RESULT:
    formatter << "-satflip.mtr";
    break;
  case TraceExecutionFailure::Reason::INVALID_MODEL:
    formatter << "-invalidmodel.mtr";
    break;
  case TraceExecutionFailure::Reason::INVALID_FAILED:
    formatter << "-invalidfailed.mtr";
    break;
  case TraceExecutionFailure::Reason::INVALID_RESULT:
    formatter << "-invalidresult.mtr";
    break;
  default:
    formatter << "-unknown.mtr";
    break;
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
