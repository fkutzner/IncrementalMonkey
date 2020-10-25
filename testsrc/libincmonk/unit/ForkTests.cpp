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

#include <libincmonk/Fork.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <signal.h>
#include <stdlib.h>

namespace incmonk {

// The fork()ed processes need to return EXIT_FAILURE to ensure that
// no further tests are executed in the test suite.
namespace {
int const childProcessRetVal = EXIT_FAILURE;
}

TEST(ForkTests, WhenFunctionExecutionSucceeds_ReturnValueIsPassedBack)
{
  int expected = 12345;
  std::optional<uint64_t> result =
      syncExecInFork([&expected]() { return expected; }, childProcessRetVal);
  ASSERT_TRUE(result.has_value());
  EXPECT_THAT(*result, ::testing::Eq(expected));
}

TEST(ForkTests, WhenFnDoesNotExceedTimeout_ItsReturnValueIsReturned)
{
  auto result = syncExecInFork(
      []() -> uint64_t {
        sleep(1);
        return 50;
      },
      childProcessRetVal,
      std::chrono::milliseconds{1500});
  ASSERT_TRUE(result.has_value());
  EXPECT_THAT(*result, ::testing::Eq(50));
}

TEST(ForkTests, WhenChildExitsPrematurely_ChildExecutionFailureIsThrown)
{
  EXPECT_THROW(syncExecInFork([]() -> uint64_t { exit(1); }, childProcessRetVal),
               ChildExecutionFailure);
}

TEST(ForkTests, WhenChildSegfaults_ChildExecutionFailureIsThrown)
{
  EXPECT_THROW(syncExecInFork(
                   []() -> uint64_t {
                     raise(SIGSEGV);
                     return 0;
                   },
                   childProcessRetVal),
               ChildExecutionFailure);
}

TEST(ForkTests, WhenFnThrows_ChildExecutionFailureIsThrown)
{
  EXPECT_THROW(syncExecInFork(
                   []() -> uint64_t {
                     // keep gtest from catching the exception in the child process
                     testing::GTEST_FLAG(catch_exceptions) = false;
                     throw 0;
                   },
                   childProcessRetVal),
               ChildExecutionFailure);
}

TEST(ForkTests, WhenFnExceedsTimeout_NothingIsReturned)
{
  auto result = syncExecInFork(
      []() -> uint64_t {
        sleep(100);
        return 0;
      },
      childProcessRetVal,
      std::chrono::milliseconds{100});
  EXPECT_FALSE(result.has_value());
}

TEST(ForkTests, WhenChildExitsPrematurelyBeforeTimeout_ChildExecutionFailureIsThrown)
{
  EXPECT_THROW(syncExecInFork(
                   []() -> uint64_t {
                     sleep(1);
                     exit(1);
                   },
                   childProcessRetVal,
                   std::chrono::milliseconds{100000}),
               ChildExecutionFailure);
}

TEST(ForkTests, WhenChildSegfaultsBeforeTimeout_ChildExecutionFailureIsThrown)
{
  EXPECT_THROW(syncExecInFork(
                   []() -> uint64_t {
                     sleep(1);
                     raise(SIGSEGV);
                     return 0;
                   },
                   childProcessRetVal,
                   std::chrono::milliseconds{100000}),
               ChildExecutionFailure);
}

TEST(ForkTests, WhenFnThrowsBeforeTimeout_ChildExecutionFailureIsThrown)
{
  EXPECT_THROW(syncExecInFork(
                   []() -> uint64_t {
                     sleep(1);
                     // keep gtest from catching the exception in the child process
                     testing::GTEST_FLAG(catch_exceptions) = false;
                     throw 0;
                   },
                   childProcessRetVal,
                   std::chrono::milliseconds{100000}),
               ChildExecutionFailure);
}
}