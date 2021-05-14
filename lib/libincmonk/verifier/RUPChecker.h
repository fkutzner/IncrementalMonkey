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
  void setupWatchers();
  void initializeProof();

  enum class AdvanceProofResult { UnaryConflict, NoConflict };
  auto advanceProof(ProofSequenceIdx index) -> AdvanceProofResult;

  enum class PropagateResult { Conflict, NoConflict };
  auto assignAndPropagateToFixpoint(Lit lit, std::optional<CRef> reason) -> PropagateResult;
  auto propagate(Lit lit) -> PropagateResult;


  struct Watcher {
    CRef m_watchedClause;
    Lit m_blocker;
  };

  ClauseCollection& m_clauses;

  /**
   * The current assignment. Unary clause assignments are preserved between
   * proof steps.
   */
  Assignment m_assignment;

  /**
   * If a literal L is assigned true, all m_watchers[L] must be checked if
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
