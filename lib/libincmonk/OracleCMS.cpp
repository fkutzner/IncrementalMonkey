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

#include <libincmonk/Oracle.h>

#include <cryptominisat5/cryptominisat.h>

namespace incmonk {
namespace {
class OracleCMS : public Oracle {
public:
  OracleCMS() {}

  void ensureSolverHasEnoughVars(CNFLit toAdd)
  {
    uint32_t var = std::abs(toAdd);
    if (var + 1 > m_numVars) {
      m_solver.new_vars(var + 1 - m_numVars);
    }
    m_numVars += (var + 1 - m_numVars);
  }

  void executeTraceCommand(AddClauseCmd const& cmd)
  {
    std::vector<CMSat::Lit> clause;

    for (CNFLit lit : cmd.clauseToAdd) {
      ensureSolverHasEnoughVars(lit);
      clause.push_back(CMSat::Lit{static_cast<uint32_t>(std::abs(lit)), lit < 0});
    }

    m_solver.add_clause(clause);
  }

  void executeTraceCommand(AssumeCmd const& cmd)
  {
    for (CNFLit lit : cmd.assumptions) {
      ensureSolverHasEnoughVars(lit);
      m_assumptions.push_back(CMSat::Lit{static_cast<uint32_t>(std::abs(lit)), lit < 0});
    }
  }

  void executeTraceCommand(SolveCmd& cmd)
  {
    if (!cmd.expectedResult.has_value()) {
      CMSat::lbool oracleResult = m_solver.solve(&m_assumptions);
      if (oracleResult != CMSat::l_Undef) {
        cmd.expectedResult = (oracleResult == CMSat::l_True);
      }
    }
    m_assumptions.clear();
  }

  void solve(FuzzTrace::iterator start, FuzzTrace::iterator stop) override
  {
    for (FuzzTrace::iterator cmd = start; cmd != stop; ++cmd) {
      std::visit([this](auto&& x) { executeTraceCommand(x); }, *cmd);
    }
  }

  ~OracleCMS() = default;

private:
  CMSat::SATSolver m_solver;
  size_t m_numVars = 0;
  std::vector<CMSat::Lit> m_assumptions;
};
}

auto createOracle() -> std::unique_ptr<Oracle>
{
  return std::make_unique<OracleCMS>();
}
}
