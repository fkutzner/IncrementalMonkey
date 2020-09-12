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


#include <libincmonk/FuzzTracePrinters.h>

#include <libincmonk/FuzzTrace.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

namespace incmonk {

class FuzzTraceTests_toCxxFunctionBody
  : public ::testing::TestWithParam<std::tuple<FuzzTrace, std::string>> {
public:
  virtual ~FuzzTraceTests_toCxxFunctionBody() = default;
};

TEST_P(FuzzTraceTests_toCxxFunctionBody, TestSuite)
{
  FuzzTrace input = std::get<0>(GetParam());
  std::string expectedResult = std::get<1>(GetParam());
  std::string result = toCxxFunctionBody(input.begin(), input.end(), "solver");

  std::string const resultWithNewlines = result;
  result.erase(std::remove(result.begin(), result.end(), '\n'), result.end());
  expectedResult.erase(std::remove(expectedResult.begin(), expectedResult.end(), '\n'),
                       expectedResult.end());

  EXPECT_THAT(result, ::testing::Eq(expectedResult)) << "Result trace:\n" << resultWithNewlines;
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(, FuzzTraceTests_toCxxFunctionBody,
  ::testing::Values(
    std::make_tuple(FuzzTrace{}, ""),

    std::make_tuple(FuzzTrace{AddClauseCmd{}}, "ipasir_add(solver, 0);"),
    std::make_tuple(FuzzTrace{AddClauseCmd{{1}}}, "for (int lit : {1,0}) {  ipasir_add(solver, lit);}"),
    std::make_tuple(FuzzTrace{AddClauseCmd{{1, -2, -3}}},
      "for (int lit : {1,-2,-3,0}) {  ipasir_add(solver, lit);}"),

    std::make_tuple(FuzzTrace{AssumeCmd{}}, ""),
    std::make_tuple(FuzzTrace{AssumeCmd{{1}}}, "for (int assump : {1}) {  ipasir_assume(solver, assump);}"),
    std::make_tuple(FuzzTrace{AssumeCmd{{1, -2, -3}}},
      "for (int assump : {1,-2,-3}) {  ipasir_assume(solver, assump);}"),

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
      "for (int lit : {1,-2,0}) {  ipasir_add(solver, lit);}"
      "for (int lit : {-2,4,0}) {  ipasir_add(solver, lit);}"
      "for (int assump : {1}) {  ipasir_assume(solver, assump);}"
      "{int result = ipasir_solve(solver); assert(result == 10);}"
    )
  )
);
// clang-format on


class FuzzTraceTests_toICNF : public ::testing::TestWithParam<std::tuple<FuzzTrace, std::string>> {
public:
  virtual ~FuzzTraceTests_toICNF() = default;
};

TEST_P(FuzzTraceTests_toICNF, TestSuite)
{
  FuzzTrace input = std::get<0>(GetParam());
  std::string expectedResult = std::get<1>(GetParam());

  std::stringstream resultCollector;
  toICNF(input.begin(), input.end(), resultCollector);
  std::string result = resultCollector.str();
  std::replace(result.begin(), result.end(), '\n', ' ');

  while (!result.empty() && result.back() == ' ') {
    result.resize(result.size() - 1);
  }

  expectedResult.erase(std::remove(expectedResult.begin(), expectedResult.end(), '\n'),
                       expectedResult.end());

  EXPECT_THAT(result, ::testing::Eq(expectedResult)) << "Result ICNF:\n" << resultCollector.str();
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(, FuzzTraceTests_toICNF,
  ::testing::Values(
    std::make_tuple(FuzzTrace{}, "p inccnf 0 0"),

    std::make_tuple(FuzzTrace{AddClauseCmd{}}, "p inccnf 0 1 0"),
    std::make_tuple(FuzzTrace{AddClauseCmd{{1}}}, "p inccnf 1 1 1 0"),
    std::make_tuple(FuzzTrace{AddClauseCmd{{1, -2, -3}}}, "p inccnf 3 1 1 -2 -3 0"),

    std::make_tuple(FuzzTrace{AssumeCmd{}}, "p inccnf 0 0"),
    std::make_tuple(FuzzTrace{AssumeCmd{{1, -2, -3}}}, "p inccnf 3 0"),

    std::make_tuple(FuzzTrace{SolveCmd{}}, "p inccnf 0 0 a 0"),
    std::make_tuple(FuzzTrace{SolveCmd{false}}, "p inccnf 0 0 a 0"),
    std::make_tuple(FuzzTrace{SolveCmd{true}}, "p inccnf 0 0 a 0"),

    std::make_tuple(
      FuzzTrace{
        AssumeCmd{{1, -2, -3}},
        SolveCmd{true}
      },
      "p inccnf 3 0 a 1 -2 -3 0"
    ),

    std::make_tuple(
      FuzzTrace{
        AddClauseCmd{{1, -2}},
        AddClauseCmd{{-2, 4}},
        AssumeCmd{{1}},
        SolveCmd{true}
      },
      "p inccnf 4 2 1 -2 0 -2 4 0 a 1 0"
    )
  )
);
// clang-format on
}