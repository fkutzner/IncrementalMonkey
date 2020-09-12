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

#include <libincmonk/FuzzTracePrinters.h>

#include <algorithm>
#include <optional>
#include <variant>

namespace incmonk {
namespace {
auto toCommaSeparatedStr(std::vector<int> vec)
{
  std::string result;
  bool first = true;
  for (CNFLit lit : vec) {
    if (!first) {
      result += ",";
    }
    result += std::to_string(lit);
    first = false;
  }
  return result;
}

auto toString(std::string const& solverVarName, AddClauseCmd const& cmd) -> std::string
{
  if (cmd.clauseToAdd.empty()) {
    return "ipasir_add(" + solverVarName + ", 0);\n";
  }

  std::string result = "for (int lit : {";
  result += toCommaSeparatedStr(cmd.clauseToAdd) + ",0}) {\n";
  result += "  ipasir_add(" + solverVarName + ", lit);\n}";
  return result;
}

auto toString(std::string const& solverVarName, AssumeCmd const& cmd) -> std::string
{
  if (cmd.assumptions.empty()) {
    return "";
  }

  std::string result = "for (int assump : {";
  result += toCommaSeparatedStr(cmd.assumptions);
  result += "}) {\n  ipasir_assume(" + solverVarName + ", assump);\n}";
  return result;
}

auto toString(std::string const& solverVarName, SolveCmd const& cmd) -> std::string
{
  if (cmd.expectedResult.has_value()) {
    std::string result;
    result += "{int result = ipasir_solve(" + solverVarName + "); assert(result == ";
    result += (*cmd.expectedResult ? "10" : "20");
    result += ");}\n";
    return result;
  }
  else {
    return "ipasir_solve(" + solverVarName + ");\n";
  }
}

auto toString(std::string const& solverVarName, HavocCmd const& cmd) -> std::string
{
  if (cmd.beforeInit) {
    return "// pre-init havoc with seed " + std::to_string(cmd.seed);
  }
  else {
    return "// havoc " + solverVarName + " with seed " + std::to_string(cmd.seed);
  }
}
}

auto toCxxFunctionBody(FuzzTrace::const_iterator first,
                       FuzzTrace::const_iterator last,
                       std::string const& argName) -> std::string
{
  std::string result;
  for (FuzzTrace::const_iterator cmd = first; cmd != last; ++cmd) {
    std::visit([&result, &argName](auto&& x) { result += toString(argName, x) + "\n"; }, *cmd);
  }
  return result;
}


namespace {
auto getMaxVar(CNFClause const& clause) -> std::optional<CNFLit>
{
  if (clause.empty()) {
    return std::nullopt;
  }

  CNFClause tmp = clause;
  std::transform(tmp.begin(), tmp.end(), tmp.begin(), [](CNFLit lit) { return std::abs(lit); });
  return *(std::max_element(tmp.begin(), tmp.end()));
}

auto getMaxVar(FuzzTrace::const_iterator first, FuzzTrace::const_iterator last) -> CNFLit
{
  CNFLit maxVar = 0;
  for (auto it = first; it != last; ++it) {
    if (AddClauseCmd const* addClause = std::get_if<AddClauseCmd>(&*it); addClause != nullptr) {
      maxVar = std::max(maxVar, getMaxVar(addClause->clauseToAdd).value_or(0));
    }
    else if (AssumeCmd const* assumeCmd = std::get_if<AssumeCmd>(&*it); assumeCmd != nullptr) {
      maxVar = std::max(maxVar, getMaxVar(assumeCmd->assumptions).value_or(0));
    }
  }
  return maxVar;
}

auto getNumClauses(FuzzTrace::const_iterator first, FuzzTrace::const_iterator last)
    -> FuzzTrace::size_type
{
  FuzzTrace::size_type result = 0;
  for (auto it = first; it != last; ++it) {
    if (AddClauseCmd const* addClause = std::get_if<AddClauseCmd>(&*it); addClause != nullptr) {
      ++result;
    }
  }
  return result;
}

void printICNFClause(std::vector<CNFLit> const& clause, std::ostream& target)
{
  for (CNFLit lit : clause) {
    target << lit << " ";
  }
  target << "0\n";
}


void printICNFFuzzCmd(SolveCmd const&,
                      std::vector<CNFLit>& currentAssumptions,
                      std::ostream& target)
{
  target << "a ";
  printICNFClause(currentAssumptions, target);
  currentAssumptions.clear();
}

void printICNFFuzzCmd(AssumeCmd const& cmd, std::vector<CNFLit>& currentAssumptions, std::ostream&)
{
  currentAssumptions.insert(
      currentAssumptions.end(), cmd.assumptions.begin(), cmd.assumptions.end());
}

void printICNFFuzzCmd(AddClauseCmd const& cmd, std::vector<CNFLit>&, std::ostream& target)
{
  printICNFClause(cmd.clauseToAdd, target);
}

void printICNFFuzzCmd(HavocCmd const& cmd, std::vector<CNFLit>&, std::ostream& target)
{
  if (cmd.beforeInit) {
    target << "c incmonk_havoc_init " << cmd.seed << "\n";
  }
  else {
    target << "c incmonk_havoc " << cmd.seed << "\n";
  }
}

void printICNFFuzzCmd(FuzzCmd const& cmd,
                      std::vector<CNFLit>& currentAssumptions,
                      std::ostream& target)
{
  std::visit([&](auto&& concreteCmd) { printICNFFuzzCmd(concreteCmd, currentAssumptions, target); },
             cmd);
}
}

void toICNF(FuzzTrace::const_iterator first,
            FuzzTrace::const_iterator last,
            std::ostream& targetStream)
{
  targetStream << "p inccnf " << getMaxVar(first, last) << " " << getNumClauses(first, last)
               << "\n";
  std::vector<CNFLit> currentAssumptions;
  for (auto it = first; it != last; ++it) {
    printICNFFuzzCmd(*it, currentAssumptions, targetStream);
  }
}
}