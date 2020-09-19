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

#include <libincmonk/verifier/Assignment.cpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <utility>
#include <vector>

using ::testing::Eq;
using ::testing::IsEmpty;
using ::testing::SizeIs;

namespace incmonk::verifier {

class AssignmentTests_Add : public ::testing::TestWithParam<std::vector<Lit>> {
public:
  virtual ~AssignmentTests_Add() = default;

  Assignment createAssignmentUnderTest()
  {
    Assignment underTest{10_Lit};
    for (Lit input : GetParam()) {
      underTest.add(input);
    }
    return underTest;
  }
};

TEST_P(AssignmentTests_Add, WhenLiteralsAreAdded_ThenSizeIsSet)
{
  Assignment underTest = createAssignmentUnderTest();
  EXPECT_THAT(underTest, SizeIs(GetParam().size()));

  if (GetParam().empty()) {
    EXPECT_THAT(underTest, IsEmpty());
  }
}

TEST_P(AssignmentTests_Add, WhenLiteralsAreAdded_ThenAssignmentsCanBeRetrieved)
{
  Assignment underTest = createAssignmentUnderTest();

  for (Lit input : GetParam()) {
    EXPECT_THAT(underTest.get(input), t_true);
    EXPECT_THAT(underTest.get(-input), t_false);
  }
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(, AssignmentTests_Add,
  ::testing::Values (
    std::vector<Lit>{},
    std::vector<Lit>{1_Lit},
    std::vector<Lit>{1_Lit, 4_Lit, -5_Lit, 10_Lit},
    std::vector<Lit>{-10_Lit}
  )
);
// clang-format on

TEST(AssignmentTests, WhenLitIsNotAssigned_ItIsIndet)
{
  Assignment underTest{10_Lit};
  EXPECT_EQ(underTest.get(5_Lit), t_indet);
  EXPECT_EQ(underTest.get(-5_Lit), t_indet);
}

TEST(AssignmentTests, WhenAssignmentIsCleared_LitsAreIndet)
{
  Assignment underTest{10_Lit};
  underTest.add(-7_Lit);
  underTest.add(3_Lit);
  ASSERT_THAT(underTest.get(7_Lit), t_false);
  ASSERT_THAT(underTest.get(3_Lit), t_true);

  underTest.clear();
  EXPECT_THAT(underTest, IsEmpty());
  ASSERT_THAT(underTest.get(7_Lit), t_indet);
  ASSERT_THAT(underTest.get(3_Lit), t_indet);
}

TEST(AssignmentTests, WhenAssignmentIsPartiallyCleared_NonClearedAssignmentsRemain)
{
  Assignment underTest{10_Lit};
  underTest.add(-7_Lit);
  underTest.add(3_Lit);
  underTest.add(5_Lit);
  underTest.add(6_Lit);

  underTest.clear(2);
  EXPECT_THAT(underTest.get(7_Lit), t_false);
  EXPECT_THAT(underTest.get(3_Lit), t_true);
  EXPECT_THAT(underTest.get(5_Lit), t_indet);
  EXPECT_THAT(underTest.get(6_Lit), t_indet);
}

// clang-format off
using AssignmentTests_RangeParams = std::tuple<
  std::vector<Lit>, // literals in assignment
  std::size_t, // range start index
  std::vector<Lit> // expected literals in range
>;
// clang-format on

class AssignmentTests_Range : public ::testing::TestWithParam<AssignmentTests_RangeParams> {
public:
  virtual ~AssignmentTests_Range() = default;
};

TEST_P(AssignmentTests_Range, checkExpectedRange)
{
  Assignment underTest{-100_Lit};
  for (Lit lit : std::get<0>(GetParam())) {
    underTest.add(lit);
  }

  auto result = underTest.range(std::get<1>(GetParam()));
  auto expected = std::get<2>(GetParam());

  EXPECT_THAT(result.size(), Eq(expected.size()));
  EXPECT_TRUE(std::equal(result.begin(), result.end(), expected.begin()));
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(, AssignmentTests_Range,
  ::testing::Values(
    std::make_tuple(std::vector<Lit>{}, 0, std::vector<Lit>{}),
    std::make_tuple(std::vector<Lit>{3_Lit, 4_Lit}, 2, std::vector<Lit>{}),
    std::make_tuple(std::vector<Lit>{3_Lit, 4_Lit}, 0, std::vector<Lit>{3_Lit, 4_Lit}),
    std::make_tuple(std::vector<Lit>{3_Lit, 4_Lit, -5_Lit, -6_Lit}, 2, std::vector<Lit>{-5_Lit, -6_Lit}),
    std::make_tuple(std::vector<Lit>{3_Lit, 4_Lit, -5_Lit, -6_Lit}, 3, std::vector<Lit>{-6_Lit})
  )
);
// clang-format on

TEST(AssignmentTests, AssignmentCanBeEnlarged)
{
  Assignment underTest{10_Lit};
  underTest.increaseSizeTo(-20_Lit);
  underTest.add(20_Lit);
  EXPECT_THAT(underTest.get(20_Lit), t_true);
}
}