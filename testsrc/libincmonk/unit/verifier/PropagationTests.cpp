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

#include <unordered_set>
#include <utility>
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
  std::vector<Lit> propagations;
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

TEST_P(PropagationTests, TestSuite)
{
  std::vector<InputClause> inputClauses = std::get<1>(GetParam());
  std::vector<PropagationCall> calls = std::get<2>(GetParam());

  ClauseCollection clauses;

  for (InputClause const& input : inputClauses) {
    CRef cref = clauses.add(input.lits, input.state, input.addedIdx);
    clauses.resolve(cref).setDelIdx(input.deletedIdx);
  }

  Lit const maxLit = 100_Lit;
  Assignment assignment{maxLit};
  Propagator underTest{clauses, assignment, maxLit};

  for (PropagationCall const& call : calls) {
    assignment.clear();

    for (Lit toPropagate : call.toPropagate) {
      assignment.add(toPropagate);
    }

    Assignment::size_type const numAssignmentsBeforePropagation = assignment.size();

    std::vector<CRef> newObligations;
    OptCRef conflict =
        underTest.propagateToFixpoint(assignment.range().begin(), call.callIdx, newObligations);

    EXPECT_EQ(conflict.has_value(), call.expectedResult.outcome == Outcome::Conflict);
    Assignment::Range newAssignmentsRng = assignment.range(numAssignmentsBeforePropagation);
    std::vector<Lit> newAssignments{newAssignmentsRng.begin(), newAssignmentsRng.end()};
    EXPECT_THAT(newAssignments,
                ::testing::UnorderedElementsAreArray(call.expectedResult.propagations));

    // TODO: also test verification work
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
        {10, {}, PropagationResult{Outcome::NoConflict, {}, {}}}
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
        {10, {2_Lit}, PropagationResult{Outcome::NoConflict, {10_Lit, 11_Lit, 12_Lit}, {}}},
        {9, {2_Lit}, PropagationResult{Outcome::NoConflict, {10_Lit, 11_Lit, 12_Lit}, {}}}
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
        {10, {2_Lit}, PropagationResult{Outcome::NoConflict, {10_Lit, 11_Lit, 12_Lit}, {}}},
        {9, {2_Lit}, PropagationResult{Outcome::NoConflict, {10_Lit, 11_Lit, 12_Lit}, {}}}
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
        {6, {2_Lit}, PropagationResult{Outcome::NoConflict, {10_Lit, -3_Lit, -20_Lit}, {}}}
      }
    ),

    std::make_tuple(
      "Conflicts in binaries are reported",
      std::vector<InputClause> {
        {5, 13, ClauseVerificationState::Passive, {10_Lit, -2_Lit}},
        {4, 13, ClauseVerificationState::VerificationPending, {-10_Lit, -3_Lit}},
        {3, 13, ClauseVerificationState::Passive, {3_Lit, -2_Lit}}
      },
      std::vector<PropagationCall> {
        {7, {2_Lit}, PropagationResult{Outcome::Conflict, {10_Lit, 3_Lit}, {}}}
      }
    ),

    std::make_tuple(
      "Conflicts in far unaries are reported",
      std::vector<InputClause> {
        {5, 13, ClauseVerificationState::Passive, {10_Lit, -2_Lit}},
        {4, 13, ClauseVerificationState::Passive, {-10_Lit}}
      },
      std::vector<PropagationCall> {
        {7, {2_Lit}, PropagationResult{Outcome::Conflict, {10_Lit}, {}}}
      }
    ),

    std::make_tuple(
      "Conflicts in core unaries are reported",
      std::vector<InputClause> {
        {5, 13, ClauseVerificationState::Irrendundant, {10_Lit, -2_Lit}},
        {4, 13, ClauseVerificationState::VerificationPending, {-10_Lit}}
      },
      std::vector<PropagationCall> {
        {7, {2_Lit}, PropagationResult{Outcome::Conflict, {10_Lit}, {}}}
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
        {7, {2_Lit, -3_Lit}, PropagationResult{Outcome::NoConflict, {1_Lit, 4_Lit, 5_Lit}, {}}},
        {7, {2_Lit, -3_Lit}, PropagationResult{Outcome::NoConflict, {1_Lit, 4_Lit, 5_Lit}, {}}},
        {7, {2_Lit, -1_Lit}, PropagationResult{Outcome::NoConflict, {3_Lit}, {}}}
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
        {7, {2_Lit, -1_Lit}, PropagationResult{Outcome::Conflict, {3_Lit}, {}}},
        {7, {2_Lit, -1_Lit}, PropagationResult{Outcome::Conflict, {3_Lit}, {}}}
      }
    )
  )
);
// clang-format on

}
}