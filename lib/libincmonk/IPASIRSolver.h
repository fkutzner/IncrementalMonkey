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
#include <libincmonk/TBool.h>

#include <exception>
#include <filesystem>
#include <functional>
#include <memory>
#include <type_traits>
#include <vector>


namespace incmonk {

using IPASIRInitFn = std::add_pointer_t<void*()>;
using IPASIRReleaseFn = std::add_pointer_t<void(void*)>;
using IPASIRAddFn = std::add_pointer_t<void(void*, int)>;
using IPASIRAssumeFn = std::add_pointer_t<void(void*, int)>;
using IPASIRSolveFn = std::add_pointer_t<int(void*)>;
using IPASIRValFn = std::add_pointer_t<int(void*, int)>;

class DSOLoadError : public std::runtime_error {
public:
  DSOLoadError(std::string const& what) : std::runtime_error(what) {}
  virtual ~DSOLoadError() = default;
};


/**
 * \brief Copyable and movable IPASIR solver DSO handle.
 * 
 * This functionality is kept separate from the IPASIR wrapper
 * implementation itself to avoid expensive `dlopen()` calls when
 * obtaining new solver instances.
 */
class IPASIRSolverDSO final {
public:
  /**
   * \brief Constructs the IPASIR DSO handle.
   * 
   * \path Path to the IPASIR DSO.
   * \throws DSOLoadError when loading the DSO failed.
   */
  explicit IPASIRSolverDSO(std::filesystem::path const& path);

private:
  std::shared_ptr<void> m_dsoContext;

public:
  IPASIRInitFn const initFn = nullptr;
  IPASIRReleaseFn const releaseFn = nullptr;
  IPASIRAddFn const addFn = nullptr;
  IPASIRAssumeFn const assumeFn = nullptr;
  IPASIRSolveFn const solveFn = nullptr;
  IPASIRValFn const valFn = nullptr;
};


/**
 * \brief Wrapper interface for IPASIR SAT solvers
 */
class IPASIRSolver {
public:
  IPASIRSolver() = default;
  virtual ~IPASIRSolver() = default;

  virtual void addClause(CNFClause const& clause) = 0;
  virtual void assume(std::vector<CNFLit> const& assumptions) = 0;

  enum class Result { SAT, UNSAT, UNKNOWN, ILLEGAL_RESULT };

  virtual auto solve() -> Result = 0;
  virtual auto getLastSolveResult() const noexcept -> Result = 0;
  virtual auto getValue(CNFLit lit) const noexcept -> TBool = 0;

  virtual void configure(uint64_t value) = 0;

private:
};

auto createIPASIRSolver(IPASIRSolverDSO const& dso) -> std::unique_ptr<IPASIRSolver>;

}
