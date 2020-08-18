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
TEST(ForkTests, WhenFunctionExecutionSucceeds_ReturnValueIsPassedBack)
{
  int expected = 12345;
  uint64_t result = syncExecInFork([&expected]() { return expected; });
  EXPECT_THAT(result, ::testing::Eq(expected));
}

TEST(ForkTests, WhenChildExitsPrematurely_ChildExecutionFailureIsThrown)
{
  EXPECT_THROW(syncExecInFork([]() -> uint64_t { exit(1); }), ChildExecutionFailure);
}

TEST(ForkTests, WhenChildSegfaults_ChildExecutionFailureIsThrown)
{
  EXPECT_THROW(syncExecInFork([]() -> uint64_t {
                 raise(SIGSEGV);
                 return 0;
               }),
               ChildExecutionFailure);
}

TEST(ForkTests, WhenFnThrows_ChildExecutionFailureIsThrown)
{
  EXPECT_THROW(syncExecInFork([]() -> uint64_t { throw 0; }), ChildExecutionFailure);
}

}