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

#include <libincmonk/FuzzTrace.h>
#include <libincmonk/IPASIRSolver.h>

#include "FileUtils.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <utility>

namespace incmonk {

class RecordingIPASIRSolver : public IPASIRSolver {
public:
  RecordingIPASIRSolver(std::vector<IPASIRSolver::Result> const& solveResults)
  {
    m_solveResults.assign(solveResults.rbegin(), solveResults.rend());
  }

  void addClause(CNFClause const& clause) override
  {
    m_recordedTrace.push_back(AddClauseCmd{clause});
  }

  void assume(std::vector<CNFLit> const& assumptions) override
  {
    m_recordedTrace.push_back(AssumeCmd{assumptions});
  }

  void reinitializeWithHavoc(uint64_t seed) noexcept override
  {
    m_recordedTrace.push_back(HavocCmd{seed, true});
  }

  void havoc(uint64_t seed) noexcept override { m_recordedTrace.push_back(HavocCmd{seed, false}); }

  auto solve() -> IPASIRSolver::Result override
  {
    assert(!m_solveResults.empty());
    IPASIRSolver::Result const result = m_solveResults.back();
    m_solveResults.pop_back();

    std::optional<bool> traceResult;
    if (result == IPASIRSolver::Result::SAT || result == IPASIRSolver::Result::UNSAT) {
      traceResult = (result == IPASIRSolver::Result::SAT);
    }

    m_recordedTrace.push_back(SolveCmd{traceResult});
    m_lastResult = result;
    return result;
  }

  auto getLastSolveResult() const noexcept -> IPASIRSolver::Result override { return m_lastResult; }

  virtual void configure(uint64_t) override { m_calledConfigure = true; }

  auto getTrace() const noexcept -> FuzzTrace const& { return m_recordedTrace; }

  auto hasConfigureBeenCalled() const noexcept -> bool { return m_calledConfigure; }

  auto getValue(CNFLit) const noexcept -> TBool override { return t_indet; }

