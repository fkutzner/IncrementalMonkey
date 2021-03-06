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

#include "FakeIPASIRSolver.h"
#include "FileUtils.h"

#include <gmock/gmock.h>
#include <gsl/gsl_util>
#include <gtest/gtest.h>

#include <filesystem>
#include <vector>

using ::testing::Eq;

namespace fs = std::filesystem;

namespace incmonk {

// clang-format off
using FuzzTraceExecTests_executeTrace_param = std::tuple<
  FuzzTrace,  // The input trace
  std::vector<FakeIPASIRResult>, // The sequence of IPASIR solve() results wrt. FuzzTrace
  std::optional<std::size_t>, // The expected index of the first failing solve command within the trace
  std::optional<TraceExecutionFailure::Reason> // The expected failure reason, if any
>;
// clang-format on

class FuzzTraceExecTests_executeTrace
  : public ::testing::TestWithParam<FuzzTraceExecTests_executeTrace_param> {
public:
  virtual ~FuzzTraceExecTests_executeTrace() = default;

  auto getInputTrace() const -> FuzzTrace { return std::get<0>(GetParam()); }

  auto getIPASIRResults() const -> std::vector<FakeIPASIRResult> { return std::get<1>(GetParam()); }

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

using SolveResults = std::vector<FakeIPASIRResult>;

// clang-format off
INSTANTIATE_TEST_SUITE_P(, FuzzTraceExecTests_executeTrace,
  ::testing::Values (
    std::make_tuple(FuzzTrace{}, SolveResults{}, std::nullopt, std::nullopt),
    std::make_tuple(FuzzTrace{AddClauseCmd{{1, 2}}}, SolveResults{}, std::nullopt, std::nullopt),

    std::make_tuple(
      FuzzTrace {
        AddClauseCmd{{1, 2}},
        AddClauseCmd{{-2, 5}},
        SolveCmd{}
      },
      SolveResults {
        {IPASIRSolver::Result::SAT, {1, 5}}
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
      SolveResults {
        {IPASIRSolver::Result::UNSAT, {1, 2}}
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
      SolveResults {
        {IPASIRSolver::Result::SAT, {1, -2}},
        {IPASIRSolver::Result::UNSAT, {1, 2}}
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
      SolveResults {
        {IPASIRSolver::Result::UNSAT, {1, 2}},
        {IPASIRSolver::Result::UNSAT, {1, 2}}
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
      SolveResults {
        {IPASIRSolver::Result::SAT, {1, -2}},
        {IPASIRSolver::Result::UNSAT, {-1, 2}}
      },
      4, TraceExecutionFailure::Reason::INVALID_FAILED
    ),

    std::make_tuple(
      FuzzTrace {
        AddClauseCmd{{1, 2}},
        AddClauseCmd{{-2, -1}},
        SolveCmd{},
        AssumeCmd{{1, 2}},
        SolveCmd{}
      },
      SolveResults {
        {IPASIRSolver::Result::SAT, {1, -2}},
        {IPASIRSolver::Result::UNSAT, {2}}
      },
      4, TraceExecutionFailure::Reason::INVALID_FAILED
    ),

    std::make_tuple(
      FuzzTrace {
        AddClauseCmd{{1, 2}},
        AddClauseCmd{{-2, -1}},
        SolveCmd{},
        AssumeCmd{{1, 2}},
        SolveCmd{}
      },
      SolveResults {
        {IPASIRSolver::Result::UNSAT, {1, 2}},
        {IPASIRSolver::Result::UNSAT, {-1, 2}}
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
      SolveResults {
        {IPASIRSolver::Result::SAT, {1, -2}},
        {IPASIRSolver::Result::SAT, {1, -2}}
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
      SolveResults {
        {IPASIRSolver::Result::SAT, {1, 2}},
        {IPASIRSolver::Result::UNSAT, {1, 2}}
      },
      2, TraceExecutionFailure::Reason::INVALID_MODEL
    ),

    std::make_tuple(
      FuzzTrace {
        AddClauseCmd{{1, 2}},
        AddClauseCmd{{-2, -1}},
        AssumeCmd{{1}},
        SolveCmd{},
        AssumeCmd{{1, 2}},
        SolveCmd{}
      },
      SolveResults {
        {IPASIRSolver::Result::SAT, {-1, 2}},
        {IPASIRSolver::Result::UNSAT, {1, 2}}
      },
      3, TraceExecutionFailure::Reason::INVALID_MODEL
    ),

    std::make_tuple(
      FuzzTrace {
        AddClauseCmd{{1, 2}},
        AddClauseCmd{{-2, -1}},
        SolveCmd{},
        AssumeCmd{{1, 2}},
        SolveCmd{}
      },
      SolveResults {
        {IPASIRSolver::Result::SAT, {1, -2}},
        {IPASIRSolver::Result::SAT, {1, 2}}
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
      SolveResults {
        {IPASIRSolver::Result::SAT, {1, -2}},
        {IPASIRSolver::Result::UNKNOWN, {}}
      },
      4, TraceExecutionFailure::Reason::INVALID_RESULT
    ),

    std::make_tuple(
      FuzzTrace {
        AddClauseCmd{{1, 2}},
        AddClauseCmd{{-2, -1}},
        SolveCmd{},
        AssumeCmd{{1, 2}},
        SolveCmd{}
      },
      SolveResults {
        {IPASIRSolver::Result::SAT, {1, -2}},
        {IPASIRSolver::Result::ILLEGAL_RESULT, {}}
      },
      4, TraceExecutionFailure::Reason::INVALID_RESULT
    )

  )
);
// clang-format on

TEST(FuzzTraceExecTests_executeTraceWithDump, WhenExecutionSucceeds_NoTraceIsWritten)
{
  PathWithDeleter tempDir = createTempDir();
  FuzzTrace inputTrace{AddClauseCmd{{1, 2}},
                       AddClauseCmd{{-2, -1}},
                       SolveCmd{},
                       SolveCmd{},
                       AssumeCmd{{1, 2}},
                       SolveCmd{}};

  // clang-format off
  FakeIPASIRSolver fakeSut{
      {{IPASIRSolver::Result::SAT, {-1, 2}},
       {IPASIRSolver::Result::SAT, {1, -2}},
       {IPASIRSolver::Result::UNSAT, {1, 2}}}
  };
  // clang-format on

  fs::path originalCwd = fs::current_path();
  fs::current_path(tempDir.getPath());

  gsl::final_action cleaupUp{[&originalCwd]() {
    std::error_code ec;
    fs::current_path(originalCwd, ec);
  }};

  std::optional<TraceExecutionFailure> result =
      executeTraceWithDump(inputTrace.begin(), inputTrace.end(), fakeSut, "incmonk-test", 512);

  EXPECT_FALSE(result.has_value());
  EXPECT_TRUE(std::filesystem::is_empty(tempDir.getPath()));
}

namespace {
void runTraceDumpTest(FakeIPASIRResult failingResult, std::string expectedFilenameSuffix)
{
  PathWithDeleter tempDir = createTempDir();

  FuzzTrace inputTrace{AddClauseCmd{{1, 2}},
                       AddClauseCmd{{-2, -1}},
                       SolveCmd{},
                       SolveCmd{},
                       AssumeCmd{{1, 2}},
                       SolveCmd{}};

  // clang-format off
  FakeIPASIRSolver fakeSut{
      {{IPASIRSolver::Result::SAT, {-1, 2}},
       {IPASIRSolver::Result::SAT, {1, -2}},
       failingResult}
  };
  // clang-format on

  fs::path expectedFilename =
      tempDir.getPath() / ("incmonk-test-000512-" + expectedFilenameSuffix + ".mtr");

  fs::path originalCwd = fs::current_path();
  fs::current_path(tempDir.getPath());

  gsl::final_action cleaupUp{[&expectedFilename, &originalCwd]() {
    std::error_code ec;
    if (fs::exists(expectedFilename, ec)) {
      fs::remove(expectedFilename, ec);
    }
    fs::current_path(originalCwd, ec);
  }};

  std::optional<TraceExecutionFailure> result =
      executeTraceWithDump(inputTrace.begin(), inputTrace.end(), fakeSut, "incmonk-test", 512);

  ASSERT_TRUE(result.has_value());
  ASSERT_TRUE(fs::exists(expectedFilename));

  SolveCmd expectedFinalCmd;
  if (result->reason != TraceExecutionFailure::Reason::INVALID_RESULT &&
      result->reason != TraceExecutionFailure::Reason::TIMEOUT) {
    expectedFinalCmd.expectedResult = false;
  }

  FuzzTrace expectedWrittenTrace{AddClauseCmd{{1, 2}},
                                 AddClauseCmd{{-2, -1}},
                                 SolveCmd{true},
                                 SolveCmd{true},
                                 AssumeCmd{{1, 2}},
                                 expectedFinalCmd};

  FuzzTrace actualWrittenTrace = loadTrace(expectedFilename);

  EXPECT_THAT(actualWrittenTrace, Eq(expectedWrittenTrace));
}
}

TEST(FuzzTraceExecTests_executeTraceWithDump, WhenExecutionYieldsIncorrect_TraceIsWritten)
{
  runTraceDumpTest({IPASIRSolver::Result::SAT, {1, 2}}, "satflip");
}

TEST(FuzzTraceExecTests_executeTraceWithDump, WhenExecutionYieldsInvalidFailed_TraceIsWritten)
{
  runTraceDumpTest({IPASIRSolver::Result::UNSAT, {-1, 2}}, "invalidfailed");
}

TEST(FuzzTraceExecTests_executeTraceWithDump, WhenExecutionYieldsIllegalResult_TraceIsWritten)
{
  runTraceDumpTest({IPASIRSolver::Result::ILLEGAL_RESULT, {}}, "invalidresult");
}

TEST(FuzzTraceExecTests_executeTraceWithDump, WhenExecutionYieldsUnknown_TraceIsWritten)
{
  runTraceDumpTest({IPASIRSolver::Result::UNKNOWN, {}}, "invalidresult");
}

}
