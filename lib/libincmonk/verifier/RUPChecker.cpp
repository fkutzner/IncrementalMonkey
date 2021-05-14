#include <libincmonk/verifier/RUPChecker.h>

namespace incmonk::verifier {
RUPChecker::RUPChecker(ClauseCollection& clauses)
  : m_clauses{clauses}
  , m_clausesUnderConsideration{clauses.begin(), clauses.end()}
  , m_assignment{clauses.getMaxVar()}
  , m_watchers{maxLit(clauses.getMaxVar())}
  , m_reasons{maxLit(clauses.getMaxVar())}
{
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
  ProofIndexUpdateResult advanceResult = advanceProof(index);
  if (advanceResult == ProofIndexUpdateResult::Conflict) {
    return RUPCheckResult::HasRUP;
  }

  size_t const startOfClause = m_assignment.size();
  OnExitScope const restoreAssignment{[this, startOfClause]() {
    // TODO: also remove reasons
    m_assignment.clear(startOfClause);
  }};

  for (Lit const& lit : clause) {
    if (m_assignment.get(lit) == t_indet) {
      // TODO: also add reasons
      m_assignment.add(-lit);
    }
    else {
      return (m_assignment.get(-lit) == t_true) ? RUPCheckResult::HasRUP
                                                : RUPCheckResult::ViolatesRUP;
    }
  }

  return propagateToFixpoint(m_assignment.range(startOfClause));
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

auto RUPChecker::propagateToFixpoint(Assignment::Range rng) -> RUPCheckResult
{
  return RUPCheckResult::ViolatesRUP;
}


auto RUPChecker::advanceProof(ProofSequenceIdx index) -> ProofIndexUpdateResult
{
  bool reconstructionRequired = false;

  if (index == std::numeric_limits<ProofSequenceIdx>::max()) {
    initializeProof();
    reconstructionRequired = true;
  }

  auto staleDirectConflicts = std::remove_if(
      m_directUnaryConflicts.begin(), m_directUnaryConflicts.end(), [this, index](CRef const& ref) {
        return m_clauses.resolve(ref).getAddIdx() >= index;
      });
  m_directUnaryConflicts.erase(staleDirectConflicts, m_directUnaryConflicts.end());

  if (!m_directUnaryConflicts.empty()) {
    return ProofIndexUpdateResult::Conflict;
  }

  std::vector<CRef> unaries;
  for (Lit const& lit : m_assignment.range(0)) {
    CRef const reasonRef = *m_reasons[lit];
    Clause const& reason = m_clauses.resolve(reasonRef);
    if (reason.getAddIdx() >= index) {
      reconstructionRequired = true;
    }
    else if (reason.size() == 1) {
      unaries.push_back(reasonRef);
    }
  }

  if (reconstructionRequired) {
    m_assignment.clear(0);

    for (CRef const& unaryRef : unaries) {
      Clause const& reason = m_clauses.resolve(unaryRef);
      Lit const unaryLit = reason[0];
      m_reasons[unaryLit] = unaryRef;
      m_assignment.add(unaryLit);
    }

    return propagateToFixpoint(m_assignment.range(0)) == RUPCheckResult::ViolatesRUP
               ? ProofIndexUpdateResult::NoConflict
               : ProofIndexUpdateResult::NoConflict;
  }

  return ProofIndexUpdateResult::NoConflict;
}
}
