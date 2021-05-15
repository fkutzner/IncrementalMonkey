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
  ClauseVerificationState initialState = ClauseVerificationState::Irredundant;
  std::vector<Lit> lits;
};

using TestClauses = std::vector<TestClause>;

struct CheckerInvocationSpec {
  size_t clauseIdx = 0;
  RUPCheckResult expected = RUPCheckResult::ViolatesRUP;
};

using CheckerInvocationSpecs = std::vector<CheckerInvocationSpec>;
using Assumptions = std::vector<Lit>;

struct RUPCheckerTestSpec {
  std::string description;
  Assumptions assumptions;
  std::vector<TestClause> proof;
  CheckerInvocationSpecs invocationSpecs;
};

auto operator<<(std::ostream& stream, RUPCheckerTestSpec const& testSpec) -> std::ostream&
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
    SetUpRUPChecker(testSpec.assumptions);
  }

  void SetUpClauseCollection(RUPCheckerTestSpec const& testSpec)
  {
    for (TestClause const& clause : testSpec.proof) {
      mClauseCollection.add(clause.lits, ClauseVerificationState::Passive, clause.addIndex);
    }
  }

  void SetUpRUPChecker(std::vector<Lit> const& assumptions)
  {
    mUnderTest = std::make_unique<RUPChecker>(mClauseCollection, assumptions);
  }

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

