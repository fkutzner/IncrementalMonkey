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

#include <dlfcn.h>
#include <filesystem>


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

void* checkedOpenDSO(std::filesystem::path const& path)
{
  void* result = dlopen(path.string().c_str(), RTLD_LOCAL);
  if (result == nullptr) {
    throw DSOLoadError{"Could not open " + path.string()};
  }
  return result;
}

class IPASIRSolverImpl : public IPASIRSolver {

public:
  IPASIRSolverImpl(IPASIRSolverDSO const& dso) : m_dso{dso}
  {
    m_ipasirContext = m_dso.initFn();
    // TODO: error handling when m_ipasircontext == nullptr
  }

  virtual ~IPASIRSolverImpl()
  {
    if (m_ipasirContext != nullptr) {
      m_dso.releaseFn(m_ipasirContext);
      m_ipasirContext = nullptr;
    }
  }

  void addClause(CNFClause const& clause) override
  {
    for (CNFLit lit : clause) {
      m_dso.addFn(m_ipasirContext, lit);
    }
    m_dso.addFn(m_ipasirContext, 0);
  }

  void assume(std::vector<CNFLit> const& assumptions) override
  {
    for (CNFLit a : assumptions) {
      m_dso.assumeFn(m_ipasirContext, a);
    }
  }

  auto solve() -> Result override
  {
    int result = m_dso.solveFn(m_ipasirContext);
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

  auto getValue(CNFLit lit) const noexcept -> TBool override
  {
    int value = m_dso.valFn(m_ipasirContext, lit);
    switch (value) {
    case 0:
      return t_false;
    case 10:
      return t_true;
    case 20:
      return t_indet;
    default:
      abort();
    }
  }

  auto isFailed(CNFLit lit) const noexcept -> bool override
  {
    int result = m_dso.failedFn(m_ipasirContext, lit);
    switch (result) {
    case 0:
      return false;
    case 1:
      return true;
    default:
      abort();
    }
  }

  void configure(uint64_t) override
  {
    // Not implemented yet
  }


private:
  IPASIRSolverDSO m_dso;
  void* m_ipasirContext = nullptr;
  Result m_lastResult = Result::UNKNOWN;
};
}

IPASIRSolverDSO::IPASIRSolverDSO(std::filesystem::path const& path)
  : m_dsoContext{checkedOpenDSO(path), [](void* dso) { dlclose(dso); }}
  , initFn{checkedGetFn<IPASIRInitFn>(m_dsoContext.get(), "ipasir_init")}
  , releaseFn{checkedGetFn<IPASIRReleaseFn>(m_dsoContext.get(), "ipasir_release")}
  , addFn{checkedGetFn<IPASIRAddFn>(m_dsoContext.get(), "ipasir_add")}
  , assumeFn{checkedGetFn<IPASIRAssumeFn>(m_dsoContext.get(), "ipasir_assume")}
  , solveFn{checkedGetFn<IPASIRSolveFn>(m_dsoContext.get(), "ipasir_solve")}
  , valFn{checkedGetFn<IPASIRValFn>(m_dsoContext.get(), "ipasir_val")}
  , failedFn{checkedGetFn<IPASIRFailedFn>(m_dsoContext.get(), "ipasir_failed")}
{
}

auto createIPASIRSolver(IPASIRSolverDSO const& dso) -> std::unique_ptr<IPASIRSolver>
{
  return std::make_unique<IPASIRSolverImpl>(dso);
}
}
