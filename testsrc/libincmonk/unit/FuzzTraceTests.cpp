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
    std::make_tuple(FuzzTrace{SolveCmd{false}}, "{int result = ipasir_solve(solver); assert(result == 20);"),
    std::make_tuple(FuzzTrace{SolveCmd{true}}, "{int result = ipasir_solve(solver); assert(result == 10);"),

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
      "{int result = ipasir_solve(solver); assert(result == 10);"
    )
  )
);
// clang-format on

}