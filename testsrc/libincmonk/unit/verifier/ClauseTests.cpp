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

#include <libincmonk/verifier/Clause.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <unordered_set>
#include <vector>

using ::testing::Eq;

namespace incmonk::verifier {

MATCHER_P3(ClauseEq, expectedLits, expectedState, expectedAddIdx, "")
{
  if (arg.getState() != expectedState) {
    return false;
  }

  if (arg.getAddIdx() != expectedAddIdx) {
    return false;
  }

  if (expectedLits.size() != arg.size()) {
    return false;
  }

  auto lits = arg.getLiterals();
  return std::equal(lits.begin(), lits.end(), expectedLits.begin());
}

class ClauseAllocator_AllocationTests : public ::testing::TestWithParam<std::vector<Lit>> {
public:
  virtual ~ClauseAllocator_AllocationTests() = default;
};

namespace {
void insertClausesAndCheck(ClauseAllocator& underTest,
                           std::vector<std::vector<Lit>> const& inputClauses)
{
  std::vector<ClauseAllocator::Ref> allocatedRefs;
  for (auto const& clause : inputClauses) {
    allocatedRefs.push_back(
        underTest.allocate(clause, ClauseVerificationState::Irrendundant, clause.size()));
  }

  std::size_t idx = 0;
  for (ClauseAllocator::Ref ref : allocatedRefs) {
    Clause const& cl = underTest.resolve(ref);
    ASSERT_THAT(cl, ClauseEq(inputClauses[idx], ClauseVerificationState::Irrendundant, cl.size()));
    ++idx;
  }
}
}

TEST_P(ClauseAllocator_AllocationTests, ClauseIsAllocatedCorrectlyInSingleAllocation)
{
  ClauseAllocator underTest;
  insertClausesAndCheck(underTest, {GetParam()});
}

TEST_P(ClauseAllocator_AllocationTests, ClauseIsAllocatedCorrectlyInMultipleAllocations)
{
  ClauseAllocator underTest;
  std::vector<std::vector<Lit>> inputClauses = {
      {1_Lit, -4_Lit, -8_Lit, 9_Lit},
      {},
      {2_Lit, 7_Lit, 11_Lit},
      GetParam(),
      {20_Lit, 40_Lit, 60_Lit, -80_Lit, 100_Lit, 120_Lit, -140_Lit}};
  insertClausesAndCheck(underTest, inputClauses);
}

namespace {
auto createIotaClause(Clause::size_type size) -> std::vector<Lit>
{
  std::vector<Lit> result;
  uint32_t var = 4;

  std::generate_n(std::back_inserter(result), size, [&var] {
    ++var;
    return Lit{Var{var}, true};
  });

  return result;
}
}

TEST_P(ClauseAllocator_AllocationTests, ClauseRefsRemainValidAfterDoublingResize)
{
  ClauseAllocator underTest;

  static std::vector<Lit> const hugeClause = createIotaClause(1 << 20);
  std::vector<std::vector<Lit>> inputClauses = {
      {1_Lit, -4_Lit, -8_Lit, 9_Lit},
      {},
      {2_Lit, 7_Lit, 11_Lit},
      GetParam(),
      hugeClause,
      {20_Lit, 40_Lit, 60_Lit, -80_Lit, 100_Lit, 120_Lit, -140_Lit}};

  insertClausesAndCheck(underTest, inputClauses);
}

TEST_P(ClauseAllocator_AllocationTests, ClauseRefsRemainValidAfterExtraLargeResize)
{
  ClauseAllocator underTest;

  // The allocator can't provide enough memory by doubling the allocation alone
  static std::vector<Lit> const hugeClause = createIotaClause(1 << 21);
  std::vector<std::vector<Lit>> inputClauses = {
      {1_Lit, -4_Lit, -8_Lit, 9_Lit},
      {},
      {2_Lit, 7_Lit, 11_Lit},
      GetParam(),
      hugeClause,
      {20_Lit, 40_Lit, 60_Lit, -80_Lit, 100_Lit, 120_Lit, -140_Lit}};

  insertClausesAndCheck(underTest, inputClauses);
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(, ClauseAllocator_AllocationTests,
  ::testing::Values(
    std::vector<Lit>{},
    std::vector<Lit>{1_Lit},
    std::vector<Lit>{-1_Lit},
    std::vector<Lit>{-1_Lit, 2_Lit},
    std::vector<Lit>{-1_Lit, 1024_Lit, 6_Lit},
    std::vector<Lit>{-1_Lit, -1_Lit, 2_Lit, 3_Lit, 4_Lit, 5_Lit, 6_Lit, 7_Lit, 8_Lit, 9_Lit, 10_Lit}
  )
);
// clang-format on

class ClauseAllocator_IterationTests
  : public ::testing::TestWithParam<std::vector<std::vector<Lit>>> {
public:
  virtual ~ClauseAllocator_IterationTests() = default;
};

TEST_P(ClauseAllocator_IterationTests, iteratedRefsPointToCorrectClause)
{
  ClauseAllocator underTest;

  std::vector<CRef> inputClauseRefs;
  for (std::vector<Lit> const& input : GetParam()) {
    inputClauseRefs.push_back(underTest.allocate(input, ClauseVerificationState::Passive, 0));
  }

  std::vector<CRef> iteratedClauseRefs{underTest.begin(), underTest.end()};
  EXPECT_THAT(iteratedClauseRefs, Eq(inputClauseRefs));
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(, ClauseAllocator_IterationTests,
  ::testing::Values(
    std::vector<std::vector<Lit>>{},
    std::vector<std::vector<Lit>>{{}},
    std::vector<std::vector<Lit>>{{1_Lit}},
    std::vector<std::vector<Lit>>{{-1_Lit, 2_Lit}},
    std::vector<std::vector<Lit>>{{-1_Lit, 2_Lit}, {}},
    std::vector<std::vector<Lit>>{{-1_Lit, 2_Lit}, {1_Lit}},
    std::vector<std::vector<Lit>>{{-1_Lit, 2_Lit}, {1_Lit, 2_Lit}},
    std::vector<std::vector<Lit>>{{-1_Lit, 2_Lit}, {}, {1_Lit, 2_Lit}},
    std::vector<std::vector<Lit>>{{-1_Lit, 2_Lit}, {2_Lit}, {1_Lit, 2_Lit}},
    std::vector<std::vector<Lit>>{{-1_Lit, 2_Lit}, {2_Lit, 3_Lit}, {1_Lit, 2_Lit}},
    std::vector<std::vector<Lit>>{{-1_Lit, 2_Lit}, {4_Lit, 6_Lit, 8_Lit, 10_Lit}, {1_Lit, 2_Lit}}
  )
);
// clang-format on

TEST(ClauseRefIteratorTests, DefaultConstructedIteratorsAreEq)
{
  ClauseAllocator::RefIterator iter1{};
  ClauseAllocator::RefIterator iter2{};

  EXPECT_TRUE(iter1 == iter2);
  EXPECT_FALSE(iter1 != iter2);
}


namespace {
template <typename Clause>
void checkClauseStateStore(Clause& clause)
{
  EXPECT_THAT(clause.getState(), Eq(ClauseVerificationState::Irrendundant));
  clause.setState(ClauseVerificationState::Verified);
  EXPECT_THAT(clause.getState(), Eq(ClauseVerificationState::Verified));
  clause.setState(ClauseVerificationState::Passive);
  EXPECT_THAT(clause.getState(), Eq(ClauseVerificationState::Passive));
}
}

TEST(ClauseTests, WhenClauseStateIsSet_itsValueChanges)
{
  ClauseAllocator underTest;
  CRef clauseRef = underTest.allocate(
      std::vector<Lit>{1_Lit, 2_Lit, 3_Lit}, ClauseVerificationState::Irrendundant, 1);
  Clause& clause = underTest.resolve(clauseRef);
  checkClauseStateStore(clause);
}

TEST(BinaryClauseTests, WhenClauseStateIsSet_itsValueChanges)
{
  BinaryClause clause{15_Lit, ClauseVerificationState::Irrendundant, 1};
  checkClauseStateStore(clause);
}

TEST(LitTests, WhenLitIsNegative_thenKeyMatchesItToAdjacentPositiveValue)
{
  Lit input = 5_Lit;
  std::size_t pos = Key<Lit>::get(input);
  std::size_t neg = Key<Lit>::get(-input);
  EXPECT_THAT(neg, ::testing::Gt(0));
  EXPECT_THAT(pos, Eq(neg + 1));
}

TEST(LitTests, DistinctLitsHaveDistinctKeys)
{
  std::unordered_set<std::size_t> keys{Key<Lit>::get(1_Lit),
                                       Key<Lit>::get(-1_Lit),
                                       Key<Lit>::get(0_Lit),
                                       Key<Lit>::get(-0_Lit),
                                       Key<Lit>::get(4_Lit),
                                       Key<Lit>::get(-4_Lit)};
  EXPECT_THAT(keys, ::testing::SizeIs(6));
}
}
