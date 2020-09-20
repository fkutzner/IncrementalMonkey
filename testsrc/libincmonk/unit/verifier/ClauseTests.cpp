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
using ::testing::IsEmpty;
using ::testing::UnorderedElementsAre;

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

class ClauseCollection_AllocationTests : public ::testing::TestWithParam<std::vector<Lit>> {
public:
  virtual ~ClauseCollection_AllocationTests() = default;
};

namespace {
void insertClausesAndCheck(ClauseCollection& underTest,
                           std::vector<std::vector<Lit>> const& inputClauses)
{
  std::vector<ClauseCollection::Ref> allocatedRefs;
  for (auto const& clause : inputClauses) {
    allocatedRefs.push_back(
        underTest.add(clause, ClauseVerificationState::Irrendundant, clause.size()));
  }

  std::size_t idx = 0;
  for (ClauseCollection::Ref ref : allocatedRefs) {
    Clause const& cl = underTest.resolve(ref);
    ASSERT_THAT(cl, ClauseEq(inputClauses[idx], ClauseVerificationState::Irrendundant, cl.size()));
    ++idx;
  }
}
}

TEST_P(ClauseCollection_AllocationTests, ClauseIsAllocatedCorrectlyInSingleAllocation)
{
  ClauseCollection underTest;
  insertClausesAndCheck(underTest, {GetParam()});
}

