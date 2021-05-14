#include <libincmonk/verifier/RUPChecker.h>

#include <libincmonk/verifier/Clause.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>
#include <ostream>
#include <string>
#include <vector>


namespace incmonk::verifier {
namespace {
struct TestClause {
  size_t addIndex = 0;
  ClauseVerificationState initialState = ClauseVerificationState::Irrendundant;
  std::vector<Lit> lits;
};

using TestClauses = std::vector<TestClause>;

struct CheckerInvocationSpec {
  size_t clauseIdx = 0;
  RUPCheckResult expected = RUPCheckResult::ViolatesRUP;
};

using CheckerInvocationSpecs = std::vector<CheckerInvocationSpec>;

struct RUPCheckerTestSpec {
  std::string description;
  std::vector<TestClause> proof;
  CheckerInvocationSpecs invocationSpecs;
};

std::ostream& operator<<(std::ostream& stream, RUPCheckerTestSpec const& testSpec)
{
  stream << "Test \"" << testSpec.description << "\"";
  return stream;
}
}

class RUPCheckerTest : public ::testing::TestWithParam<RUPCheckerTestSpec> {
public:
  void SetUp() override
  {
    RUPCheckerTestSpec const& testSpec = GetParam();
    SetUpClauseCollection(testSpec);
    SetUpRUPChecker();
  }

  void SetUpClauseCollection(RUPCheckerTestSpec const& testSpec)
  {
    for (TestClause const& clause : testSpec.proof) {
      mClauseCollection.add(clause.lits, ClauseVerificationState::Passive, clause.addIndex);
    }
  }

  void SetUpRUPChecker() { mUnderTest = std::make_unique<RUPChecker>(mClauseCollection); }

protected:
  ClauseCollection mClauseCollection;
  std::unique_ptr<RUPChecker> mUnderTest;
};

TEST_P(RUPCheckerTest, Check)
{
  RUPCheckerTestSpec const& testSpec = GetParam();

  for (auto it = testSpec.invocationSpecs.begin(); it != testSpec.invocationSpecs.end(); ++it) {
    CheckerInvocationSpec const& invocation = *it;
    TestClause const& testClause = testSpec.proof[invocation.clauseIdx];
    auto const result = mUnderTest->checkRUP(testClause.lits, testClause.addIndex);
    EXPECT_THAT(result, ::testing::Eq(invocation.expected))
        << "Failed at invocation spec index "
        << std::distance(testSpec.invocationSpecs.begin(), it);
  }
}

constexpr ProofSequenceIdx MaxProofIdx = std::numeric_limits<ProofSequenceIdx>::max();

// clang-format off
INSTANTIATE_TEST_SUITE_P(RUPCheckerTest, RUPCheckerTest,
  ::testing::Values(
   
    RUPCheckerTestSpec {
      "Contradictory unary has RUP",
      TestClauses {
        {1, ClauseVerificationState::Irrendundant, {1_Lit}},
        {MaxProofIdx, ClauseVerificationState::Passive, {-1_Lit}}
      },
      CheckerInvocationSpecs {
        {1, RUPCheckResult::HasRUP},
      }
    },

    RUPCheckerTestSpec {
      "Multiple contradictory unaries with RUP",
      TestClauses {
        {1, ClauseVerificationState::Irrendundant, {1_Lit}},
        {2, ClauseVerificationState::Passive, {-1_Lit}},
        {3, ClauseVerificationState::Passive, {1_Lit}},
        {4, ClauseVerificationState::Passive, {1_Lit}},
        {5, ClauseVerificationState::Passive, {-1_Lit}},
        {6, ClauseVerificationState::Passive, {1_Lit}},
        {MaxProofIdx, ClauseVerificationState::Passive, {-1_Lit}}
      },
      CheckerInvocationSpecs {
        {6, RUPCheckResult::HasRUP},
        {5, RUPCheckResult::HasRUP},
        {4, RUPCheckResult::HasRUP},
        {3, RUPCheckResult::HasRUP},
        {2, RUPCheckResult::HasRUP},
        {1, RUPCheckResult::HasRUP},
        {0, RUPCheckResult::ViolatesRUP}
      }
    },

    RUPCheckerTestSpec {
      "Empty clause after contradictory unaries has RUP",
      TestClauses {
        {1, ClauseVerificationState::Irrendundant, {1_Lit}},
        {2, ClauseVerificationState::Irrendundant, {-1_Lit}},
        {MaxProofIdx, ClauseVerificationState::Passive, {}}
      },
      CheckerInvocationSpecs {
        {2, RUPCheckResult::HasRUP}
      }
    }

  ));
// clang-format off
}