// clang-format off
INSTANTIATE_TEST_SUITE_P(RUPCheckerTest, RUPCheckerTest,
  ::testing::Values(

    RUPCheckerTestSpec {
      "Contradictory unary has RUP",
      Assumptions{},
      TestClauses {
        {1, ClauseVerificationState::Irredundant, {1_Lit}},
        {2, ClauseVerificationState::Passive, {1_Lit}}
      },
      CheckerInvocationSpecs {
        {1, RUPCheckResult::HasRUP},
      }
    },

    RUPCheckerTestSpec {
      "Multiple contradictory unaries with RUP",
      Assumptions{},
      TestClauses {
        {1, ClauseVerificationState::Irredundant, {1_Lit}},
        {2, ClauseVerificationState::Passive, {1_Lit}},
        {3, ClauseVerificationState::Passive, {-1_Lit}},
        {4, ClauseVerificationState::Passive, {1_Lit}},
        {5, ClauseVerificationState::Passive, {-1_Lit}},
        {6, ClauseVerificationState::Passive, {1_Lit}},
        {7, ClauseVerificationState::Passive, {-1_Lit}}
      },
      CheckerInvocationSpecs {
        {6, RUPCheckResult::HasRUP},
        {5, RUPCheckResult::HasRUP},
        {4, RUPCheckResult::HasRUP},
        {3, RUPCheckResult::HasRUP},
        {2, RUPCheckResult::ViolatesRUP},
        {1, RUPCheckResult::HasRUP},
        {0, RUPCheckResult::ViolatesRUP}
      }
    },

    RUPCheckerTestSpec {
      "Empty clause after contradictory unaries has RUP",
      Assumptions{},
      TestClauses {
        {1, ClauseVerificationState::Irredundant, {1_Lit}},
        {2, ClauseVerificationState::Irredundant, {-1_Lit}},
        {3, ClauseVerificationState::Passive, {}}
      },
      CheckerInvocationSpecs {
        {2, RUPCheckResult::HasRUP}
      }
    },

    RUPCheckerTestSpec {
      "Small real, correct, nontrivial RUP problem without non-empty passive clauses",
      Assumptions{},
      TestClauses {
        {1, ClauseVerificationState::Irredundant, {1_Lit, 3_Lit}},
        {1, ClauseVerificationState::Irredundant, {-3_Lit, 2_Lit}},
        {1, ClauseVerificationState::Irredundant, {-1_Lit, -2_Lit}},
        {1, ClauseVerificationState::Irredundant, {-1_Lit}},
        {1, ClauseVerificationState::Irredundant, {-2_Lit}},
        {2, ClauseVerificationState::Passive, {}}
      },
      CheckerInvocationSpecs {
        {5, RUPCheckResult::HasRUP}
      }
    },

    RUPCheckerTestSpec {
      "Minimal RUP problem without direct unary conflict (positive)",
      Assumptions{},
      TestClauses {
        {1, ClauseVerificationState::Irredundant, {1_Lit, 2_Lit}},
        {1, ClauseVerificationState::Irredundant, {-1_Lit, -2_Lit}},
        {1, ClauseVerificationState::Irredundant, {1_Lit, -2_Lit}},
        {1, ClauseVerificationState::Irredundant, {-1_Lit, 2_Lit}},
        {2, ClauseVerificationState::Passive, {1_Lit}}
      },
      CheckerInvocationSpecs {
        {4, RUPCheckResult::HasRUP}
      }
    },

    RUPCheckerTestSpec {
      "Minimal RUP problem without direct unary conflict (negative)",
      Assumptions{},
      TestClauses {
        {1, ClauseVerificationState::Irredundant, {1_Lit, 2_Lit}},
        {1, ClauseVerificationState::Irredundant, {-1_Lit, -2_Lit}},
        {1, ClauseVerificationState::Irredundant, {1_Lit, -2_Lit}},
        {2, ClauseVerificationState::Passive, {-1_Lit}}
      },
      CheckerInvocationSpecs {
        {3, RUPCheckResult::ViolatesRUP}
      }
    },

    RUPCheckerTestSpec {
      "Future clauses are ignored",
      Assumptions{},
      TestClauses {
        {1, ClauseVerificationState::Irredundant, {1_Lit, 2_Lit}},
        {1, ClauseVerificationState::Irredundant, {-1_Lit, -2_Lit}},
        {2, ClauseVerificationState::Passive, {2_Lit}},
        {3, ClauseVerificationState::Passive, {1_Lit, -2_Lit}},
        {4, ClauseVerificationState::Passive, {-1_Lit, 2_Lit}},
        {5, ClauseVerificationState::Passive, {1_Lit}},
        {6, ClauseVerificationState::Passive, {1_Lit}}
      },
      CheckerInvocationSpecs {
        {6, RUPCheckResult::HasRUP},
        {5, RUPCheckResult::HasRUP},
        {2, RUPCheckResult::ViolatesRUP}
      }
    },

    RUPCheckerTestSpec {
      "RUP problem with ternary clauses (positive)",
      Assumptions{},
      TestClauses {
        // Unsatisfiable problem instance
        {0, ClauseVerificationState::Irredundant, {1_Lit,  2_Lit, -3_Lit}},
        {0, ClauseVerificationState::Irredundant, {-1_Lit, -2_Lit, 3_Lit}},
        {0, ClauseVerificationState::Irredundant, {2_Lit, 3_Lit, -4_Lit}},
        {0, ClauseVerificationState::Irredundant, {-2_Lit, -3_Lit, 4_Lit}},
        {0, ClauseVerificationState::Irredundant, {1_Lit, 3_Lit, 4_Lit}},
        {0, ClauseVerificationState::Irredundant, {-1_Lit, -3_Lit, -4_Lit}},
        {0, ClauseVerificationState::Irredundant, {-1_Lit, 2_Lit, 4_Lit}},
        {0, ClauseVerificationState::Irredundant, {1_Lit, -2_Lit, -4_Lit}},

        // RUP proof for unsatisfiability
        {1, ClauseVerificationState::Passive, {1_Lit, 2_Lit}},
        {2, ClauseVerificationState::Passive, {1_Lit}},
        {3, ClauseVerificationState::Passive, {2_Lit}},
        {4, ClauseVerificationState::Passive, {}}
      },
      CheckerInvocationSpecs {
       {11, RUPCheckResult::HasRUP},
       {10, RUPCheckResult::HasRUP},
       {9, RUPCheckResult::HasRUP},
       {8, RUPCheckResult::HasRUP}
      }
    },

    RUPCheckerTestSpec {
      "RUP problem with ternary clauses (negative, due to bad proof ordering)",
      Assumptions{},
      TestClauses {
        // Unsatisfiable problem instance
        {0, ClauseVerificationState::Irredundant, {1_Lit,  2_Lit, -3_Lit}},
        {0, ClauseVerificationState::Irredundant, {-1_Lit, -2_Lit, 3_Lit}},
        {0, ClauseVerificationState::Irredundant, {2_Lit, 3_Lit, -4_Lit}},
        {0, ClauseVerificationState::Irredundant, {-2_Lit, -3_Lit, 4_Lit}},
        {0, ClauseVerificationState::Irredundant, {1_Lit, 3_Lit, 4_Lit}},
        {0, ClauseVerificationState::Irredundant, {-1_Lit, -3_Lit, -4_Lit}},
        {0, ClauseVerificationState::Irredundant, {-1_Lit, 2_Lit, 4_Lit}},
        {0, ClauseVerificationState::Irredundant, {1_Lit, -2_Lit, -4_Lit}},

        // Broken RUP proof for unsatisfiability: {1} is not RUP before
        // {1, 2} has been added
        {1, ClauseVerificationState::Passive, {1_Lit}},
        {2, ClauseVerificationState::Passive, {1_Lit, 2_Lit}},
        {3, ClauseVerificationState::Passive, {2_Lit}},
        {4, ClauseVerificationState::Passive, {}}
      },
      CheckerInvocationSpecs {
       {11, RUPCheckResult::HasRUP},
       {10, RUPCheckResult::HasRUP},
       {9, RUPCheckResult::HasRUP},
       {8, RUPCheckResult::ViolatesRUP}
      }
    },

    RUPCheckerTestSpec {
      "Minimal RUP problem without direct unary conflict, with assumptions (positive)",
      Assumptions{-5_Lit},
      TestClauses {
        {1, ClauseVerificationState::Irredundant, {5_Lit, 1_Lit, 2_Lit}},
        {1, ClauseVerificationState::Irredundant, {-1_Lit, -2_Lit}},
        {1, ClauseVerificationState::Irredundant, {1_Lit, -2_Lit}},
        {1, ClauseVerificationState::Irredundant, {-1_Lit, 2_Lit}},
        {2, ClauseVerificationState::Passive, {1_Lit}}
      },
      CheckerInvocationSpecs {
        {4, RUPCheckResult::HasRUP}
      }
    },

    RUPCheckerTestSpec {
      "Minimal RUP problem without direct unary conflict, with assumptions (negative)",
      Assumptions{5_Lit},
      TestClauses {
        {1, ClauseVerificationState::Irredundant, {5_Lit, 1_Lit, 2_Lit}},
        {1, ClauseVerificationState::Irredundant, {-1_Lit, -2_Lit}},
        {1, ClauseVerificationState::Irredundant, {1_Lit, -2_Lit}},
        {1, ClauseVerificationState::Irredundant, {-1_Lit, 2_Lit}},
        {2, ClauseVerificationState::Passive, {1_Lit}}
      },
      CheckerInvocationSpecs {
        {4, RUPCheckResult::ViolatesRUP}
      }
    },

    RUPCheckerTestSpec {
      "Problem with assumptions directly contradicting a clause literal",
      Assumptions{1_Lit},
      TestClauses {
        {1, ClauseVerificationState::Irredundant, {1_Lit, 3_Lit}},
        {1, ClauseVerificationState::Irredundant, {-3_Lit, 2_Lit}},
        {1, ClauseVerificationState::Irredundant, {-1_Lit, -2_Lit}},
        {1, ClauseVerificationState::Irredundant, {-1_Lit}},
        {2, ClauseVerificationState::Passive, {}}
      },
      CheckerInvocationSpecs {
        {4, RUPCheckResult::HasRUP}
      }
    },

    RUPCheckerTestSpec {
      "Problem with assumptions indirectly contradicting a clause literal",
      Assumptions{-1_Lit},
      TestClauses {
        {1, ClauseVerificationState::Irredundant, {1_Lit, 3_Lit}},
        {1, ClauseVerificationState::Irredundant, {-3_Lit, 2_Lit}},
        {1, ClauseVerificationState::Irredundant, {-1_Lit, -2_Lit}},
        {1, ClauseVerificationState::Irredundant, {-2_Lit}},
        {2, ClauseVerificationState::Passive, {}}
      },
      CheckerInvocationSpecs {
        {4, RUPCheckResult::HasRUP}
      }
    }
  ));
// clang-format off
}

