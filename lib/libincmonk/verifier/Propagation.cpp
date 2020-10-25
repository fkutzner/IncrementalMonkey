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

#include <cassert>
#include <limits>

namespace incmonk::verifier {

namespace {
auto isFromFuture(Watcher const& watcher, ProofSequenceIdx currentIndex) -> bool
{
  return currentIndex <= watcher.pointOfAdd;
}
}

Propagator::Propagator(ClauseCollection& clauses, Assignment& assignment, Var maxVar)
  : m_clauses{clauses}
  , m_deletedClauses{clauses.getDeletedClausesOrdered()}
  , m_assignment{assignment}
  , m_watchers{maxLit(maxVar)}
  , m_proofSequenceIndex{std::numeric_limits<ProofSequenceIdx>::max()}
{
  for (CRef cref : clauses) {
    Clause const& clause = m_clauses.resolve(cref);
    if (clause.getDelIdx() == std::numeric_limits<ProofSequenceIdx>::max()) {
      addClause(cref, clause);
    }
    // Clauses with a deletion index will be added on-demand
  }
}

void Propagator::addClause(CRef cref, Clause const& clause)
{
  bool const isUnary = (clause.size() == 1);

  if (!isUnary) {
    bool const isBinary = (clause.size() == 2);
    bool const isFar = (clause.getState() == ClauseVerificationState::Passive);
    getWatcherList(clause[0], isBinary, isFar)
        .push_back(makeWatcher(cref, clause, WatcherPos::First));
    getWatcherList(clause[1], isBinary, isFar)
        .push_back(makeWatcher(cref, clause, WatcherPos::Second));
  }
}

auto Propagator::getWatcherList(Lit watchedLit, bool isBinary, bool isFar) noexcept -> WatcherList&
{
  WatcherLists& watchers = m_watchers[-watchedLit];
  if (!isFar && isBinary) {
    return watchers.coreBinaries;
  }
  else if (!isFar && !isBinary) {
    return watchers.core;
  }
  else if (isFar && isBinary) {
    return watchers.farBinaries;
  }
  else {
    assert(isFar && !isBinary);
    return watchers.far;
  }
}

auto Propagator::makeWatcher(CRef cref, Clause const& clause, WatcherPos watcherPos) const noexcept
    -> Watcher
{
  Lit const blocker = (watcherPos == WatcherPos::First ? clause[1] : clause[0]);
  return Watcher{clause.getAddIdx(), blocker, cref};
}

auto Propagator::propagateToFixpoint(Assignment::const_iterator start) -> OptCRef
{
  for (Lit l : m_assignment.range()) {
    if (!m_watchers[l].isUnaryReason) {
      m_watchers[l].assignmentReason = std::nullopt;
    }
  }

  Assignment::const_iterator cursor = start;
  while (cursor != m_assignment.range().end()) {
    if (OptCRef conflict = propagate(*cursor); conflict.has_value()) {
      analyzeCoreClausesInConflict(*conflict);
      return conflict;
    }
    ++cursor;
  }

  return std::nullopt;
}

void Propagator::activateUnary(CRef unary)
{
  Lit unaryLit = m_clauses.resolve(unary)[0];
  m_watchers[unaryLit].assignmentReason = unary;
  m_watchers[unaryLit].isUnaryReason = true;
}

void Propagator::dismissUnary(CRef unary)
{
  Lit unaryLit = m_clauses.resolve(unary)[0];
  m_watchers[unaryLit].assignmentReason = std::nullopt;
  m_watchers[unaryLit].isUnaryReason = false;
}

auto Propagator::propagate(Lit newAssign) -> OptCRef
{
  WatcherLists& watchers = m_watchers[newAssign];

  if (OptCRef conflict = propagateBinaries(watchers.coreBinaries); conflict.has_value()) {
    return conflict;
  }
  if (OptCRef conflict = propInNonBins<true>(newAssign, watchers.core); conflict.has_value()) {
    return conflict;
  }
  if (OptCRef conflict = propagateBinaries(watchers.farBinaries); conflict.has_value()) {
    return conflict;
  }
  if (OptCRef conflict = propInNonBins<false>(newAssign, watchers.far); conflict.has_value()) {
    return conflict;
  }

  return std::nullopt;
}

namespace {
void eraseFromWatcherList(Watcher& toErase, WatcherList& toEraseFrom)
{
  assert(!toEraseFrom.empty());
  using std::swap;
  swap(toErase, toEraseFrom.back());
  toEraseFrom.pop_back();
}
}

auto Propagator::propagateBinaries(WatcherList& watchers) -> OptCRef
{
  for (WatcherList::size_type curIdx = 0; curIdx < watchers.size();) {
    Watcher& watcher = watchers[curIdx];
    if (isFromFuture(watcher, m_proofSequenceIndex)) {
      eraseFromWatcherList(watcher, watchers);
      continue;
    }

    TBool const assignment = m_assignment.get(watcher.blocker);
    if (assignment == t_indet) {
      m_assignment.add(watcher.blocker);
      m_watchers[watcher.blocker].assignmentReason = watcher.clause;
    }
    else if (assignment == t_false) {
      return watcher.clause;
    }

    ++curIdx;
  }

  return std::nullopt;
}

template <bool IsInCore>
auto Propagator::propInNonBins(Lit newAssign, WatcherList& watchers) -> OptCRef
{
  WatcherList::size_type curIdx = 0;
  while (curIdx < watchers.size()) {
    Watcher& watcher = watchers[curIdx];
    if (isFromFuture(watcher, m_proofSequenceIndex)) {
      eraseFromWatcherList(watcher, watchers);
      continue;
    }

    if (m_assignment.get(watcher.blocker) == t_true) {
      ++curIdx;
      continue;
    }

    Clause& clause = m_clauses.resolve(watcher.clause);

    if constexpr (!IsInCore) {
      if (clause.getState() != ClauseVerificationState::Passive) {
        eraseFromWatcherList(watcher, watchers);
        continue;
      }
    }

    Clause::size_type const watchedIndex = (-newAssign == clause[0] ? 0 : 1);
    watcher.blocker = clause[1 - watchedIndex];
    if (m_assignment.get(watcher.blocker) == t_true) {
      ++curIdx;
      continue;
    }

    TBool const otherWatchedLitAssignment = m_assignment.get(watcher.blocker);
    if (otherWatchedLitAssignment == t_true) {
      ++curIdx;
      continue;
    }

    for (Clause::size_type idx = 2; idx < clause.size(); ++idx) {
      Lit const currentLit = clause[idx];
      if (m_assignment.get(currentLit) != t_false) {
        std::swap(clause[watchedIndex], clause[idx]);
        WatcherList* target = nullptr;
        if constexpr (IsInCore) {
          target = &(m_watchers[-currentLit].core);
        }
        else {
          target = &(m_watchers[-currentLit].far);
        }
        target->push_back(watcher);
        eraseFromWatcherList(watcher, watchers);

        goto next;
      }
    }

    if (otherWatchedLitAssignment == t_indet) {
      m_assignment.add(watcher.blocker);
      m_watchers[watcher.blocker].assignmentReason = watcher.clause;
    }
    else {
      return watcher.clause;
    }
    ++curIdx;

  next:
    (void)0;
  }

  return std::nullopt;
}

namespace advanceProof_detail {
namespace {
void tryMoveNonFalseLitsToFront(Clause& clause, Assignment const& assignment)
{
  for (Clause::size_type watchedIdx : {0, 1}) {
    if (assignment.get(clause[watchedIdx]) != t_false) {
      continue;
    }

    for (Clause::size_type curIdx = 2, end = clause.size(); curIdx < end; ++curIdx) {
      if (assignment.get(clause[watchedIdx]) != t_false) {
        std::swap(clause[watchedIdx], clause[curIdx]);
        break;
      }
    }
  }

  if (assignment.get(clause[1]) != t_false) {
    std::swap(clause[0], clause[1]);
  }
}

auto isFalsified(Clause const& clause, Assignment const& assignment) -> bool
{
  return assignment.get(clause[0]) == t_false && assignment.get(clause[1]) == t_false;
}

auto isForcingFirstLit(Clause const& clause, Assignment const& assignment) -> bool
{
  return assignment.get(clause[0]) == t_indet && assignment.get(clause[1]) == t_false;
}
}
}

auto Propagator::advanceProof(ProofSequenceIdx curProofSeqIdx) -> OptCRef
{
  using namespace advanceProof_detail;

  assert(curProofSeqIdx <= m_proofSequenceIndex);
  m_proofSequenceIndex = curProofSeqIdx;

  Assignment::size_type const originalNumAssigns = m_assignment.size();

  std::size_t resurrected = 0;
  std::optional<CRef> falsifiedClause;
  for (auto crIter = m_deletedClauses.rbegin(); crIter != m_deletedClauses.rend(); ++crIter) {
    CRef crefToAdd = *crIter;
    Clause& clauseToAdd = m_clauses.resolve(crefToAdd);
    if (clauseToAdd.getDelIdx() <= m_proofSequenceIndex) {
      break;
    }

    if (clauseToAdd.size() >= 2 && clauseToAdd.getAddIdx() < m_proofSequenceIndex) {
      // Trying to make the clause addition consistent with the current assignment.
      // However, if falsifiedClause already has a value, the current assignment
      // needs to be undone completely before propagating again, rendering further
      // consistency maintenance obsolete:
      bool const preserveAssignment = (!m_assignment.empty() && !falsifiedClause.has_value());
      if (preserveAssignment) {
        tryMoveNonFalseLitsToFront(clauseToAdd, m_assignment);
        if (isFalsified(clauseToAdd, m_assignment)) {
          falsifiedClause = crefToAdd;

          // The fact clauseToAdd this clause is falsified can be used to
          // justify other parts of the proof, so mark it (and its dependencies)
          // as verification work:
          analyzeCoreClausesInConflict(*falsifiedClause);
        }
        else if (isForcingFirstLit(clauseToAdd, m_assignment)) {
          m_assignment.add(clauseToAdd[0]);
          m_watchers[clauseToAdd[0]].assignmentReason = crefToAdd;
        }
      }

      addClause(crefToAdd, clauseToAdd);
    }

    ++resurrected;
  }

  std::size_t remaining = m_deletedClauses.size() - resurrected;
  m_deletedClauses = m_deletedClauses.subspan(0, remaining);

  if (falsifiedClause.has_value()) {
    return falsifiedClause;
  }
  else {
    auto const propStart = m_assignment.range().begin() + originalNumAssigns;
    return propagateToFixpoint(propStart);
  }
}

void Propagator::analyzeCoreClausesInConflict(CRef conflict)
{
  std::vector<CRef> analysisWork{conflict};

  while (!analysisWork.empty()) {
    CRef const currentRef = analysisWork.back();
    Clause& currentClause = m_clauses.resolve(currentRef);
    analysisWork.pop_back();

    if (currentClause.getState() == ClauseVerificationState::Passive) {
      currentClause.setState(ClauseVerificationState::VerificationPending);

      // Re-adding the clause as a core clause. It will be cleaned from the far
      // watchers on-the-fly later on.
      addClause(currentRef, currentClause);
    }

    for (Lit l : currentClause.getLiterals()) {
      if (OptCRef reason = m_watchers[-l].assignmentReason; reason.has_value()) {
        analysisWork.push_back(*reason);
        m_watchers[-l].assignmentReason = std::nullopt;
      }
    }
  }
}

}