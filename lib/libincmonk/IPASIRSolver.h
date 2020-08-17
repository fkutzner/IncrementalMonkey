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

#include <exception>
#include <filesystem>
#include <memory>
#include <type_traits>
#include <vector>

#include "CNF.h"

namespace incmonk {

class DSOLoadError : public std::runtime_error {
public:
  DSOLoadError(std::string const& what) : std::runtime_error(what) {}
  virtual ~DSOLoadError() = default;
};

using IPASIRInitFn = std::add_pointer_t<void*()>;
using IPASIRReleaseFn = std::add_pointer_t<void(void*)>;
using IPASIRAddFn = std::add_pointer_t<void(void*, int)>;
using IPASIRAssumeFn = std::add_pointer_t<void(void*, int)>;
using IPASIRSolveFn = std::add_pointer_t<int(void*)>;
using IPASIRValFn = std::add_pointer_t<int(void*, int)>;


class IPASIRSolver {
public:
  IPASIRSolver() = default;
  virtual ~IPASIRSolver() = default;

  virtual void addClause(CNFClause const& clause) = 0;
  virtual void assume(std::vector<CNFLit> const& assumptions) = 0;

  enum class Result { SAT, UNSAT, UNKNOWN, ILLEGAL_RESULT };

  virtual auto solve() -> Result = 0;

  virtual void configure(uint64_t value) = 0;
};

auto createIPASIRSolver(std::filesystem::path const& pathToDSO) -> std::unique_ptr<IPASIRSolver>;

}
