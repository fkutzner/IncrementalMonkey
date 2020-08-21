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

#include <libincmonk/Oracle.h>

#include <libincmonk/FuzzTrace.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cmath>
#include <tuple>

using ::testing::Eq;

namespace incmonk {

class OracleTests_resolveSolveCmds
  : public ::testing::TestWithParam<std::tuple<FuzzTrace, FuzzTrace>> {
public:
  virtual ~OracleTests_resolveSolveCmds() = default;
};

TEST_P(OracleTests_resolveSolveCmds, solveAllAtOnce)
{
  FuzzTrace toComplete = std::get<0>(GetParam());
  FuzzTrace expectedResult = std::get<1>(GetParam());

  std::unique_ptr<Oracle> underTest = createOracle();
  underTest->solve(toComplete.begin(), toComplete.end());

  EXPECT_THAT(toComplete, Eq(expectedResult));
}

TEST_P(OracleTests_resolveSolveCmds, solveWithMultipleInvocations)
{
  FuzzTrace toComplete = std::get<0>(GetParam());
  FuzzTrace expectedResult = std::get<1>(GetParam());

  std::size_t lowerThirdIdx = toComplete.size() / 3;
  std::size_t upperThirdIdx = std::ceil(toComplete.size() * (2.0 / 3.0));

  FuzzTrace::iterator lowerThirdIter = toComplete.begin() + lowerThirdIdx;
  FuzzTrace::iterator upperThirdIter = toComplete.begin() + upperThirdIdx;

  std::unique_ptr<Oracle> underTest = createOracle();

  underTest->solve(toComplete.begin(), lowerThirdIter);
  underTest->solve(lowerThirdIter, upperThirdIter);
  underTest->solve(upperThirdIter, toComplete.end());

  EXPECT_THAT(toComplete, Eq(expectedResult));
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(, OracleTests_resolveSolveCmds,
  ::testing::Values (
    std::make_tuple(FuzzTrace{}, FuzzTrace{}),

    std::make_tuple(
      FuzzTrace {
        SolveCmd{}
      },
      FuzzTrace {
        SolveCmd{true}
      }
    ),

    std::make_tuple(
      FuzzTrace {
        AddClauseCmd{{1, 2, 3}},
        AddClauseCmd{{-1, 2}},
        AddClauseCmd{{-2, -3}},
        SolveCmd{}
      },
      FuzzTrace {
        AddClauseCmd{{1, 2, 3}},
        AddClauseCmd{{-1, 2}},
        AddClauseCmd{{-2, -3}},
        SolveCmd{true}
      }
    ),

    std::make_tuple(
      FuzzTrace {
        AddClauseCmd{{1, 2, 3}},
        AddClauseCmd{{-1, 2}},
        AssumeCmd{{1, 3}},
        AddClauseCmd{{-2, -3}},
        SolveCmd{},
        SolveCmd{}
      },
      FuzzTrace {
        AddClauseCmd{{1, 2, 3}},
        AddClauseCmd{{-1, 2}},
        AssumeCmd{{1, 3}},
        AddClauseCmd{{-2, -3}},
        SolveCmd{false},
        SolveCmd{true}
      }
    ),

    std::make_tuple(
      FuzzTrace {
        // Check that existing SolveCmd results are not overwritten
        SolveCmd{false}
      },
      FuzzTrace {
        SolveCmd{false}
      }
    ),

    std::make_tuple(
      FuzzTrace {
        AddClauseCmd{{1, 2, 3}},
        AddClauseCmd{{-1, 2}},
        AssumeCmd{{1, 3}},
        AddClauseCmd{{-2, -3}},
        SolveCmd{},
        SolveCmd{},
        AddClauseCmd{{1}},
        SolveCmd{},
        AssumeCmd{{3}},
        SolveCmd{},
        AddClauseCmd{{7, 8, 2, 3}},
        AddClauseCmd{{7, 10, 2, 3}},
        SolveCmd{true}
      },
      FuzzTrace {
        AddClauseCmd{{1, 2, 3}},
        AddClauseCmd{{-1, 2}},
        AssumeCmd{{1, 3}},
        AddClauseCmd{{-2, -3}},
        SolveCmd{false},
        SolveCmd{true},
        AddClauseCmd{{1}},
        SolveCmd{true},
        AssumeCmd{{3}},
        SolveCmd{false},
        AddClauseCmd{{7, 8, 2, 3}},
        AddClauseCmd{{7, 10, 2, 3}},
        SolveCmd{true}
      }
    )
  )
);
// clang-format on

}