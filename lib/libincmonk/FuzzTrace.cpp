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
#include <libincmonk/IOUtils.h>
#include <libincmonk/IPASIRSolver.h>

#include <gsl/gsl_util>

#include <cstdio>
#include <fstream>
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
  IPASIRSolver::Result result = solver.solve();

  if (cmd.expectedResult.has_value()) {
    bool const isSat = (result == IPASIRSolver::Result::SAT);
    return *(cmd.expectedResult) == isSat;
  }
  return true;
}
}

auto applyTrace(FuzzTrace::const_iterator first,
                FuzzTrace::const_iterator last,
                IPASIRSolver& target) -> FuzzTrace::const_iterator
{
  FuzzTrace::const_iterator cmd = first;

  for (; cmd != last; ++cmd) {
    bool succeded = true;
    std::visit([&target, &succeded](auto&& x) { succeded = applyCmd(target, x); }, *cmd);
    if (!succeded) {
      break;
    }
  }

  return cmd;
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
    result += ");}\n";
    return result;
  }
  else {
    return "ipasir_solve(" + solverVarName + ");\n";
  }
}
}

auto toCFunctionBody(FuzzTrace::const_iterator first,
                     FuzzTrace::const_iterator last,
                     std::string const& argName) -> std::string
{
  std::string result;
  for (FuzzTrace::const_iterator cmd = first; cmd != last; ++cmd) {
    std::visit([&result, &argName](auto&& x) { result += toString(argName, x) + "\n"; }, *cmd);
  }
  return result;
}

IOException::IOException(std::string const& what) : std::runtime_error(what) {}

namespace {
constexpr uint32_t magicCookie = 0xF2950000; // 0xF2950000 + format version

void appendToTraceBuffer(uint32_t value, std::vector<uint32_t>& buffer)
{
  buffer.push_back(toSmallEndian(value));
}

uint32_t litAsBinary(CNFLit lit)
{
  uint32_t absVal = std::abs(lit) << 1;
  absVal += (lit < 0 ? 1 : 0);
  return absVal;
}

void appendLitsToTraceBuffer(std::vector<CNFLit> const& lits, std::vector<uint32_t>& buffer)
{
  for (CNFLit lit : lits) {
    appendToTraceBuffer(litAsBinary(lit), buffer);
  }
}

void storeCmd(AddClauseCmd const& cmd, std::vector<uint32_t>& buffer)
{
  uint32_t header = 1 << 24 | cmd.clauseToAdd.size();
  appendToTraceBuffer(header, buffer);
  appendLitsToTraceBuffer(cmd.clauseToAdd, buffer);
}

void storeCmd(AssumeCmd const& cmd, std::vector<uint32_t>& buffer)
{
  uint32_t header = 2 << 24 | cmd.assumptions.size();
  appendToTraceBuffer(header, buffer);
  appendLitsToTraceBuffer(cmd.assumptions, buffer);
}

void storeCmd(SolveCmd const& cmd, std::vector<uint32_t>& buffer)
{
  uint32_t header = 3 << 24;
  if (cmd.expectedResult.has_value()) {
    header |= ((*cmd.expectedResult) ? 10 : 20);
  }
  appendToTraceBuffer(header, buffer);
}
}

void storeTrace(FuzzTrace::const_iterator first,
                FuzzTrace::const_iterator last,
                std::filesystem::path const& filename)
{
  std::vector<uint32_t> buffer;

  appendToTraceBuffer(magicCookie, buffer);

  for (FuzzTrace::const_iterator cmd = first; cmd != last; ++cmd) {
    std::visit([&buffer](auto&& x) { storeCmd(x, buffer); }, *cmd);
  }

  FILE* output = fopen(filename.string().c_str(), "w");
  if (output == nullptr) {
    throw IOException{"Could not open file " + filename.string()};
  }

  auto closeOutput = gsl::finally([output]() { fclose(output); });
  size_t written = fwrite(buffer.data(), buffer.size() * sizeof(uint32_t), 1, output);
  if (written != 1) {
    throw IOException{"I/O error while writing to " + filename.string()};
  }
}

namespace {
std::vector<CNFLit> readCNFLits(FILE* input, size_t size)
{
  std::vector<uint32_t> buffer;
  buffer.resize(size);
  size_t numRead = fread(buffer.data(), sizeof(uint32_t), size, input);
  if (numRead != size) {
    throw IOException{"Unexpected end of file"};
  }

  std::transform(buffer.begin(), buffer.end(), buffer.begin(), fromSmallEndian);

  std::vector<CNFLit> result;
  for (uint32_t i : buffer) {
    if (i == 0 || (i & 0xFF000000) != 0) {
      throw IOException{"Bad literal"};
    }

    int sign = ((i & 1) == 1 ? -1 : 1);
    result.push_back(static_cast<int32_t>(i / 2) * sign);
  }

  return result;
}

std::optional<FuzzCmd> readFuzzCmd(FILE* input)
{
  uint32_t cmdHeader;
  size_t const numReadHeader = fread(&cmdHeader, sizeof(uint32_t), 1, input);
  if (numReadHeader == 0) {
    return std::nullopt;
  }

  cmdHeader = fromSmallEndian(cmdHeader);

  uint32_t const type = cmdHeader >> 24;

  if (type == 1) {
    AddClauseCmd result;
    result.clauseToAdd = readCNFLits(input, cmdHeader & 0x00FFFFFF);
    return result;
  }
  else if (type == 2) {
    AssumeCmd result;
    result.assumptions = readCNFLits(input, cmdHeader & 0x00FFFFFF);
    return result;
  }
  else if (type == 3) {
    SolveCmd result;
    uint32_t expected = cmdHeader & 0xFF;
    if (expected == 10 || expected == 20) {
      result.expectedResult = (expected == 10);
    }
    else if (expected != 0) {
      throw IOException{"Invalid fuzz command value"};
    }
    return result;
  }
  else {
    throw IOException{"Invalid fuzz command header"};
  }
}

bool readMagicCookie(FILE* input)
{
  uint32_t cookie;
  size_t const numRead = fread(&cookie, sizeof(uint32_t), 1, input);
  if (numRead != 1) {
    return false;
  }

  return fromSmallEndian(cookie) == magicCookie;
}
}

auto loadTrace(std::filesystem::path const& filename) -> FuzzTrace
{
  FuzzTrace result;

  FILE* input = fopen(filename.string().c_str(), "r");
  if (input == nullptr) {
    throw IOException("Could not open file " + filename.string());
  }
  auto closeInput = gsl::finally([input]() { fclose(input); });

  if (!readMagicCookie(input)) {
    throw IOException{"Bad file format: magic cookie not found"};
  }

  std::optional<FuzzCmd> currentCmd = readFuzzCmd(input);
  while (currentCmd.has_value()) {
    result.push_back(*currentCmd);
    currentCmd = readFuzzCmd(input);
  }

  return result;
}
}
