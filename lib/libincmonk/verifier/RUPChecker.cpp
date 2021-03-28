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

auto RUPChecker::checkRUP(gsl::span<Lit> clause, ProofSequenceIdx index) -> RUPCheckResult
{
  ProofIndexUpdateResult advanceResult = advanceProof(index);
  if (advanceResult == ProofIndexUpdateResult::Conflict) {
    return RUPCheckResult::HasRUP;
  }

  return RUPCheckResult::ViolatesRUP;
}

auto RUPChecker::initializeProof(ProofSequenceIdx index) -> ProofIndexUpdateResult
{
  for (CRef const cref : m_clauses) {
    Clause& clause = m_clauses.resolve(cref);

    if (clause.getAddIdx() >= index) {
      // Clauses are sorted by their addition index, so all further ones are
      // irrelevant.
      break;
    }

    if (clause.getDelIdx() < index) {
      // Will be added later, but is not in effect initially
      continue;
    }

    if (clause.size() == 1) {
      Lit const unaryLit = clause[0];

      if (m_assignment.get(unaryLit) == t_indet) {
        // TODO: also deal with multiple unary assignments
        m_assignment.add(unaryLit);
        m_reasons[unaryLit] = cref;
      }
      else if (m_assignment.get(unaryLit) == t_false) {
        m_directUnaryConflicts[unaryLit.getVar()].push_back(cref);
      }
    }
  }

  if (!m_directUnaryConflicts.empty()) {
    return ProofIndexUpdateResult::Conflict;
  }

  RUPCheckResult initialRUPCheck = propagateToFixpoint(m_assignment.range());
  if (initialRUPCheck == RUPCheckResult::HasRUP) {
    return ProofIndexUpdateResult::Conflict;
  }
  else {
    return ProofIndexUpdateResult::NoConflict;
  }
}

auto RUPChecker::advanceInitializedProof(ProofSequenceIdx index) -> ProofIndexUpdateResult
{
  return ProofIndexUpdateResult::NoConflict;
}

auto RUPChecker::propagateToFixpoint(Assignment::Range rng) -> RUPCheckResult {
  return RUPCheckResult::ViolatesRUP;
}


auto RUPChecker::advanceProof(ProofSequenceIdx index) -> ProofIndexUpdateResult
{
  if (m_currentProofSequenceIndex == std::numeric_limits<ProofSequenceIdx>::max()) {
    return initializeProof(index);
  }
  else {
    return advanceInitializedProof(index);
  }
}
}
