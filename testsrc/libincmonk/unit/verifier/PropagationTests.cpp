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

using ::testing::UnorderedElementsAreArray;

namespace incmonk::verifier {

namespace {

// *************************
// Test parameter structures
// *************************

// Representation of a clause added as part of the test setup
struct InputClause {
  ProofSequenceIdx addedIdx = 0;
  ProofSequenceIdx deletedIdx = 0;
  ClauseVerificationState state = ClauseVerificationState::Passive;
  std::vector<Lit> lits;
};

auto operator<<(std::ostream& stream, InputClause const&) -> std::ostream&
{
  stream << "<InputClause>"; // test names are printed on failure, don't flood the screen
  return stream;
}

enum Outcome { Conflict, NoConflict };

using PropagatedLits = std::vector<std::variant<Lit, Var>>;
using ClaMarkedForVerify = std::vector<size_t>;

// The expected propagation result
struct PropagationResult {
  Outcome outcome;

  // If a variable V is specified in `propagatedLits`, either V or -V is expected
  // to be part of the assignment after propagation
  PropagatedLits propagatedLits;

  // Indices of (wrt the list of InputClause objects) of clauses which have entered
  // the VerificationPending state during propagation
  ClaMarkedForVerify clausesMarkedForVerification;
};

using LitsOnTrail = std::vector<Lit>;
using LitsToPropagate = std::vector<Lit>;

// Representation of a propagation call, including the expected result.
// A propagation test consists of a sequence of propagation calls.
struct PropagationCall {
  ProofSequenceIdx callIdx = 0;
  LitsOnTrail litsOnTrail;     // literals that are put on the trail and won't be propagated
  LitsToPropagate toPropagate; // literals that are put on the trail and will be propagated
  PropagationResult expectedResult;
};

auto operator<<(std::ostream& stream, PropagationCall const&) -> std::ostream&
{
  stream << "<PropagationCall>"; // test names are printed on failure, don't flood the screen
  return stream;
}

// clang-format off
using PropagationTestParam = std::tuple<
  std::string, // test description
  std::vector<InputClause>, // the clauses to add before applying the propagation calls 
  std::vector<PropagationCall> // the sequence of propagation calls (containing test expectations)
>;
// clang-format on


// *************************
// Test fixture
// *************************

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


// Checks whether an assignment given as a vector of literals matches
// the expected assignment, which is given as a vector of std::variant<Lit,Var>
// (with variables meaning that the variable must be assigned, but the truth
// value doesn't matter)
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

// Orders clause references by their deletion index (ascending)
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

// Registers all deletions given in `deletedClauses` with the clause collection
void addDeletions(ClauseCollection& clauses, std::vector<CRef> deletedClauses)
{
  std::sort(deletedClauses.begin(), deletedClauses.end(), CRefDeletionCompare{clauses});
  for (CRef delRef : deletedClauses) {
    Clause const& delClause = clauses.resolve(delRef);
    clauses.markDeleted(delRef, delClause.getDelIdx());
  }
}

auto createClauseCollection(std::vector<InputClause> const& inputClauses)
    -> std::pair<std::unordered_map<CRef, std::size_t>, ClauseCollection>
{

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

  return std::make_pair(std::move(clauseIndices), std::move(clauses));
}

// Helper class for obtaining clauses that have been additionally marked as
// verification work
class VerificationWorkCollector {
public:
  explicit VerificationWorkCollector(ClauseCollection const& clauses) : m_clauses{clauses}
  {
    for (CRef cref : m_clauses) {
      Clause const& clause = m_clauses.resolve(cref);
      if (clause.getState() == ClauseVerificationState::VerificationPending) {
        m_previousWork.insert(cref);
      }
    }
  }

  // Returns refs to all clauses that have entered the ClauseVerificationState::VerificationPending
  // state since the last call to getNewClausesPendingVerification()
  std::vector<CRef> getNewClausesPendingVerification() const
  {
    std::vector<CRef> result;

    for (CRef cref : m_clauses) {
      Clause const& clause = m_clauses.resolve(cref);
      bool const isPending = clause.getState() == ClauseVerificationState::VerificationPending;
      if (auto it = m_previousWork.find(cref); isPending && it == m_previousWork.end()) {
        result.push_back(cref);
      }
    }

    return result;
  }

private:
  ClauseCollection const& m_clauses;
  std::unordered_set<CRef> m_previousWork;
};

// Helper class for determining which unaries should be part of the assignment, given a proof
// sequence index
class UnaryScheduler {
public:
  explicit UnaryScheduler(ClauseCollection const& clauses)
    : m_clauses{clauses}, m_currentProofSequenceIdx{std::numeric_limits<ProofSequenceIdx>::max()}
  {
  }

