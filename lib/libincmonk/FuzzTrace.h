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

/**
 * \file
 * 
 * \brief IPASIR solver trace data structures and related apply/print/load/store functions
 */

#include <filesystem>
#include <optional>
#include <ostream>
#include <variant>
#include <vector>

#include "CNF.h"

namespace incmonk {

class IPASIRSolver;

/**
 * \brief A FuzzTrace command representing the addition of a clause, i.e. a sequence
 *   of `ipasir_add()` calls terminating with the addition of the 0 literal.
 */
struct AddClauseCmd {
  CNFClause clauseToAdd;
};

auto operator==(AddClauseCmd const& lhs, AddClauseCmd const& rhs) noexcept -> bool;
auto operator!=(AddClauseCmd const& lhs, AddClauseCmd const& rhs) noexcept -> bool;
auto operator<<(std::ostream& stream, AddClauseCmd const& cmd) -> std::ostream&;

/**
 * \brief A FuzzTrace command representing the assumption of facts, i.e. a sequence
 *   of `ipasir_assume()` calls.
 */
struct AssumeCmd {
  std::vector<CNFLit> assumptions;
};

auto operator==(AssumeCmd const& lhs, AssumeCmd const& rhs) noexcept -> bool;
auto operator!=(AssumeCmd const& lhs, AssumeCmd const& rhs) noexcept -> bool;
auto operator<<(std::ostream& stream, AssumeCmd const& cmd) -> std::ostream&;

/**
 * \brief A FuzzTrace command representing an `ipasir_solve` invocation.
 */
struct SolveCmd {
  /// The expected result, `true` iff SAT is expected.
  std::optional<bool> expectedResult;
};

auto operator==(SolveCmd const& lhs, SolveCmd const& rhs) noexcept -> bool;
auto operator!=(SolveCmd const& lhs, SolveCmd const& rhs) noexcept -> bool;
auto operator<<(std::ostream& stream, SolveCmd const& cmd) -> std::ostream&;

/**
 * \brief A FuzzTrace element, containing any FuzzTrace command.
 */
using FuzzCmd = std::variant<AddClauseCmd, AssumeCmd, SolveCmd>;

auto operator<<(std::ostream& stream, FuzzCmd const& cmd) -> std::ostream&;

/**
 * \brief A trace of IPASIR commands.
 */
using FuzzTrace = std::vector<FuzzCmd>;


/**
 * \brief Applies the given trace to an IPASIR SAT solver, checking
 *   any expected SolveCmd results if specified.
 * 
 * \returns The index of the first SolveCmd within `trace` for which
 *   the SAT solver returned an unexpected result. Otherwise, nothing
 *   is returned.
 */
auto applyTrace(FuzzTrace const& trace, IPASIRSolver& target) -> std::optional<size_t>;


/**
 * \brief Translates the given trace to a sequence of corresponding C function calls
 *   
 * No `ipasir_init` or `ipasir_destroy` calls are generated.
 * 
 * Usage example: generate regression tests for error traces.
 * 
 * \param trace       The trace to be translated
 * \param solverName  The variable name of the IPASIR solver pointer (ie. the `void*` argument)
 * 
 * \returns           A newline-separated sequence of IPASIR function calls corresponding to `trace`
 */
auto toCFunctionBody(FuzzTrace const& trace, std::string const& solverName) -> std::string;


void storeTrace(FuzzTrace const& trace, std::filesystem::path const& filename);
auto loadTrace(std::filesystem::path const& filename) -> FuzzTrace;
}
