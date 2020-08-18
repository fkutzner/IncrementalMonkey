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

#include "IPASIRSolver.h"

#include "CNF.h"

#include <filesystem>

#include <dlfcn.h>

namespace incmonk {
namespace {

template <typename FnTy>
auto checkedGetFn(void* dso, std::string name) -> FnTy
{
  void* sym = dlsym(dso, name.c_str());
  if (sym == nullptr) {
    throw DSOLoadError{"Could not find symbol " + name};
  }
  return reinterpret_cast<FnTy>(sym);
}

class IPASIRSolverImpl : public IPASIRSolver {

public:
  IPASIRSolverImpl(std::filesystem::path const& path)
  {
    m_dsoContext = dlopen(path.string().c_str(), RTLD_LOCAL);
    if (m_dsoContext == nullptr) {
      throw DSOLoadError{"Could not open " + path.string()};
    }

    m_initFn = checkedGetFn<IPASIRInitFn>(m_dsoContext, "ipasir_init");
    m_releaseFn = checkedGetFn<IPASIRReleaseFn>(m_dsoContext, "ipasir_release");
    m_addFn = checkedGetFn<IPASIRAddFn>(m_dsoContext, "ipasir_add");
    m_assumeFn = checkedGetFn<IPASIRAssumeFn>(m_dsoContext, "ipasir_assume");
    m_solveFn = checkedGetFn<IPASIRSolveFn>(m_dsoContext, "ipasir_solve");
    m_valFn = checkedGetFn<IPASIRValFn>(m_dsoContext, "ipasir_val");

    m_ipasirContext = m_initFn();
    if (m_ipasirContext == nullptr) {
      throw DSOLoadError{"Could not initialize IPASIR"};
    }
  }

  virtual ~IPASIRSolverImpl()
  {
    if (m_ipasirContext != nullptr) {
      m_releaseFn(m_ipasirContext);
      m_ipasirContext = nullptr;
    }

    if (m_dsoContext != nullptr) {
      dlclose(m_dsoContext);
      m_dsoContext = nullptr;
    }
  }

  void addClause(CNFClause const& clause) override
  {
    for (CNFLit lit : clause) {
      m_addFn(m_ipasirContext, lit);
    }
    m_addFn(m_ipasirContext, 0);
  }

  void assume(std::vector<CNFLit> const& assumptions) override
  {
    for (CNFLit a : assumptions) {
      m_assumeFn(m_ipasirContext, a);
    }
  }

  auto solve() -> Result override
  {
    int result = m_solveFn(m_ipasirContext);
    if (result == 0) {
      m_lastResult = Result::UNKNOWN;
    }
    else if (result == 10) {
      m_lastResult = Result::SAT;
    }
    else if (result == 20) {
      m_lastResult = Result::UNSAT;
    }
    else {
      m_lastResult = Result::ILLEGAL_RESULT;
    }
    return m_lastResult;
  }

  auto getLastSolveResult() const noexcept -> Result override { return m_lastResult; }

  void configure(uint64_t) override
  {
    // Not implemented yet
  }


private:
  void* m_dsoContext = nullptr;
  void* m_ipasirContext = nullptr;

  IPASIRInitFn m_initFn = nullptr;
  IPASIRReleaseFn m_releaseFn = nullptr;
  IPASIRAddFn m_addFn = nullptr;
  IPASIRAssumeFn m_assumeFn = nullptr;
  IPASIRSolveFn m_solveFn = nullptr;
  IPASIRValFn m_valFn = nullptr;

  Result m_lastResult = Result::UNKNOWN;
};
}

auto createIPASIRSolver(std::filesystem::path const& pathToDSO) -> std::unique_ptr<IPASIRSolver>
{
  return std::make_unique<IPASIRSolverImpl>(pathToDSO);
}
}