  struct AdvanceProofResult {
    std::vector<Lit> currentUnaries;
    std::vector<CRef> unariesToAdd;
    std::vector<CRef> unariesToDismiss;
  };

  auto advanceProof(ProofSequenceIdx newIdx) -> AdvanceProofResult
  {
    assert(newIdx <= m_currentProofSequenceIdx);

    AdvanceProofResult result;

    ProofSequenceIdx const oldProofSequenceIdx = m_currentProofSequenceIdx;

    for (CRef cref : m_clauses) {
      Clause const& clause = m_clauses.resolve(cref);
      if (clause.size() != 1) {
        continue;
      }

      if (clause.getAddIdx() <= oldProofSequenceIdx && clause.getAddIdx() > newIdx) {
        // The unary was active during the previous propagation, but is not anymore
        result.unariesToDismiss.push_back(cref);
      }
      else if (clause.getAddIdx() <= newIdx && clause.getDelIdx() > newIdx) {
        result.currentUnaries.push_back(clause[0]);
        if (clause.getDelIdx() < oldProofSequenceIdx) {
          result.unariesToAdd.push_back(cref);
        }
      }
    }

    m_currentProofSequenceIdx = newIdx;
    return result;
  }

private:
  ClauseCollection const& m_clauses;
  ProofSequenceIdx m_currentProofSequenceIdx;
};
}

TEST_P(PropagationTests, TestSuite)
{
  std::vector<InputClause> inputClauses = std::get<1>(GetParam());
  std::vector<PropagationCall> calls = std::get<2>(GetParam());

  auto [clauseIndices, clauses] = createClauseCollection(inputClauses);
  Var const maxVar = 100_Var;
  Assignment assignment{maxVar};
  UnaryScheduler unaryScheduler{clauses};
  Propagator underTest{clauses, assignment, maxVar};

  std::size_t callIndex = 0;
  for (PropagationCall const& call : calls) {
    VerificationWorkCollector newPendingClauses{clauses};

    assignment.clear();
    UnaryScheduler::AdvanceProofResult unaryChanges = unaryScheduler.advanceProof(call.callIdx);
    assignment.add(unaryChanges.currentUnaries);

    assignment.add(call.litsOnTrail);
    std::size_t const litsOnTrailSize = assignment.size();

    assignment.add(call.toPropagate);
    std::size_t const trailSizeBeforePropagation = assignment.size();

    for (CRef unaryToActivate : unaryChanges.unariesToAdd) {
      underTest.activateUnary(unaryToActivate);
    }
    for (CRef unaryToDismiss : unaryChanges.unariesToDismiss) {
      underTest.dismissUnary(unaryToDismiss);
    }

    OptCRef conflict = underTest.advanceProof(call.callIdx);
    if (!conflict.has_value()) {
      auto const propStart = assignment.range().begin() + litsOnTrailSize;
      conflict = underTest.propagateToFixpoint(propStart);
    }

    std::string const failMessage = "Failed at call " + std::to_string(callIndex);

    bool const expectedOutcomeIsConflict = call.expectedResult.outcome == Outcome::Conflict;
    EXPECT_EQ(conflict.has_value(), expectedOutcomeIsConflict) << failMessage;

    auto const expectedPropagations = call.expectedResult.propagatedLits;
    EXPECT_THAT(assignment.range(trailSizeBeforePropagation),
                AssignmentMatches(expectedPropagations))
        << failMessage;

    // Check the clauses additionally marked for verification:
    std::vector<std::size_t> newObligationsIdx;
    for (CRef obligation : newPendingClauses.getNewClausesPendingVerification()) {
      newObligationsIdx.push_back(clauseIndices[obligation]);
    }
    auto const expectedVerifWork = call.expectedResult.clausesMarkedForVerification;
    EXPECT_THAT(newObligationsIdx, UnorderedElementsAreArray(expectedVerifWork)) << failMessage;

    ++callIndex;
  }
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(, PropagationTests,
  ::testing::Values(
    std::make_tuple(
      "When no clauses are present, then propagation does not add any literals to the assignment",
      std::vector<InputClause>{},
      std::vector<PropagationCall> {
        PropagationCall {
          ProofSequenceIdx{0}, LitsOnTrail{10_Lit}, LitsToPropagate{5_Lit},
          PropagationResult {
            Outcome::NoConflict,
            PropagatedLits{},
            ClaMarkedForVerify{}
          }
        }
      }
    ),

    std::make_tuple(
      "When a size-3 clause is added before current index, then it is used for propagation",
      std::vector<InputClause>{
        InputClause {
          ProofSequenceIdx{0},
          ProofSequenceIdx{10},
          ClauseVerificationState::Passive,
          std::vector<Lit>{1_Lit, -2_Lit, 3_Lit}
        }
      },
      std::vector<PropagationCall> {
        PropagationCall {
          ProofSequenceIdx{5},LitsOnTrail{-3_Lit}, LitsToPropagate{-1_Lit},
          PropagationResult {
            Outcome::NoConflict,
            PropagatedLits{-2_Lit},
            ClaMarkedForVerify{} // No conflict -> no new verification work
          }
        }
      }
    ),

    std::make_tuple(
      "When a size-3 clause is deleted at current index, then it is used for propagation",
      std::vector<InputClause>{
        InputClause {
          ProofSequenceIdx{0},
          ProofSequenceIdx{10},
          ClauseVerificationState::Passive,
          std::vector<Lit>{1_Lit, -2_Lit, 3_Lit}
        }
      },
      std::vector<PropagationCall> {
        PropagationCall {
          ProofSequenceIdx{10},LitsOnTrail{-3_Lit}, LitsToPropagate{-1_Lit},
          PropagationResult {
            Outcome::NoConflict,
            PropagatedLits{-2_Lit},
            ClaMarkedForVerify{} // No conflict -> no new verification work
          }
        }
      }
    ),

    std::make_tuple(
      "When a size-3 clause is deleted before current index, then it is not used for propagation",
      std::vector<InputClause>{
        InputClause {
          ProofSequenceIdx{0},
          ProofSequenceIdx{10},
          ClauseVerificationState::Passive,
          std::vector<Lit>{1_Lit, -2_Lit, 3_Lit}
        }
      },
      std::vector<PropagationCall> {
        PropagationCall {
          ProofSequenceIdx{11},LitsOnTrail{-3_Lit}, LitsToPropagate{-1_Lit},
          PropagationResult {
            Outcome::NoConflict,
            PropagatedLits{},
            ClaMarkedForVerify{}
          }
        }
      }
    ),

    std::make_tuple(
      "When a size-3 clause is added at current index, then it is not used for propagation",
      std::vector<InputClause>{
        InputClause {
          ProofSequenceIdx{4},
          ProofSequenceIdx{10},
          ClauseVerificationState::Passive,
          std::vector<Lit>{1_Lit, -2_Lit, 3_Lit}
        }
      },
      std::vector<PropagationCall> {
        PropagationCall {
          ProofSequenceIdx{4},LitsOnTrail{-3_Lit}, LitsToPropagate{-1_Lit},
          PropagationResult {
            Outcome::NoConflict,
            PropagatedLits{},
            ClaMarkedForVerify{}
          }
        }
      }
    ),

    std::make_tuple(
      "When a size-3 clause is added before current index, then it is not used for propagation",
      std::vector<InputClause>{
        InputClause {
          ProofSequenceIdx{4},
          ProofSequenceIdx{10},
          ClauseVerificationState::Passive,
          std::vector<Lit>{1_Lit, -2_Lit, 3_Lit}
        }
      },
      std::vector<PropagationCall> {
        PropagationCall {
          ProofSequenceIdx{2},LitsOnTrail{-3_Lit}, LitsToPropagate{-1_Lit},
          PropagationResult {
            Outcome::NoConflict,
            PropagatedLits{},
            ClaMarkedForVerify{}
          }
        }
      }
    ),

    std::make_tuple(
      "When a clause is part of a conflict, then it is marked as verification work",
      std::vector<InputClause>{
        InputClause {
          ProofSequenceIdx{4},
          ProofSequenceIdx{10},
          ClauseVerificationState::Passive,
          std::vector<Lit>{1_Lit, -2_Lit, -3_Lit, 4_Lit}
        },

        InputClause {
          ProofSequenceIdx{4},
          ProofSequenceIdx{10},
          ClauseVerificationState::Passive,
          std::vector<Lit>{1_Lit, -2_Lit, -3_Lit, -4_Lit}
        },

        InputClause { // irrelevant clause, should not be marked for verification
          ProofSequenceIdx{4},
          ProofSequenceIdx{10},
          ClauseVerificationState::Passive,
          std::vector<Lit>{1_Lit, 2_Lit, -3_Lit, -4_Lit}
        }
      },
      std::vector<PropagationCall> {
        PropagationCall {
          ProofSequenceIdx{6},LitsOnTrail{-1_Lit}, LitsToPropagate{2_Lit, 3_Lit},
          PropagationResult {
            Outcome::Conflict,
            PropagatedLits{},
            ClaMarkedForVerify{0, 1}
          }
        }
      }
    )
  )
);
// clang-format on
}
}