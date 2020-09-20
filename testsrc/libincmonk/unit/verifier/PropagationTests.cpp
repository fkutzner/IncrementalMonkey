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

#include <libincmonk/verifier/Propagation.h>

#include <libincmonk/verifier/Clause.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>


namespace incmonk::verifier {

namespace {
struct InputClause {
  ProofSequenceIdx addedIdx = 0;
  ProofSequenceIdx deletedIdx = 0;
  ClauseVerificationState state = ClauseVerificationState::Passive;
  std::vector<Lit> lits;
};

enum Outcome { Conflict, NoConflict };

struct PropagationResult {
  Outcome outcome;

  // If a variable V is specified in `propagations`, either V or -V is expected
  // to be part of the assignment after propagation
  std::vector<std::variant<Lit, Var>> propagations;

  std::vector<size_t> clausesMarkedForVerification;
};

struct PropagationCall {
  ProofSequenceIdx callIdx = 0;
  std::vector<Lit> toPropagate;
  PropagationResult expectedResult;
};

using PropagationTestParam =
    std::tuple<std::string, std::vector<InputClause>, std::vector<PropagationCall>>;

class PropagationTests : public ::testing::TestWithParam<PropagationTestParam> {
public:
  virtual ~PropagationTests() = default;
};

namespace {
template <typename T>
auto contains(std::unordered_set<T> const& set, T const& item) noexcept -> bool
{
  return set.find(item) != set.end();
}

MATCHER_P(AssignmentMatches, expected, "")
{
  if (expected.size() != arg.size()) {
    return false;
  }

  std::unordered_set<Lit> argSet{arg.begin(), arg.end()};

  for (std::variant<Lit, Var> expectedItem : expected) {
    if (Lit* lit = std::get_if<Lit>(&expectedItem); lit != nullptr) {
      if (!contains(argSet, *lit)) {
        return false;
      }
    }
    else if (Var* var = std::get_if<Var>(&expectedItem); var != nullptr) {
      if (!contains(argSet, Lit{*var, true}) && !contains(argSet, Lit{*var, false})) {
        return false;
      }
    }
  }

  return true;
}

class CRefDeletionCompare {
public:
  CRefDeletionCompare(ClauseCollection const& clauses) : m_clauses{clauses} {}

