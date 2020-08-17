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

class FuzzTraceTests_toCFunctionBody
  : public ::testing::TestWithParam<std::tuple<FuzzTrace, std::string>> {
public:
  virtual ~FuzzTraceTests_toCFunctionBody() = default;
};

TEST_P(FuzzTraceTests_toCFunctionBody, TestSuite)
{
  FuzzTrace input = std::get<0>(GetParam());
  std::string expectedResult = std::get<1>(GetParam());
  std::string result = toCFunctionBody(input, "solver");

  std::string const resultWithNewlines = result;
  result.erase(std::remove(result.begin(), result.end(), '\n'), result.end());
  expectedResult.erase(std::remove(expectedResult.begin(), expectedResult.end(), '\n'),
                       expectedResult.end());

  EXPECT_THAT(result, ::testing::Eq(expectedResult)) << "Result trace:\n" << resultWithNewlines;
}

// clang-format off
INSTANTIATE_TEST_CASE_P(, FuzzTraceTests_toCFunctionBody,
  ::testing::Values(
    std::make_tuple(FuzzTrace{}, ""),

    std::make_tuple(FuzzTrace{AddClauseCmd{}}, "ipasir_add(solver, 0);"),
    std::make_tuple(FuzzTrace{AddClauseCmd{{1}}}, "ipasir_add(solver, 1);ipasir_add(solver, 0);"),
    std::make_tuple(FuzzTrace{AddClauseCmd{{1, -2, -3}}},
      "ipasir_add(solver, 1);ipasir_add(solver, -2);ipasir_add(solver, -3);ipasir_add(solver, 0);"),

    std::make_tuple(FuzzTrace{AssumeCmd{}}, ""),
    std::make_tuple(FuzzTrace{AssumeCmd{{1}}}, "ipasir_assume(solver, 1);"),
    std::make_tuple(FuzzTrace{AssumeCmd{{1, -2, -3}}},
      "ipasir_assume(solver, 1);ipasir_assume(solver, -2);ipasir_assume(solver, -3);"),

    std::make_tuple(FuzzTrace{SolveCmd{}}, "ipasir_solve(solver);"),
    std::make_tuple(FuzzTrace{SolveCmd{false}}, "{int result = ipasir_solve(solver); assert(result == 20);}"),
    std::make_tuple(FuzzTrace{SolveCmd{true}}, "{int result = ipasir_solve(solver); assert(result == 10);}"),

    std::make_tuple(
      FuzzTrace{
        AddClauseCmd{{1, -2}},
        AddClauseCmd{{-2, 4}},
        AssumeCmd{{1}},
        SolveCmd{true}
      },
      "ipasir_add(solver, 1);ipasir_add(solver, -2);ipasir_add(solver, 0);"
      "ipasir_add(solver, -2);ipasir_add(solver, 4);ipasir_add(solver, 0);"
      "ipasir_assume(solver, 1);"
      "{int result = ipasir_solve(solver); assert(result == 10);}"
    )
  )
);
// clang-format on


class RecordingIPASIRSolver : public IPASIRSolver {
public:
  RecordingIPASIRSolver(std::vector<int> const& solveResults)
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

  auto solve() -> int override
  {
    assert(!m_solveResults.empty());
    int const result = m_solveResults.back();
    m_solveResults.pop_back();

    std::optional<bool> traceResult;
    if (result > 0) {
      traceResult = (result == 10);
    }

    m_recordedTrace.push_back(SolveCmd{traceResult});
    return result;
  }

  virtual void configure(uint64_t) override { m_calledConfigure = true; }

  auto getTrace() const noexcept -> FuzzTrace const& { return m_recordedTrace; }

  auto hasConfigureBeenCalled() const noexcept -> bool { return m_calledConfigure; }

private:
  std::vector<int> m_solveResults;
  FuzzTrace m_recordedTrace;
  bool m_calledConfigure = false;
};


class FuzzTraceTests_applyTrace : public ::testing::TestWithParam<FuzzTrace> {
public:
  virtual ~FuzzTraceTests_applyTrace() = default;
};

namespace {
std::vector<int> getSolveResults(FuzzTrace const& trace)
{
  std::vector<int> result;
  for (FuzzCmd const& cmd : trace) {
    if (SolveCmd const* solveCmd = std::get_if<SolveCmd>(&cmd); solveCmd != nullptr) {
      if (solveCmd->expectedResult.has_value()) {
        result.push_back(*(solveCmd->expectedResult) ? 10 : 20);
      }
      else {
        result.push_back(0);
      }
    }
  }
  return result;
}
}

