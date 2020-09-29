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

#pragma once

#include <libincmonk/TBool.h>
#include <libincmonk/verifier/Assignment.h>
#include <libincmonk/verifier/BoundedMap.h>
#include <libincmonk/verifier/Clause.h>

#include <optional>
#include <vector>

namespace incmonk::verifier {
struct Watcher {
  ProofSequenceIdx pointOfAdd;
  Lit blocker;
  CRef clause;
};

using WatcherList = std::vector<Watcher>;

class Propagator {
public:
  using CRefVec = std::vector<CRef>;

  Propagator(ClauseCollection& clauses, Assignment& assignment, Var maxVar);
  auto propagateToFixpoint(Assignment::const_iterator start,
                           ProofSequenceIdx curProofSeqIdx,
                           CRefVec& newObligations) -> OptCRef;

  void activateUnary(CRef unary);
  void dismissUnary(CRef unary);

  Propagator(Propagator const&) = delete;
  auto operator=(Propagator const&) -> Propagator& = delete;

private:
  enum class WatcherPos { First, Second };

  void addClause(CRef cref, Clause const& clause);
  auto makeWatcher(CRef cref, Clause const& clause, WatcherPos watcherPos) const noexcept
      -> Watcher;
  auto getWatcherList(Lit watchedLit, bool isBinary, bool isFar) noexcept -> WatcherList&;

  void resurrectDeletedClauses();
  void analyzeCoreClausesInConflict(CRef conflict, CRefVec& newObligations);

  auto propagate(Lit newAssign) -> OptCRef;
  auto propagateBinaries(WatcherList& watchers) -> OptCRef;
  template <bool IsInCore>
  auto propInNonBins(Lit newAssign, WatcherList& watchers) -> OptCRef;


  struct WatcherLists {
    WatcherLists() = default;
    WatcherList coreBinaries;
    WatcherList core;
    WatcherList farBinaries;
    WatcherList far;

    std::optional<CRef> assignmentReason;
    bool isUnaryReason = false;
  };

  ClauseCollection& m_clauses;
  ClauseCollection::DeletedClausesRng m_deletedClauses;
  Assignment& m_assignment;
  BoundedMap<Lit, WatcherLists> m_watchers;
  ProofSequenceIdx m_proofSequenceIndex;
};
}