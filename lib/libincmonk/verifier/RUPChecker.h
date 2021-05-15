#pragma once

#include <libincmonk/verifier/Assignment.h>
#include <libincmonk/verifier/Clause.h>

#include <limits>
#include <vector>

namespace incmonk::verifier {

class RUPChecker {
public:
  /**
   * Constructs a RUP checker for the given clauses and assumptions. The
   * assumptions are treated as additional unary clauses until reset() is
   * called with a different set of assumptions.
   */
  explicit RUPChecker(ClauseCollection& clauses, gsl::span<Lit const> assumptions);

  /**
   * Returns true iff the given clause has the RUP property regarding the
   * clauses contained in the clause collection passed to the checker during
   * construction, taking only clauses with `addIndex` < `index` into account.
   *
   * `index` must be monotonically decreasing across invocations of isRUP, except
   * directly after calling reset(),
   */
  auto isRUP(gsl::span<Lit const> clause, ProofSequenceIdx index) -> bool;

  /**
   * Resets the RUP checker with a new set of assumptions. This allows isRUP()
   * to be called with a larger proof sequence index than at its last invocation.
   */
  void reset(gsl::span<Lit const> assumptions);


private:
  void setupWatchers();
  void initializeProof(gsl::span<Lit const> assumptions);

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
   * The current variable assignment.
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

  /**
   * List of current assumptions (withot cref) and problem unaries (with cref).
   */
  std::vector<std::pair<Lit, std::optional<CRef>>> m_unaries;
};
}