TEST_P(FuzzTraceTests_applyTrace, TestSuite_withoutFailure)
{
  FuzzTrace input = GetParam();
  RecordingIPASIRSolver recorder{getSolveResults(input)};
  std::optional<size_t> resultIndex = applyTrace(input, recorder);

  EXPECT_FALSE(resultIndex.has_value());
  EXPECT_FALSE(recorder.hasConfigureBeenCalled());
  EXPECT_THAT(recorder.getTrace(), ::testing::Eq(input));
}

// clang-format off
INSTANTIATE_TEST_CASE_P(, FuzzTraceTests_applyTrace,
  ::testing::Values(
    FuzzTrace{},
    FuzzTrace{AddClauseCmd{}},
    FuzzTrace{AddClauseCmd{{1, -2, -3}}},
    FuzzTrace{AssumeCmd{{1, -2, -3}}},
    FuzzTrace{SolveCmd{}},
    FuzzTrace{SolveCmd{false}},
    FuzzTrace{SolveCmd{true}},
    FuzzTrace{
        AddClauseCmd{{1, -2}},
        AddClauseCmd{{-2, 4}},
        AssumeCmd{{1}},
        SolveCmd{true}
    },
    FuzzTrace{
        AddClauseCmd{{1, -2}},
        AddClauseCmd{{-2, 4}},
        AssumeCmd{{1}},
        SolveCmd{true},
        SolveCmd{true},
        AddClauseCmd{{2}},
        AddClauseCmd{{-4}},
        SolveCmd{false}
    }
  )
);
// clang-format on

TEST_F(FuzzTraceTests_applyTrace, WhenSolvingFails_ThenIndexOfFailingCmdIsReturned)
{
  // clang-format off
  FuzzTrace input {
    AddClauseCmd{{1, -2}},
    AddClauseCmd{{-2, 4}},
    AssumeCmd{{1}},
    SolveCmd{true},
    SolveCmd{true},
    AddClauseCmd{{2}},
    AddClauseCmd{{-4}},
    SolveCmd{false}
  };
  // clang-format on

  std::vector<int> solveResults{10, 20, 20};

  RecordingIPASIRSolver recorder{solveResults};
  std::optional<size_t> resultIndex = applyTrace(input, recorder);

  ASSERT_TRUE(resultIndex.has_value());
  EXPECT_THAT(*resultIndex, ::testing::Eq(4));

  EXPECT_FALSE(recorder.hasConfigureBeenCalled());

  FuzzTrace expectedTrace{input.begin(), input.begin() + 4};
  expectedTrace.push_back(SolveCmd{false});

  EXPECT_THAT(recorder.getTrace(), ::testing::Eq(expectedTrace));
}

class FuzzTraceTests_loadStoreTrace
  : public ::testing::TestWithParam<std::tuple<FuzzTrace, std::vector<uint32_t>>> {
public:
  virtual ~FuzzTraceTests_loadStoreTrace() = default;
};

TEST_P(FuzzTraceTests_loadStoreTrace, TestSuite_store)
{
  PathWithDeleter tempFile = createTempFile();
  FuzzTrace input = std::get<0>(GetParam());

  storeTrace(input, tempFile.getPath());

  auto maybeResult = slurpUInt32File(tempFile.getPath());
  ASSERT_TRUE(maybeResult.has_value());

  std::vector<std::uint32_t> result = *maybeResult;
  std::vector<std::uint32_t> expected = std::get<1>(GetParam());
  EXPECT_THAT(result, ::testing::Eq(expected));
}

/*TEST_P(FuzzTraceTests_loadStoreTrace, TestSuite_load)
{
  PathWithDeleter tempFile = createTempFile();
  std::vector<std::uint32_t> input = std::get<1>(GetParam());
  
  writeUInt32VecToFile(input, tempFile.getPath());

  FuzzTrace expected = std::get<0>(GetParam());
  FuzzTrace result = loadTrace(tempFile.getPath());

  EXPECT_THAT(result, ::testing::Eq(expected));
}*/


namespace {
constexpr static uint32_t magicCookie = 0xABCD0000;

}

// clang-format off
INSTANTIATE_TEST_CASE_P(, FuzzTraceTests_loadStoreTrace,
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

    std::make_tuple(
      FuzzTrace{
        AddClauseCmd{{1, -2}},
        AddClauseCmd{{-2, 4}},
        AssumeCmd{{1}},
        SolveCmd{true},
        SolveCmd{true},
        AddClauseCmd{},
        AddClauseCmd{{2}},
        AddClauseCmd{{-4}},
        SolveCmd{false}
      },
      std::vector<uint32_t>{
        magicCookie,
        0x01000002, 2, 5,
        0x01000002, 5, 8,
        0x02000001, 2,
        0x0300000A,
        0x0300000A,
        0x01000000,
        0x01000001, 4,
        0x01000001, 9,
        0x03000014
      }
    )
  )
);
// clang-format on
}