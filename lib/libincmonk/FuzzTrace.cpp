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

#include <libincmonk/FuzzTrace.h>
#include <libincmonk/IPASIRSolver.h>

#include <string>
#include <type_traits>
#include <variant>

namespace incmonk {

auto operator==(AddClauseCmd const& lhs, AddClauseCmd const& rhs) noexcept -> bool
{
  return &lhs == &rhs || lhs.clauseToAdd == rhs.clauseToAdd;
}

auto operator!=(AddClauseCmd const& lhs, AddClauseCmd const& rhs) noexcept -> bool
{
  return !(lhs == rhs);
}

auto operator<<(std::ostream& stream, AddClauseCmd const& cmd) -> std::ostream&
{
  stream << "(AddClauseCmd";
  for (CNFLit lit : cmd.clauseToAdd) {
    stream << " " << std::to_string(lit);
  }
  stream << ")";
  return stream;
}

auto operator==(AssumeCmd const& lhs, AssumeCmd const& rhs) noexcept -> bool
{
  return &lhs == &rhs || lhs.assumptions == rhs.assumptions;
}

auto operator!=(AssumeCmd const& lhs, AssumeCmd const& rhs) noexcept -> bool
{
  return !(lhs == rhs);
}

auto operator<<(std::ostream& stream, AssumeCmd const& cmd) -> std::ostream&
{
  stream << "(AssumeCmd";
  for (CNFLit lit : cmd.assumptions) {
    stream << " " << std::to_string(lit);
  }
  stream << ")";
  return stream;
}

auto operator==(SolveCmd const& lhs, SolveCmd const& rhs) noexcept -> bool
{
  return &lhs == &rhs || lhs.expectedResult == rhs.expectedResult;
}

auto operator!=(SolveCmd const& lhs, SolveCmd const& rhs) noexcept -> bool
{
  return !(lhs == rhs);
}

auto operator<<(std::ostream& stream, SolveCmd const& cmd) -> std::ostream&
{
  stream << "(SolveCmd ";
  if (cmd.expectedResult.has_value()) {
    stream << ((*cmd.expectedResult) ? "10" : "20");
  }
  else {
    stream << "<undetermined>";
  }
  stream << ")";
  return stream;
}

auto operator<<(std::ostream& stream, FuzzCmd const& cmd) -> std::ostream&
{
  std::visit([&stream](auto&& x) { stream << x; }, cmd);
  return stream;
}

namespace {
auto applyCmd(IPASIRSolver& solver, AddClauseCmd const& cmd) -> bool
{
  solver.addClause(cmd.clauseToAdd);
  return true;
}

auto applyCmd(IPASIRSolver& solver, AssumeCmd const& cmd) -> bool
{
  solver.assume(cmd.assumptions);
  return true;
}

auto applyCmd(IPASIRSolver& solver, SolveCmd const& cmd) -> bool
{
  int result = solver.solve();

  if (cmd.expectedResult.has_value()) {
    bool const isSat = (result == 10);
    return *(cmd.expectedResult) == isSat;
  }
  return true;
}
}

auto applyTrace(FuzzTrace const& trace, IPASIRSolver& target) -> std::optional<size_t>
{
  size_t index = 0;
  bool succeded = true;

  for (FuzzCmd const& cmd : trace) {
    std::visit([&target, &succeded](auto&& x) { succeded = applyCmd(target, x); }, cmd);
    if (!succeded) {
      break;
    }
    ++index;
  }

  return succeded ? std::nullopt : std::make_optional(index);
}

namespace {
auto toString(std::string const& solverVarName, AddClauseCmd const& cmd) -> std::string
{
  std::string result;
  for (CNFLit lit : cmd.clauseToAdd) {
    result += "ipasir_add(" + solverVarName + ", " + std::to_string(lit) + ");\n";
  }
  result += "ipasir_add(" + solverVarName + ", 0);\n";
  return result;
}

auto toString(std::string const& solverVarName, AssumeCmd const& cmd) -> std::string
{
  std::string result;
  for (CNFLit lit : cmd.assumptions) {
    result += "ipasir_assume(" + solverVarName + ", " + std::to_string(lit) + ");\n";
  }
  return result;
}

auto toString(std::string const& solverVarName, SolveCmd const& cmd) -> std::string
{
  if (cmd.expectedResult.has_value()) {
    std::string result;
    result += "{int result = ipasir_solve(" + solverVarName + "); assert(result == ";
    result += (*cmd.expectedResult ? "10" : "20");
    result += ");\n";
    return result;
  }
  else {
    return "ipasir_solve(" + solverVarName + ");\n";
  }
}
}

auto toCFunctionBody(FuzzTrace const& trace, std::string const& argName) -> std::string
{
  std::string result;
  for (FuzzCmd const& cmd : trace) {
    std::visit([&result, &argName](auto&& x) { result += toString(argName, x) + "\n"; }, cmd);
  }
  return result;
}
}