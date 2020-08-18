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
#include <libincmonk/IPASIRSolver.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <vector>

using ::testing::Eq;

namespace incmonk {
namespace {
class FakeIPASIRSolver : public IPASIRSolver {
public:
  FakeIPASIRSolver(std::vector<Result> const& fakedResults)
  {
    m_fakedResults.assign(fakedResults.rbegin(), fakedResults.rend());
  }


  auto solve() -> Result override
  {
    assert(!m_fakedResults.empty());
    m_lastResult = m_fakedResults.back();
    m_fakedResults.pop_back();
    return m_lastResult;
  }

  auto getLastSolveResult() const noexcept -> Result override { return m_lastResult; }

  void addClause(CNFClause const&) override {}
  void assume(std::vector<CNFLit> const&) override {}
  void configure(uint64_t) override {}

  virtual ~FakeIPASIRSolver() = default;

private:
  std::vector<Result> m_fakedResults;
  Result m_lastResult = Result::UNKNOWN;
};

// clang-format off
using FuzzTraceExecTests_executeTrace_param = std::tuple<
  FuzzTrace,  // The input trace
  std::vector<IPASIRSolver::Result>, // The sequence of IPASIR solve() results wrt. FuzzTrace
  std::optional<std::size_t>, // The expected index of the first failing solve command within the trace
  std::optional<TraceExecutionFailure::Reason> // The expected failure reason, if any
>;
// clang-format on
}

class FuzzTraceExecTests_executeTrace
  : public ::testing::TestWithParam<FuzzTraceExecTests_executeTrace_param> {
public:
  virtual ~FuzzTraceExecTests_executeTrace() = default;

  auto getInputTrace() const -> FuzzTrace { return std::get<0>(GetParam()); }

  auto getIPASIRResults() const -> std::vector<IPASIRSolver::Result>
  {
    return std::get<1>(GetParam());
  }

  auto getFailureIndex() const -> std::optional<std::size_t> { return std::get<2>(GetParam()); }

  auto getFailureReason() const -> std::optional<TraceExecutionFailure::Reason>
  {
    return std::get<3>(GetParam());
  }
};

TEST_P(FuzzTraceExecTests_executeTrace, TestSuite)
{
  FuzzTrace inputTrace = getInputTrace();
  FakeIPASIRSolver fakeSut{getIPASIRResults()};

  std::optional<TraceExecutionFailure> result =
      executeTrace(inputTrace.begin(), inputTrace.end(), fakeSut);

  if (std::optional<std::size_t> failInd = getFailureIndex(); failInd.has_value()) {
    // Check that the execution did fail:
    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(getFailureReason().has_value());

    EXPECT_THAT(result->reason, Eq(*getFailureReason()));
    auto distanceToFailure = std::distance(inputTrace.begin(), result->solveCmd);
    EXPECT_THAT(static_cast<std::size_t>(distanceToFailure), Eq(failInd));
  }
  else {
    // Check that the execution did not fail:
    EXPECT_TRUE(!result.has_value());
  }
}

using IRVec = std::vector<IPASIRSolver::Result>;

// clang-format off
INSTANTIATE_TEST_CASE_P(, FuzzTraceExecTests_executeTrace,
  ::testing::Values (
    std::make_tuple(FuzzTrace{}, IRVec{}, std::nullopt, std::nullopt),
    std::make_tuple(FuzzTrace{AddClauseCmd{{1, 2}}}, IRVec{}, std::nullopt, std::nullopt),

    std::make_tuple(
      FuzzTrace {
        AddClauseCmd{{1, 2}},
        AddClauseCmd{{-2, 5}},
        SolveCmd{}
      },
      IRVec {
        IPASIRSolver::Result::SAT
      },
      std::nullopt, std::nullopt
    ),

    std::make_tuple(
      FuzzTrace {
        AddClauseCmd{{1, 2}},
        AddClauseCmd{{-2, -1}},
        AssumeCmd{{1, 2}},
        SolveCmd{}
      },
      IRVec {
        IPASIRSolver::Result::UNSAT
      },
      std::nullopt, std::nullopt
    ),

    std::make_tuple(
      FuzzTrace {
        AddClauseCmd{{1, 2}},
        AddClauseCmd{{-2, -1}},
        SolveCmd{},
        AssumeCmd{{1, 2}},
        SolveCmd{}
      },
      IRVec {
        IPASIRSolver::Result::SAT,
        IPASIRSolver::Result::UNSAT
      },
      std::nullopt, std::nullopt
    ),

    std::make_tuple(
      FuzzTrace {
        AddClauseCmd{{1, 2}},
        AddClauseCmd{{-2, -1}},
        SolveCmd{},
        AssumeCmd{{1, 2}},
        SolveCmd{}
      },
      IRVec {
        IPASIRSolver::Result::UNSAT,
        IPASIRSolver::Result::UNSAT
      },
      2, TraceExecutionFailure::Reason::INCORRECT_RESULT
    ),

    std::make_tuple(
      FuzzTrace {
        AddClauseCmd{{1, 2}},
        AddClauseCmd{{-2, -1}},
        SolveCmd{},
        AssumeCmd{{1, 2}},
        SolveCmd{}
      },
      IRVec {
        IPASIRSolver::Result::SAT,
        IPASIRSolver::Result::SAT
      },
      4, TraceExecutionFailure::Reason::INCORRECT_RESULT
    ),

    std::make_tuple(
      FuzzTrace {
        AddClauseCmd{{1, 2}},
        AddClauseCmd{{-2, -1}},
        SolveCmd{},
        AssumeCmd{{1, 2}},
        SolveCmd{}
      },
      IRVec {
        IPASIRSolver::Result::SAT,
        IPASIRSolver::Result::UNKNOWN
      },
      4, TraceExecutionFailure::Reason::INCORRECT_RESULT
    ),

    std::make_tuple(
      FuzzTrace {
        AddClauseCmd{{1, 2}},
        AddClauseCmd{{-2, -1}},
        SolveCmd{},
        AssumeCmd{{1, 2}},
        SolveCmd{}
      },
      IRVec {
        IPASIRSolver::Result::SAT,
        IPASIRSolver::Result::ILLEGAL_RESULT
      },
      4, TraceExecutionFailure::Reason::INCORRECT_RESULT
    )

  )
);
// clang-format on
}