  auto operator()(CRef lhs, CRef rhs) const noexcept -> bool
  {
    Clause const& lhsClause = m_clauses.resolve(lhs);
    Clause const& rhsClause = m_clauses.resolve(rhs);

    return lhsClause.getDelIdx() < rhsClause.getDelIdx();
  }

private:
  ClauseCollection const& m_clauses;
};

void addDeletions(ClauseCollection& clauses, std::vector<CRef> deletedClauses)
{
  std::sort(deletedClauses.begin(), deletedClauses.end(), CRefDeletionCompare{clauses});
  for (CRef delRef : deletedClauses) {
    Clause const& delClause = clauses.resolve(delRef);
    clauses.markDeleted(delRef, delClause.getDelIdx());
  }
}

}

TEST_P(PropagationTests, TestSuite)
{
  std::vector<InputClause> inputClauses = std::get<1>(GetParam());
  std::vector<PropagationCall> calls = std::get<2>(GetParam());

  ClauseCollection clauses;

  std::unordered_map<CRef, std::size_t> clauseIndices;
  std::vector<CRef> deletedClauses;

  std::size_t currentIndex = 0;
  for (InputClause const& input : inputClauses) {
    CRef cref = clauses.add(input.lits, input.state, input.addedIdx);
    clauses.resolve(cref).setDelIdx(input.deletedIdx);
    clauseIndices[cref] = currentIndex;
    ++currentIndex;

    if (input.deletedIdx < std::numeric_limits<ProofSequenceIdx>::max()) {
      deletedClauses.push_back(cref);
    }
  }

  addDeletions(clauses, deletedClauses);

  Lit const maxLit = 100_Lit;
  Assignment assignment{maxLit};
  Propagator underTest{clauses, assignment, maxLit};

  std::size_t callIndex = 0;
  for (PropagationCall const& call : calls) {
    assignment.clear();

    for (Lit toPropagate : call.toPropagate) {
      assignment.add(toPropagate);
    }

    Assignment::size_type const numAssignmentsBeforeProp = assignment.size();

    std::vector<CRef> newObligations;
    OptCRef conflict =
        underTest.propagateToFixpoint(assignment.range().begin(), call.callIdx, newObligations);

    std::string failMessage = "Failed at call " + std::to_string(callIndex);

    EXPECT_EQ(conflict.has_value(), call.expectedResult.outcome == Outcome::Conflict)
        << failMessage;
    EXPECT_THAT(assignment.range(numAssignmentsBeforeProp),
                AssignmentMatches(call.expectedResult.propagations))
        << failMessage;

    std::vector<std::size_t> newObligationsIdx;
    for (CRef obligation : newObligations) {
      newObligationsIdx.push_back(clauseIndices[obligation]);
    }
    EXPECT_THAT(
        newObligationsIdx,
        ::testing::UnorderedElementsAreArray(call.expectedResult.clausesMarkedForVerification))
        << failMessage;

    ++callIndex;
  }
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(, PropagationTests,
  ::testing::Values(
    std::make_tuple(
      "When nothing is propagated, no assignments are added",
      std::vector<InputClause> {
        {5, 13, ClauseVerificationState::Passive, {1_Lit, -2_Lit}},
        {4, 13, ClauseVerificationState::Passive, {3_Lit}}
      },
      std::vector<PropagationCall> {
        {15, {}, PropagationResult{Outcome::NoConflict, {}, {}}}
      }
    ),

    std::make_tuple(
      "Core binaries with addition index before current index are used in propagation",
      std::vector<InputClause> {
        {5, 13, ClauseVerificationState::Verified, {10_Lit, -2_Lit}},
        {4, 13, ClauseVerificationState::Irrendundant, {11_Lit, -2_Lit}},
        {3, 13, ClauseVerificationState::VerificationPending, {12_Lit, -2_Lit}},
        {2, 13, ClauseVerificationState::VerificationPending, {2_Lit, -12_Lit}}
      },
      std::vector<PropagationCall> {
        {8, {2_Lit}, PropagationResult{Outcome::NoConflict, {10_Lit, 11_Lit, 12_Lit}, {}}},
        {7, {2_Lit}, PropagationResult{Outcome::NoConflict, {10_Lit, 11_Lit, 12_Lit}, {}}}
      }
    ),

    std::make_tuple(
      "Core binaries with future addition index are not used for propagation",
      std::vector<InputClause> {
        {5, 13, ClauseVerificationState::Verified, {10_Lit, -2_Lit}},
        {4, 13, ClauseVerificationState::Irrendundant, {11_Lit, -2_Lit}},
        {3, 13, ClauseVerificationState::VerificationPending, {12_Lit, -2_Lit}}
      },
      std::vector<PropagationCall> {
        {3, {2_Lit}, PropagationResult{Outcome::NoConflict, {}, {}}}
      }
    ),

    std::make_tuple(
      "Far binaries with addition index before current index are used in propagation",
      std::vector<InputClause> {
        {5, 13, ClauseVerificationState::Passive, {10_Lit, -2_Lit}},
        {4, 13, ClauseVerificationState::Passive, {11_Lit, -2_Lit}},
        {3, 13, ClauseVerificationState::Passive, {12_Lit, -2_Lit}},
        {2, 13, ClauseVerificationState::Passive, {2_Lit, -12_Lit}}
      },
      std::vector<PropagationCall> {
        {8, {2_Lit}, PropagationResult{Outcome::NoConflict, {10_Lit, 11_Lit, 12_Lit}, {}}},
        {7, {2_Lit}, PropagationResult{Outcome::NoConflict, {10_Lit, 11_Lit, 12_Lit}, {}}}
      }
    ),

    std::make_tuple(
      "Far binaries with future addition index are not used for propagation",
      std::vector<InputClause> {
        {5, 13, ClauseVerificationState::Passive, {10_Lit, -2_Lit}},
        {4, 13, ClauseVerificationState::Passive, {11_Lit, -2_Lit}},
        {3, 13, ClauseVerificationState::Passive, {12_Lit, -2_Lit}}
      },
      std::vector<PropagationCall> {
        {3, {2_Lit}, PropagationResult{Outcome::NoConflict, {}, {}}}
      }
    ),

    std::make_tuple(
      "Far binary propagations trigger core binary propagations",
      std::vector<InputClause> {
        {5, 13, ClauseVerificationState::Passive, {10_Lit, -2_Lit}},
        {4, 13, ClauseVerificationState::Irrendundant, {-10_Lit, -3_Lit}},
        {3, 13, ClauseVerificationState::Passive, {3_Lit, -20_Lit}}
      },
      std::vector<PropagationCall> {
        {8, {2_Lit}, PropagationResult{Outcome::NoConflict, {10_Lit, -3_Lit, -20_Lit}, {}}}
      }
    ),

    std::make_tuple(
      "Conflicts in binaries are reported & new verification work is marked",
      std::vector<InputClause> {
        {5, 13, ClauseVerificationState::Passive, {10_Lit, -2_Lit}},
        {4, 13, ClauseVerificationState::VerificationPending, {-10_Lit, -3_Lit}},
        {3, 13, ClauseVerificationState::Passive, {3_Lit, -2_Lit}}
      },
      std::vector<PropagationCall> {
        {8, {2_Lit}, PropagationResult{Outcome::Conflict, {10_Lit, 3_Lit}, {0, 2}}}
      }
    ),

    std::make_tuple(
      "Conflicts in far unaries are reported & new verification work is marked",
      std::vector<InputClause> {
        {5, 13, ClauseVerificationState::Passive, {10_Lit, -2_Lit}},
        {4, 13, ClauseVerificationState::Passive, {-10_Lit}}
      },
      std::vector<PropagationCall> {
        {8, {2_Lit}, PropagationResult{Outcome::Conflict, {10_Lit}, {0, 1}}}
      }
    ),

    std::make_tuple(
      "Conflicts in core unaries are reported",
      std::vector<InputClause> {
        {5, 13, ClauseVerificationState::Irrendundant, {10_Lit, -2_Lit}},
        {4, 13, ClauseVerificationState::VerificationPending, {-10_Lit}}
      },
      std::vector<PropagationCall> {
        {8, {2_Lit}, PropagationResult{Outcome::Conflict, {10_Lit}, {}}}
      }
    ),

    std::make_tuple(
      "Long core clauses are used for propagation",
      std::vector<InputClause> {
        {5, 13, ClauseVerificationState::Irrendundant, {1_Lit, -2_Lit, 3_Lit}},
        {4, 13, ClauseVerificationState::Irrendundant, {-1_Lit, -2_Lit, 4_Lit}},
        {3, 13, ClauseVerificationState::Irrendundant, {-1_Lit, -2_Lit, 5_Lit}},
      },
      std::vector<PropagationCall> {
        {8, {2_Lit, -3_Lit}, PropagationResult{Outcome::NoConflict, {1_Lit, 4_Lit, 5_Lit}, {}}},
        {8, {2_Lit, -3_Lit}, PropagationResult{Outcome::NoConflict, {1_Lit, 4_Lit, 5_Lit}, {}}},
        {8, {2_Lit, -1_Lit}, PropagationResult{Outcome::NoConflict, {3_Lit}, {}}}
      }
    ),

    std::make_tuple(
      "Long far clauses are used for propagation",
      std::vector<InputClause> {
        {5, 13, ClauseVerificationState::Passive, {1_Lit, -2_Lit, 3_Lit}},
        {4, 13, ClauseVerificationState::Passive, {-1_Lit, -2_Lit, 4_Lit}},
        {3, 13, ClauseVerificationState::Passive, {-1_Lit, -2_Lit, 5_Lit}},
      },
      std::vector<PropagationCall> {
        {7, {2_Lit, -3_Lit}, PropagationResult{Outcome::NoConflict, {1_Lit, 4_Lit, 5_Lit}, {}}},
        {7, {2_Lit, -3_Lit}, PropagationResult{Outcome::NoConflict, {1_Lit, 4_Lit, 5_Lit}, {}}},
        {7, {2_Lit, -1_Lit}, PropagationResult{Outcome::NoConflict, {3_Lit}, {}}}
      }
    ),

    std::make_tuple(
      "Far clauses used in conflict are used as core clauses afterwards",
      std::vector<InputClause> {
        {5, 13, ClauseVerificationState::Passive, {1_Lit, -2_Lit, 3_Lit}},
        {4, 13, ClauseVerificationState::Passive, {1_Lit, -2_Lit, -3_Lit}},
      },
      std::vector<PropagationCall> {
        {7, {2_Lit, -1_Lit}, PropagationResult{Outcome::Conflict, {3_Var}, {0, 1}}},
        {7, {2_Lit, -1_Lit}, PropagationResult{Outcome::Conflict, {3_Var}, {}}}
      }
    ),

    std::make_tuple(
      "Clauses are marked as verification work are distinct",
      std::vector<InputClause> {
        {6, 13, ClauseVerificationState::Passive, {1_Lit, -2_Lit, 3_Lit}},
        {5, 13, ClauseVerificationState::Passive, {-3_Lit, 4_Lit}},
        {4, 13, ClauseVerificationState::Irrendundant, {-3_Lit, 5_Lit}},
        {3, 13, ClauseVerificationState::Passive, {-4_Lit, -5_Lit, 6_Lit}},
        {2, 13, ClauseVerificationState::Passive, {-6_Lit}},
      },
      std::vector<PropagationCall> {
        {7, {2_Lit, -1_Lit}, PropagationResult{Outcome::Conflict, {3_Var, 4_Var, 5_Var, 6_Var}, {0, 1, 3, 4}}},
        {7, {2_Lit, -1_Lit}, PropagationResult{Outcome::Conflict, {3_Var, 4_Var, 5_Var, 6_Var}, {}}}
      }
    ),

    std::make_tuple(
      "Deleted clauses are inactive",
      std::vector<InputClause> {
        {6, 13, ClauseVerificationState::Passive, {1_Lit, -2_Lit}},
        {4, 10, ClauseVerificationState::Irrendundant, {1_Lit, 2_Lit, 3_Lit}},
        {2, 7, ClauseVerificationState::Passive, {5_Lit, -2_Lit, 3_Lit}},
        {3, 1, ClauseVerificationState::Passive, {6_Lit, -2_Lit, 3_Lit}}
      },
      std::vector<PropagationCall> {
        {14, {2_Lit, -1_Lit}, PropagationResult{Outcome::NoConflict, {}, {}}},
        {13, {2_Lit, -1_Lit}, PropagationResult{Outcome::NoConflict, {}, {}}},
        {12, {2_Lit, -1_Lit}, PropagationResult{Outcome::Conflict, {}, {0}}},
        {10, {-1_Lit, -2_Lit}, PropagationResult{Outcome::NoConflict, {}, {}}},
        {9, {-1_Lit, -2_Lit}, PropagationResult{Outcome::NoConflict, {3_Lit}, {}}},
        {7, {2_Lit, -1_Lit}, PropagationResult{Outcome::Conflict, {}, {}}},
        {5, {-2_Lit, -1_Lit}, PropagationResult{Outcome::NoConflict, {3_Lit}, {}}},
        {3, {-1_Lit, -2_Lit}, PropagationResult{Outcome::NoConflict, {}, {}}}
      }
    ),

    std::make_tuple(
      "Clauses with max deletion time are not considered deleted",
      std::vector<InputClause> {
        {6, std::numeric_limits<ProofSequenceIdx>::max(), ClauseVerificationState::Passive, {1_Lit, -2_Lit}}
      },
      std::vector<PropagationCall> {
        {14, {2_Lit}, PropagationResult{Outcome::NoConflict, {1_Lit}, {}}}
      }
    ),

    std::make_tuple(
      "Far clauses that are used in conflict are used for propagation as core clauses",
      std::vector<InputClause> {
        {6, std::numeric_limits<ProofSequenceIdx>::max(), ClauseVerificationState::Passive, {1_Lit, -2_Lit}}
      },
      std::vector<PropagationCall> {
        {14, {-1_Lit, 2_Lit}, PropagationResult{Outcome::Conflict, {}, {0}}},
        {14, {2_Lit}, PropagationResult{Outcome::NoConflict, {1_Lit}, {}}}
      }
    ),

    std::make_tuple(
      "Unaries are deactivated in propagations preceding their time of addition",
      std::vector<InputClause> {
        {6, std::numeric_limits<ProofSequenceIdx>::max(), ClauseVerificationState::Passive, {-2_Lit}}
      },
      std::vector<PropagationCall> {
        {5, {2_Lit}, PropagationResult{Outcome::NoConflict, {}, {}}}
      }
    )
  )
);
// clang-format on

}
}