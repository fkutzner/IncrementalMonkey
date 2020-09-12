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
}