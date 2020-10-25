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

#include <libincmonk/CNF.h>
#include <libincmonk/FuzzTrace.h>
#include <libincmonk/IPASIRSolver.h>

#include <unordered_set>
#include <vector>

namespace incmonk {
struct FakeIPASIRResult {
  IPASIRSolver::Result result;

  // if result is SAT, modelOrFailed contains the model. If result is UNSAT,
  // modelOrFailed contains the failed assertions. Otherwise, it is ignored.
  std::vector<CNFLit> modelOrFailed;
};

class FakeIPASIRSolver : public IPASIRSolver {
public:
  FakeIPASIRSolver(std::vector<FakeIPASIRResult> const& fakedResults)
  {
    m_fakedResults.assign(fakedResults.rbegin(), fakedResults.rend());
  }


  auto solve() -> Result override
  {
    assert(!m_fakedResults.empty());
    FakeIPASIRResult const& resultInfo = m_fakedResults.back();
    m_lastResult = resultInfo.result;

    m_model.clear();
    m_failed.clear();
    if (m_lastResult == Result::SAT) {
      m_model.insert(resultInfo.modelOrFailed.begin(), resultInfo.modelOrFailed.end());
    }
    else if (m_lastResult == Result::UNSAT) {
      m_failed.insert(resultInfo.modelOrFailed.begin(), resultInfo.modelOrFailed.end());
    }

    m_fakedResults.pop_back();
    return m_lastResult;
  }

  auto getLastSolveResult() const noexcept -> Result override { return m_lastResult; }

  void addClause(CNFClause const&) override {}
  void assume(std::vector<CNFLit> const&) override {}
  void configure(uint64_t) override {}

  auto getValue(CNFLit lit) const noexcept -> TBool override
  {
    if (m_model.find(lit) != m_model.end()) {
      return t_true;
    }
    if (m_model.find(-lit) != m_model.end()) {
      return t_false;
    }
    return t_indet;
  }

  auto isFailed(CNFLit lit) const noexcept -> bool override
  {
    return m_failed.find(lit) != m_failed.end();
  }

  void reinitializeWithHavoc(uint64_t) noexcept override {}

  void havoc(uint64_t) noexcept override {}

  virtual ~FakeIPASIRSolver() = default;

private:
  std::vector<FakeIPASIRResult> m_fakedResults;

  std::unordered_set<CNFLit> m_model;
  std::unordered_set<CNFLit> m_failed;


  Result m_lastResult = Result::UNKNOWN;
};
}