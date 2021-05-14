#include <libincmonk/verifier/RUPChecker.h>

#include <cassert>

namespace incmonk::verifier {
RUPChecker::RUPChecker(ClauseCollection& clauses)
  : m_clauses{clauses}
  , m_assignment{clauses.getMaxVar()}
  , m_watchers{maxLit(clauses.getMaxVar())}
  , m_reasons{maxLit(clauses.getMaxVar())}
{
  setupWatchers();
  initializeProof();
}

void RUPChecker::setupWatchers()
{
  for (CRef const& clauseRef : m_clauses) {
    Clause const& clause = m_clauses.resolve(clauseRef);
    if (clause.size() > 1) {
      m_watchers[-clause[0]].push_back(Watcher{clauseRef, clause[1]});
      m_watchers[-clause[1]].push_back(Watcher{clauseRef, clause[0]});
    }
  }
}

void RUPChecker::initializeProof()
{
  for (CRef const& cref : m_clauses) {
    Clause& clause = m_clauses.resolve(cref);

    if (clause.size() == 1) {
      Lit const unaryLit = clause[0];

      if (m_assignment.get(unaryLit) == t_indet) {
        m_assignment.add(unaryLit);
        m_reasons[unaryLit] = cref;
      }
      else if (m_assignment.get(unaryLit) == t_false) {
        m_directUnaryConflicts.push_back(cref);
      }
    }
  }
}

namespace {
class OnExitScope {
public:
  explicit OnExitScope(std::function<void()> const& fn) : m_fn{fn} {}
  ~OnExitScope() { m_fn(); }

private:
  std::function<void()> m_fn;
};
}

auto RUPChecker::checkRUP(gsl::span<Lit const> clause, ProofSequenceIdx index) -> RUPCheckResult
{
  assert(index <= m_currentProofSequenceIndex &&
         "The proof sequence idx must decrease monotonically");

  if (advanceProof(index) == AdvanceProofResult::UnaryConflict) {
    return RUPCheckResult::HasRUP;
  }

  size_t const numAssignmentsAtStart = m_assignment.size();

  bool const hasRUP = std::any_of(clause.begin(), clause.end(), [this](Lit const& lit) {
    return assignAndPropagateToFixpoint(-lit, std::nullopt) == PropagateResult::Conflict;
  });

  for (Lit const& toClear : m_assignment.range(numAssignmentsAtStart)) {
    m_reasons[toClear] = std::nullopt;
  }
  m_assignment.clear(numAssignmentsAtStart);

  return hasRUP ? RUPCheckResult::HasRUP : RUPCheckResult::ViolatesRUP;
}

auto RUPChecker::assignAndPropagateToFixpoint(Lit toPropagate, std::optional<CRef> reason)
    -> PropagateResult
{
  TBool currentAssignment = m_assignment.get(toPropagate);
  if (currentAssignment == t_false) {
    return PropagateResult::Conflict;
  }
  else if (currentAssignment == t_true) {
    return PropagateResult::NoConflict;
  }
  else {
    size_t propagationIdx = m_assignment.size();
    m_assignment.add(toPropagate);
    m_reasons[toPropagate] = reason;

    auto work = m_assignment.range(propagationIdx);
    while (work.begin() != work.end()) {
      PropagateResult const intermediateResult = propagate(*work.begin());
      if (intermediateResult == PropagateResult::Conflict) {
        return PropagateResult::Conflict;
      }
      ++propagationIdx;
      work = m_assignment.range(propagationIdx);
    }
  }

  return PropagateResult::NoConflict;
}


auto RUPChecker::propagate(Lit lit) -> PropagateResult
{
  // Possible future optimizations:
  // - propagate binaries separately
  // - core-first propagation

  std::vector<Watcher>& watchers = m_watchers[lit];

  auto watcherIt = watchers.begin();
  auto watcherEnd = watchers.begin() + watchers.size();
  OnExitScope eraseWatchers{
      [&watchers, &watcherEnd]() { watchers.erase(watcherEnd, watchers.end()); }};

  while (watcherIt != watcherEnd) {
    Watcher& watcher = *watcherIt;
    Clause& clause = m_clauses.resolve(watcher.m_watchedClause);

    if (m_assignment.get(watcher.m_blocker) == t_true) {
      ++watcherIt;
      continue;
    }

    size_t watcherIndex = ((clause[0] == -lit) ? 0 : 1);

    watcher.m_blocker = clause[1 - watcherIndex];

    if (m_assignment.get(watcher.m_blocker) == t_true) {
      ++watcherIt;
      continue;
    }

    if (clause.getAddIdx() >= m_currentProofSequenceIndex) {
      --watcherEnd;
      std::iter_swap(watcherIt, watcherEnd);
      continue;
    }

    bool clauseIsDeterminate = true;
    for (size_t idx = 2; idx < clause.size(); ++idx) {
      Lit& current = clause[idx];
      if (m_assignment.get(current) != t_false) {
        clauseIsDeterminate = false;

        std::swap(clause[watcherIndex], clause[idx]);
        m_watchers[-clause[watcherIndex]].push_back(watcher);
        --watcherEnd;

        std::iter_swap(watcherIt, watcherEnd);
      }
    }

    if (clauseIsDeterminate) {
      if (m_assignment.get(watcher.m_blocker) == t_false) {
        // All literals of the clause are false
        return PropagateResult::Conflict;
      }
      else {
        // The blocker is the last non-assigned literal, so its assignment is
        // forced:
        m_assignment.add(watcher.m_blocker);
        m_reasons[watcher.m_blocker] = watcher.m_watchedClause;
        ++watcherIt;
      }
    }
  }

  return PropagateResult::NoConflict;
}

auto RUPChecker::advanceProof(ProofSequenceIdx index) -> AdvanceProofResult
{
  bool recomputeUnariesRequired = (m_currentProofSequenceIndex == std::numeric_limits<ProofSequenceIdx>::max());
  m_currentProofSequenceIndex = index;

  auto staleDirectConflicts = std::remove_if(
      m_directUnaryConflicts.begin(), m_directUnaryConflicts.end(), [this, index](CRef const& ref) {
        return m_clauses.resolve(ref).getAddIdx() >= index;
      });
  m_directUnaryConflicts.erase(staleDirectConflicts, m_directUnaryConflicts.end());

  if (!m_directUnaryConflicts.empty()) {
    return AdvanceProofResult::UnaryConflict;
  }

  std::vector<CRef> unaries;
  for (Lit const& lit : m_assignment.range(0)) {
    CRef const reasonRef = *m_reasons[lit];
    Clause const& reason = m_clauses.resolve(reasonRef);
    if (reason.getAddIdx() >= index) {
      recomputeUnariesRequired = true;
    }
    else if (reason.size() == 1) {
      unaries.push_back(reasonRef);
    }
  }

  if (recomputeUnariesRequired) {
    m_assignment.clear(0);

    for (CRef const& unaryRef : unaries) {
      Clause const& reason = m_clauses.resolve(unaryRef);
      Lit const unaryLit = reason[0];
      if (assignAndPropagateToFixpoint(unaryLit, unaryRef) == PropagateResult::Conflict) {
        return AdvanceProofResult::UnaryConflict;
      }
    }
  }

  return AdvanceProofResult::NoConflict;
}
}

