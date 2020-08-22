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


#include <cryptominisat5/cryptominisat.h>
#include <ipasir.h>

#include <optional>
#include <random>
#include <vector>

#include <signal.h>


namespace {

CMSat::Lit cmsLit(int lit)
{
  return CMSat::Lit{static_cast<uint32_t>(std::abs(lit)), lit < 0};
}

class Solver {
public:
  void add(int lit) noexcept
  {
    if (m_invalid) {
      return;
    }

    try {
      m_rng = m_rng.value_or(std::mt19937{static_cast<uint32_t>(lit)});

      ensureSolverHasEnoughVars(lit);
      if (lit != 0) {
        m_clauseBuf.push_back(cmsLit(lit));
      }
      else {

        constexpr double p = 0.0005;
        if (m_faultDistr(*m_rng) < p) {
#if defined(INJECT_CORRECTNESS_FAULT)
          m_clauseBuf.clear();
#elif defined(INJECT_SEG_FAULT)
          raise(SIGSEGV);
#endif
        }

        m_solver.add_clause(m_clauseBuf);
        m_clauseBuf.clear();
      }
    }
    catch (...) {
      m_invalid = true;
    }
  }

  void assume(int lit) noexcept
  {
    if (m_invalid) {
      return;
    }

    try {
      ensureSolverHasEnoughVars(lit);
      m_assumptionBuf.push_back(cmsLit(lit));
    }
    catch (...) {
      m_invalid = true;
    }
  }

  int solve() noexcept
  {
    if (m_invalid) {
      return 0;
    }

    try {
      CMSat::lbool result = m_solver.solve(&m_assumptionBuf);
      if (result == CMSat::l_False) {
        return 20;
      }
      if (result == CMSat::l_True) {
        return 10;
      }
      return 0;
    }
    catch (...) {
      m_invalid = true;
      return 0;
    }
  }

  int val(int lit) noexcept
  {
    // not supported yet
    return 0;
  }

  int failed(int lit) noexcept
  {
    // not supported yet
    return 0;
  }

private:
  void ensureSolverHasEnoughVars(int toAdd)
  {
    uint32_t var = std::abs(toAdd);
    if (var + 1 > m_numVars) {
      m_solver.new_vars(var + 1 - m_numVars);
    }
    m_numVars += (var + 1 - m_numVars);
  }

  CMSat::SATSolver m_solver;
  bool m_invalid = false;
  std::vector<CMSat::Lit> m_assumptionBuf;
  std::vector<CMSat::Lit> m_clauseBuf;
  int m_numVars = 0;

  std::optional<std::mt19937> m_rng;
  std::uniform_real_distribution<double> m_faultDistr{0.0, 1.0};
};

}

extern "C" {
IPASIR_API const char* ipasir_signature()
{
  return "IPASIR solver with injected faults";
}

IPASIR_API void* ipasir_init()
{
  try {
    return new Solver{};
  }
  catch (...) {
    return nullptr;
  }
}

IPASIR_API void ipasir_release(void* solver)
{
  try {
    delete reinterpret_cast<Solver*>(solver);
  }
  catch (...) {
  }
}

IPASIR_API void ipasir_add(void* solver, int lit_or_zero)
{
  reinterpret_cast<Solver*>(solver)->add(lit_or_zero);
}

IPASIR_API void ipasir_assume(void* solver, int lit)
{
  reinterpret_cast<Solver*>(solver)->assume(lit);
}

IPASIR_API int ipasir_solve(void* solver)
{
  return reinterpret_cast<Solver*>(solver)->solve();
}

IPASIR_API int ipasir_val(void* solver, int lit)
{
  return reinterpret_cast<Solver*>(solver)->val(lit);
}

IPASIR_API int ipasir_failed(void* solver, int lit)
{
  return reinterpret_cast<Solver*>(solver)->failed(lit);
}

IPASIR_API void ipasir_set_terminate(void*, void*, int (*)(void*))
{
  // ignored
}

IPASIR_API void
ipasir_set_learn(void* solver, void* data, int max_length, void (*learn)(void* data, int* clause))
{
  // ignored
}
}