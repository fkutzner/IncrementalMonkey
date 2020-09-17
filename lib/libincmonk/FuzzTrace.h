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
 * \brief FuzzTrace data structure (& core operations) for representing sequences
 *   of IPASIR API calls.
 */

#pragma once

#include <cstdint>
#include <exception>
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
 * \brief A FuzzTrace havoc command
 */
struct HavocCmd {
  uint64_t seed = 0;
  bool beforeInit = false;
};

auto operator==(HavocCmd const& lhs, HavocCmd const& rhs) noexcept -> bool;
auto operator!=(HavocCmd const& lhs, HavocCmd const& rhs) noexcept -> bool;
auto operator<<(std::ostream& stream, HavocCmd const& cmd) -> std::ostream&;

/**
 * \brief A FuzzTrace element, containing any FuzzTrace command.
 */
using FuzzCmd = std::variant<AddClauseCmd, AssumeCmd, SolveCmd, HavocCmd>;

auto operator<<(std::ostream& stream, FuzzCmd const& cmd) -> std::ostream&;

/**
 * \brief A trace of IPASIR commands.
 */
using FuzzTrace = std::vector<FuzzCmd>;


/**
 * \brief Applies the given trace to an IPASIR SAT solver, checking
 *   any expected SolveCmd results if specified.
 * 
 * \returns Iterator to the first SolveCmd within [first, last) for which
 *   the SAT solver returned an unexpected result or the solve command has
 *   no satisfibility info. Otherwise, `last` is returned.
 */
auto applyTrace(FuzzTrace::const_iterator first,
                FuzzTrace::const_iterator last,
                IPASIRSolver& target) -> FuzzTrace::const_iterator;


class IOException : public std::runtime_error {
public:
  IOException(std::string const& what);
  virtual ~IOException() = default;
};

/**
 * \brief Stores the given trace in a file
 * 
 * TODO: describe format
 * 
 * \throw IOException   on file I/O failures
 */
void storeTrace(FuzzTrace::const_iterator first,
                FuzzTrace::const_iterator last,
                std::filesystem::path const& filename);


enum class LoaderStrictness { STRICT, PERMISSIVE };

/**
 * \brief Loads the trace from the given file
 * 
 * \param filename    The file to load
 * \param strictness  If STRICT, only valid traces are parsed. If PERMISSIVE, any byte sequence
 *                    is interpreted as a trace, ignoring the versioning header and wrapping
 *                    command numbers.
 * 
 * \throw IOException   on file I/O failures and file format errors
 */
auto loadTrace(std::filesystem::path const& filename,
               LoaderStrictness strictness = LoaderStrictness::STRICT) -> FuzzTrace;

/**
 * \brief Loads the trace from the given C file stream
 * 
 * \param stream      The stream from which to load the trace
 * \param strictness  If STRICT, only valid traces are parsed. If PERMISSIVE, any byte sequence
 *                    is interpreted as a trace, ignoring the versioning header and wrapping
 *                    command numbers.
 * 
 * \throw IOException   on file I/O failures and file format errors
 */
auto loadTrace(FILE& stream, LoaderStrictness strictness = LoaderStrictness::STRICT) -> FuzzTrace;
}