TEST_P(ClauseCollection_AllocationTests, ClauseIsAllocatedCorrectlyInMultipleAllocations)
{
  ClauseCollection underTest;
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

TEST_P(ClauseCollection_AllocationTests, ClauseRefsRemainValidAfterDoublingResize)
{
  ClauseCollection underTest;

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

TEST_P(ClauseCollection_AllocationTests, ClauseRefsRemainValidAfterExtraLargeResize)
{
  ClauseCollection underTest;

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
INSTANTIATE_TEST_SUITE_P(, ClauseCollection_AllocationTests,
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

class ClauseCollection_IterationTests
  : public ::testing::TestWithParam<std::vector<std::vector<Lit>>> {
public:
  virtual ~ClauseCollection_IterationTests() = default;
};

TEST_P(ClauseCollection_IterationTests, iteratedRefsPointToCorrectClause)
{
  ClauseCollection underTest;

  std::vector<CRef> inputClauseRefs;
  for (std::vector<Lit> const& input : GetParam()) {
    inputClauseRefs.push_back(underTest.add(input, ClauseVerificationState::Passive, 0));
  }

  std::vector<CRef> iteratedClauseRefs{underTest.begin(), underTest.end()};
  EXPECT_THAT(iteratedClauseRefs, Eq(inputClauseRefs));
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(, ClauseCollection_IterationTests,
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


TEST(ClauseCollection_FindTests, WhenClauseDBIsEmpty_NoClauseIsFound)
{
  ClauseCollection underTest;
  EXPECT_THAT(underTest.find({}), Eq(std::nullopt));
  EXPECT_THAT(underTest.find(std::vector<Lit>{10_Lit}), Eq(std::nullopt));
}

TEST(ClauseCollection_FindTests, WhenClauseDBContainsSingleClause_ItIsReturnedByFind)
{
  ClauseCollection underTest;
  CRef clause =
      underTest.add(std::vector<Lit>{10_Lit, 20_Lit}, ClauseVerificationState::Irrendundant, 0);
  EXPECT_THAT(underTest.find(std::vector<Lit>{10_Lit}), Eq(std::nullopt));
  EXPECT_THAT(underTest.find(std::vector<Lit>{20_Lit, 10_Lit}), Eq(clause));
  EXPECT_THAT(underTest.find(std::vector<Lit>{10_Lit, 20_Lit}), Eq(clause));
}

TEST(ClauseCollection_FindTests, WhenAddIsCalledAfterFind_NewClausesCanBeFound)
{
  ClauseCollection underTest;
  auto const irredundant = ClauseVerificationState::Irrendundant;
  CRef clause1 = underTest.add(std::vector<Lit>{10_Lit, 20_Lit}, irredundant, 0);
  EXPECT_THAT(underTest.find(std::vector<Lit>{10_Lit}), Eq(std::nullopt));
  CRef clause2 = underTest.add(std::vector<Lit>{1_Lit, -20_Lit, 5_Lit}, irredundant, 0);

  EXPECT_THAT(underTest.find(std::vector<Lit>{20_Lit, 10_Lit}), Eq(clause1));
  EXPECT_THAT(underTest.find(std::vector<Lit>{1_Lit, -20_Lit, 5_Lit}), Eq(clause2));
  EXPECT_THAT(underTest.find(std::vector<Lit>{10_Lit, 20_Lit, 5_Lit}), Eq(std::nullopt));
  EXPECT_THAT(underTest.find(std::vector<Lit>{-20_Lit, 5_Lit, 1_Lit}), Eq(clause2));
}

TEST(ClauseCollection_OccurrenceTests, WhenClauseDBIsEmpty_NoOccurrencesAreFound)
{
  ClauseCollection underTest;
  EXPECT_THAT(underTest.getOccurrences({2_Lit}), IsEmpty());
}

namespace {
template <typename T>
auto toVec(gsl::span<T const> span) -> std::vector<T>
{
  return std::vector<T>{span.begin(), span.end()};
}
}

TEST(ClauseCollection_OccurrenceTests, WhenClauseDBContainsOneClause_OnlyItsLiteralsAreInOccMap)
{
  ClauseCollection underTest;
  auto const irredundant = ClauseVerificationState::Irrendundant;
  CRef clause1 = underTest.add(std::vector<Lit>{1_Lit, -20_Lit, 5_Lit}, irredundant, 0);

  EXPECT_THAT(toVec(underTest.getOccurrences(2_Lit)), IsEmpty());
  EXPECT_THAT(toVec(underTest.getOccurrences(1_Lit)), UnorderedElementsAre(clause1));
  EXPECT_THAT(toVec(underTest.getOccurrences(-1_Lit)), IsEmpty());
  EXPECT_THAT(toVec(underTest.getOccurrences(-20_Lit)), UnorderedElementsAre(clause1));
  EXPECT_THAT(toVec(underTest.getOccurrences(20_Lit)), IsEmpty());
  EXPECT_THAT(toVec(underTest.getOccurrences(5_Lit)), UnorderedElementsAre(clause1));
  EXPECT_THAT(toVec(underTest.getOccurrences(-5_Lit)), IsEmpty());
}

TEST(ClauseCollection_OccurrenceTests, WhenClauseDBContainsOneClause_ItsLiteralsAreInOccMap)
{
  ClauseCollection underTest;
  auto const irredundant = ClauseVerificationState::Irrendundant;
  CRef clause1 = underTest.add(std::vector<Lit>{1_Lit, -20_Lit, 5_Lit}, irredundant, 0);

  EXPECT_THAT(toVec(underTest.getOccurrences(1_Lit)), UnorderedElementsAre(clause1));
  EXPECT_THAT(toVec(underTest.getOccurrences(-20_Lit)), UnorderedElementsAre(clause1));
  EXPECT_THAT(toVec(underTest.getOccurrences(5_Lit)), UnorderedElementsAre(clause1));

  CRef clause2 = underTest.add(std::vector<Lit>{1_Lit, 20_Lit, 3_Lit}, irredundant, 0);

  EXPECT_THAT(toVec(underTest.getOccurrences(1_Lit)), UnorderedElementsAre(clause1, clause2));
  EXPECT_THAT(toVec(underTest.getOccurrences(-20_Lit)), UnorderedElementsAre(clause1));
  EXPECT_THAT(toVec(underTest.getOccurrences(20_Lit)), UnorderedElementsAre(clause2));
  EXPECT_THAT(toVec(underTest.getOccurrences(5_Lit)), UnorderedElementsAre(clause1));
  EXPECT_THAT(toVec(underTest.getOccurrences(3_Lit)), UnorderedElementsAre(clause2));
}


TEST(ClauseRefIteratorTests, DefaultConstructedIteratorsAreEq)
{
  ClauseCollection::RefIterator iter1{};
  ClauseCollection::RefIterator iter2{};

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
  ClauseCollection underTest;
  CRef clauseRef = underTest.add(
      std::vector<Lit>{1_Lit, 2_Lit, 3_Lit}, ClauseVerificationState::Irrendundant, 1);
  Clause& clause = underTest.resolve(clauseRef);
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
