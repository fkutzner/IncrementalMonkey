#include <libincmonk/verifier/RUPChecker.h>

#include <cassert>

namespace incmonk::verifier {
RUPChecker::RUPChecker(ClauseCollection& clauses, gsl::span<Lit const> assumptions)
  : m_clauses{clauses}
  , m_assignment{clauses.getMaxVar()}
  , m_watchers{maxLit(clauses.getMaxVar())}
  , m_reasons{maxLit(clauses.getMaxVar())}
{
  reset(assumptions);
}

void RUPChecker::setupWatchers()
{
  for (CRef const& clauseRef : m_clauses) {
    Clause const& clause = m_clauses.resolve(clauseRef);

    if (m_currentProofSequenceIndex <= clause.getAddIdx()) {
      continue;
    }

    if (clause.size() > 1) {
      m_watchers[-clause[0]].push_back(Watcher{clauseRef, clause[1]});
      m_watchers[-clause[1]].push_back(Watcher{clauseRef, clause[0]});
    }
  }
}

void RUPChecker::initializeProof(gsl::span<Lit const> assumptions)
{
  m_unaries.clear();
  m_assignment.clear(0);

  for (Lit const& assumption : assumptions) {
    m_unaries.emplace_back(assumption, std::nullopt);
    m_assignment.add(assumption);
  }

  for (CRef const& cref : m_clauses) {
    Clause& clause = m_clauses.resolve(cref);

    if (m_currentProofSequenceIndex <= clause.getAddIdx()) {
      continue;
    }

    if (clause.size() == 1) {
      Lit const unaryLit = clause[0];

      if (m_assignment.get(unaryLit) == t_indet) {
        m_assignment.add(unaryLit);
        m_reasons[unaryLit] = cref;
        m_unaries.emplace_back(unaryLit, cref);
      }
      else if (m_assignment.get(unaryLit) == t_false) {
        m_directUnaryConflicts.push_back(cref);
      }
    }
  }

  m_assignment.clear(0);
}

void RUPChecker::reset(gsl::span<Lit const> assumptions)
{
  setupWatchers();
  initializeProof(assumptions);
  m_currentProofSequenceIndex = std::numeric_limits<ProofSequenceIdx>::max();
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

auto RUPChecker::isRUP(gsl::span<Lit const> clause, ProofSequenceIdx index) -> bool
{
  assert(index <= m_currentProofSequenceIndex &&
         "The proof sequence index must decrease monotonically");

  if (advanceProof(index) == AdvanceProofResult::UnaryConflict) {
    return true;
  }

  size_t const numAssignmentsAtStart = m_assignment.size();

  bool const hasRUP = std::any_of(clause.begin(), clause.end(), [this](Lit const& lit) {
    return assignAndPropagateToFixpoint(-lit, std::nullopt) == PropagateResult::Conflict;
  });

  for (Lit const& toClear : m_assignment.range(numAssignmentsAtStart)) {
    m_reasons[toClear] = std::nullopt;
  }
  m_assignment.clear(numAssignmentsAtStart);

  return hasRUP;
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
      // Since the proof sequence index is monotonically decreasing, this
      // clause won't be relevant until the next reset, so it can be safely
      // removed for now:
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
  m_currentProofSequenceIndex = index;

  auto staleDirectConflicts = std::remove_if(
      m_directUnaryConflicts.begin(), m_directUnaryConflicts.end(), [this, index](CRef const& ref) {
        return m_clauses.resolve(ref).getAddIdx() >= index;
      });
  m_directUnaryConflicts.erase(staleDirectConflicts, m_directUnaryConflicts.end());

  if (!m_directUnaryConflicts.empty()) {
    return AdvanceProofResult::UnaryConflict;
  }

  // Future optimization: keep unary assignments between checkRUP calls when possible
  m_assignment.clear(0);
  for (auto const& [unary, unaryCRef] : m_unaries) {
    if (unaryCRef.has_value()) {
      Clause const& unaryClause = m_clauses.resolve(*unaryCRef);
      if (unaryClause.getAddIdx() >= m_currentProofSequenceIndex) {
        continue;
      }
    }

    if (assignAndPropagateToFixpoint(unary, unaryCRef) == PropagateResult::Conflict) {
      return AdvanceProofResult::UnaryConflict;
    }
  }

  return AdvanceProofResult::NoConflict;
}
}