  auto isFailed(CNFLit) const noexcept -> bool override { return false; }

private:
  std::vector<IPASIRSolver::Result> m_solveResults;
  FuzzTrace m_recordedTrace;
  bool m_calledConfigure = false;
  Result m_lastResult = Result::UNKNOWN;
};


class FuzzTraceTests_applyTrace : public ::testing::TestWithParam<FuzzTrace> {
public:
  virtual ~FuzzTraceTests_applyTrace() = default;
};

namespace {
std::vector<IPASIRSolver::Result> getSolveResults(FuzzTrace const& trace)
{
  std::vector<IPASIRSolver::Result> result;
  for (FuzzCmd const& cmd : trace) {
    if (SolveCmd const* solveCmd = std::get_if<SolveCmd>(&cmd); solveCmd != nullptr) {
      if (solveCmd->expectedResult.has_value()) {
        result.push_back(*(solveCmd->expectedResult) ? IPASIRSolver::Result::SAT
                                                     : IPASIRSolver::Result::UNSAT);
      }
      else {
        result.push_back(IPASIRSolver::Result::UNKNOWN);
      }
    }
  }
  return result;
}
}

TEST_P(FuzzTraceTests_applyTrace, TestSuite_stopsAtSolveCmd)
{
  FuzzTrace input = GetParam();
  RecordingIPASIRSolver recorder{getSolveResults(input)};
  auto resultIter = applyTrace(input.begin(), input.end(), recorder);

  bool inputContainsSolve = std::any_of(input.begin(), input.end(), [](FuzzCmd const& cmd) {
    return std::holds_alternative<SolveCmd>(cmd);
  });

  if (inputContainsSolve) {
    EXPECT_TRUE(std::holds_alternative<SolveCmd>(*resultIter));
    size_t distToStart = std::distance(input.cbegin(), resultIter);

    FuzzTrace expectedPrefix{input.begin(), input.begin() + distToStart + 1};


    EXPECT_FALSE(recorder.hasConfigureBeenCalled());
    EXPECT_THAT(recorder.getTrace(), ::testing::Eq(expectedPrefix));
  }
  else {
    EXPECT_FALSE(recorder.hasConfigureBeenCalled());
    EXPECT_THAT(recorder.getTrace(), ::testing::Eq(input));
  }
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(, FuzzTraceTests_applyTrace,
  ::testing::Values(
    FuzzTrace{},
    FuzzTrace{AddClauseCmd{}},
    FuzzTrace{AddClauseCmd{{1, -2, -3}}},
    FuzzTrace{AssumeCmd{{1, -2, -3}}},
    FuzzTrace{SolveCmd{false}},
    FuzzTrace{SolveCmd{true}},
    FuzzTrace{HavocCmd{1}},
    FuzzTrace{
      HavocCmd{1, true},
      HavocCmd{2, false}
    },
    FuzzTrace{
        AddClauseCmd{{1, -2}},
        AddClauseCmd{{-2, 4}},
        AssumeCmd{{1}},
        SolveCmd{true}
    },
    FuzzTrace{
        HavocCmd{3, true},
        AddClauseCmd{{1, -2}},
        AddClauseCmd{{-2, 4}},
        AssumeCmd{{1}},
        SolveCmd{true},
        SolveCmd{true},
        AddClauseCmd{{2}},
        AddClauseCmd{{-4}},
        HavocCmd{10, false},
        SolveCmd{false}
    }
  )
);
// clang-format on


class FuzzTraceTests_loadStoreTrace
  : public ::testing::TestWithParam<std::tuple<FuzzTrace, std::vector<uint32_t>>> {
public:
  virtual ~FuzzTraceTests_loadStoreTrace() = default;
};

TEST_P(FuzzTraceTests_loadStoreTrace, TestSuite_store)
{
  PathWithDeleter tempFile = createTempFile();
  FuzzTrace input = std::get<0>(GetParam());

  storeTrace(input.begin(), input.end(), tempFile.getPath());

  auto maybeResult = slurpUInt32File(tempFile.getPath());
  ASSERT_TRUE(maybeResult.has_value());

  std::vector<std::uint32_t> result = *maybeResult;
  std::vector<std::uint32_t> expected = std::get<1>(GetParam());
  EXPECT_THAT(result, ::testing::Eq(expected));
}

TEST_P(FuzzTraceTests_loadStoreTrace, TestSuite_loadCorrectlyFormattedFile)
{
  PathWithDeleter tempFile = createTempFile();
  std::vector<std::uint32_t> input = std::get<1>(GetParam());

  writeUInt32VecToFile(input, tempFile.getPath());

  FuzzTrace expected = std::get<0>(GetParam());
  FuzzTrace result = loadTrace(tempFile.getPath());

  EXPECT_THAT(result, ::testing::Eq(expected));
}


namespace {
constexpr static uint32_t magicCookie = 0xF2950000;

}

// clang-format off
INSTANTIATE_TEST_SUITE_P(, FuzzTraceTests_loadStoreTrace,
  ::testing::Values(
    std::make_tuple(FuzzTrace{}, std::vector<uint32_t>{magicCookie}),

    std::make_tuple(
      FuzzTrace{AddClauseCmd{{1, -2}}},
      std::vector<uint32_t>{magicCookie,
        0x01000002, 2, 5
      }
    ),

    std::make_tuple(
      FuzzTrace{AssumeCmd{{1, -2, 10, -11}}},
      std::vector<uint32_t>{magicCookie,
        0x02000004, 2, 5, 20, 23
      }
    ),

    std::make_tuple(
      FuzzTrace{AssumeCmd{{1, -2, 10, -11}}},
      std::vector<uint32_t>{magicCookie,
        0x02000004, 2, 5, 20, 23
      }
    ),

    std::make_tuple(FuzzTrace{SolveCmd{}}, std::vector<uint32_t>{magicCookie, 0x03000000}),
    std::make_tuple(FuzzTrace{SolveCmd{true}}, std::vector<uint32_t>{magicCookie, 0x0300000A}),
    std::make_tuple(FuzzTrace{SolveCmd{false}}, std::vector<uint32_t>{magicCookie, 0x03000014}),
    std::make_tuple(FuzzTrace{HavocCmd{15}}, std::vector<uint32_t>{magicCookie, 0x04000000, 15, 0}),

    std::make_tuple(
      FuzzTrace{
        HavocCmd{(uint64_t{2} << 32) + 16, true},
        AddClauseCmd{{1, -2}},
        AddClauseCmd{{-2, 4}},
        AssumeCmd{{1}},
        SolveCmd{true},
        SolveCmd{true},
        HavocCmd{(uint64_t{2} << 32) + 16, false},
        AddClauseCmd{},
        AddClauseCmd{{2}},
        AddClauseCmd{{-4}},
        SolveCmd{false}
      },
      std::vector<uint32_t>{
        magicCookie,
        0x04000001, 16, 2,
        0x01000002, 2, 5,
        0x01000002, 5, 8,
        0x02000001, 2,
        0x0300000A,
        0x0300000A,
        0x04000000, 16, 2,
        0x01000000,
        0x01000001, 4,
        0x01000001, 9,
        0x03000014
      }
    )
  )
);
// clang-format on

// TODO: negative parsing tests
}