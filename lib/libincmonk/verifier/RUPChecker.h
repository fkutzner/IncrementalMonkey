#pragma once

#include <libincmonk/verifier/Assignment.h>
#include <libincmonk/verifier/Clause.h>

#include <limits>
#include <vector>

namespace incmonk::verifier {

enum RUPCheckResult { HasRUP, ViolatesRUP };

class RUPChecker {
public:
  explicit RUPChecker(ClauseCollection& clauses);

  auto checkRUP(gsl::span<Lit const> clause, ProofSequenceIdx index) -> RUPCheckResult;

private:
  enum ProofIndexUpdateResult {
    // Advancing the proof to some index may immediately result in a
    // conflict forced by unary clauses. In these regions of the proof,
    // all clauses have RUP.
    Conflict,
    NoConflict
  };

  void initializeProof();
  auto advanceProof(ProofSequenceIdx index) -> ProofIndexUpdateResult;
  auto propagateToFixpoint(Assignment::Range assignment) -> RUPCheckResult;


  struct Watcher {
    CRef m_watchedClause;
    Lit m_blocker;
  };

  ClauseCollection& m_clauses;

  /**
   * The list of clauses under current consideration. Clauses are popped off
   * this list as the proof progresses.
   */
  std::vector<CRef> m_clausesUnderConsideration;

  /**
   * The current assignment. Unary clause assignments are preserved between
   * proof steps.
   */
  Assignment m_assignment;

  /**
   * If a literal L is assigned false, all m_watchers[L] must be checked if
   * their clause forces an assignment.
   */
  BoundedMap<Lit, std::vector<Watcher>> m_watchers;

  /**
   * Reason clauses are clauses that forced an assignment. Due to the watcher
   * system, a reason clause always forces its first or second literal.
   */
  BoundedMap<Lit, OptCRef> m_reasons;

  /**
   * If the proof contains clauses {-x} and {x}, all clauses beyond them
   * have the RUP property. These unaries need to be checked as well, eventually.
   */
  std::vector<CRef> m_directUnaryConflicts;

  /**
   * The current proof sequence index, a monotonically decreasing value.
   */
  ProofSequenceIdx m_currentProofSequenceIndex = std::numeric_limits<ProofSequenceIdx>::max();
};
}
