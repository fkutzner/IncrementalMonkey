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
#include <numeric>
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

class ClauseAllocator_allocationTests : public ::testing::TestWithParam<std::vector<CNFLit>> {
public:
  virtual ~ClauseAllocator_allocationTests() = default;
};

namespace {
void insertClausesAndCheck(ClauseAllocator& underTest,
                           std::vector<std::vector<CNFLit>> const& inputClauses)
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

TEST_P(ClauseAllocator_allocationTests, clauseIsAllocatedCorrectlyInSingleAllocation)
{
  ClauseAllocator underTest;
  insertClausesAndCheck(underTest, {GetParam()});
}

TEST_P(ClauseAllocator_allocationTests, clauseIsAllocatedCorrectlyInMultipleAllocations)
{
  ClauseAllocator underTest;
  std::vector<std::vector<CNFLit>> inputClauses = {
      {1, -4, -8, 9}, {}, {2, 7, 11}, GetParam(), {20, 40, 60, -80, 100, 120, -140}};
  insertClausesAndCheck(underTest, inputClauses);
}

namespace {
auto createIotaClause(Clause::size_type size) -> std::vector<CNFLit>
{
  std::vector<CNFLit> result;
  result.resize(size);
  std::iota(result.begin(), result.end(), 4);
  return result;
}
}

TEST_P(ClauseAllocator_allocationTests, clauseRefsRemainValidAfterDoublingResize)
{
  ClauseAllocator underTest;

  static std::vector<CNFLit> const hugeClause = createIotaClause(1 << 20);
  std::vector<std::vector<CNFLit>> inputClauses = {
      {1, -4, -8, 9}, {}, {2, 7, 11}, GetParam(), hugeClause, {20, 40, 60, -80, 100, 120, -140}};

  insertClausesAndCheck(underTest, inputClauses);
}

TEST_P(ClauseAllocator_allocationTests, clauseRefsRemainValidAfterExtraLargeResize)
{
  ClauseAllocator underTest;

  // The allocator can't provide enough memory by doubling the allocation alone
  static std::vector<CNFLit> const hugeClause = createIotaClause(1 << 21);
  std::vector<std::vector<CNFLit>> inputClauses = {
      {1, -4, -8, 9}, {}, {2, 7, 11}, GetParam(), hugeClause, {20, 40, 60, -80, 100, 120, -140}};

  insertClausesAndCheck(underTest, inputClauses);
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(, ClauseAllocator_allocationTests,
  ::testing::Values(
    std::vector<CNFLit>{},
    std::vector<CNFLit>{1},
    std::vector<CNFLit>{-1},
    std::vector<CNFLit>{-1, 2},
    std::vector<CNFLit>{-1, 1024, 6},
    std::vector<CNFLit>{-1, -1, 2, 3, 4, 5, 6, 7, 8, 9, 10}
  )
);
// clang-format on


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

TEST(ClauseTests, whenClauseStateIsSet_itsValueChanges)
{
  ClauseAllocator underTest;
  CRef clauseRef =
      underTest.allocate(std::vector<CNFLit>{1, 2, 3}, ClauseVerificationState::Irrendundant, 1);
  Clause& clause = underTest.resolve(clauseRef);
  checkClauseStateStore(clause);
}

TEST(BinaryClauseTests, whenClauseStateIsSet_itsValueChanges)
{
  BinaryClause clause{15, ClauseVerificationState::Irrendundant, 1};
  checkClauseStateStore(clause);
}
}
