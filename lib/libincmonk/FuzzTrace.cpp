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

#include <cassert>
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

auto operator==(HavocCmd const& lhs, HavocCmd const& rhs) noexcept -> bool
{
  return &lhs == &rhs || lhs.seed == rhs.seed;
}

auto operator!=(HavocCmd const& lhs, HavocCmd const& rhs) noexcept -> bool
{
  return !(lhs == rhs);
}

auto operator<<(std::ostream& stream, HavocCmd const& cmd) -> std::ostream&
{
  stream << "(HavocCmd " << (cmd.beforeInit ? "pre-init " : "") << cmd.seed << ")";
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

auto applyCmd(IPASIRSolver& solver, SolveCmd const&) -> bool
{
  solver.solve();
  return false;
}

auto applyCmd(IPASIRSolver& solver, HavocCmd const& cmd) -> bool
{
  if (cmd.beforeInit) {
    solver.reinitializeWithHavoc(cmd.seed);
  }
  else {
    solver.havoc(cmd.seed);
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
    bool doContinue = true;
    std::visit([&target, &doContinue](auto&& x) { doContinue = applyCmd(target, x); }, *cmd);
    if (!doContinue) {
      break;
    }
  }

  return cmd;
}


IOException::IOException(std::string const& what) : std::runtime_error(what) {}

namespace {
constexpr uint32_t magicCookie = 0xF2950001; // 0xF2950000 + format version

constexpr uint8_t addClauseCmdId = 0;
constexpr uint8_t assumeCmdId = 1;
constexpr uint8_t solveWithoutExpectedResultCmdId = 2;
constexpr uint8_t solveWithFalseResultCmdId = 3;
constexpr uint8_t solveWithTrueResultCmdId = 4;
constexpr uint8_t havocInitCmdId = 5;
constexpr uint8_t havocCmdId = 6;
constexpr uint8_t maxCmdId = 6;


template <typename I>
void appendToTraceFile(I value, FILE* stream)
{
  static_assert(std::is_integral_v<I>);
  I valueToWrite = toSmallEndian(value);
  size_t const written = fwrite(&valueToWrite, sizeof(valueToWrite), 1, stream);
  if (written != 1) {
    throw IOException{"Write failure"};
  }
}

uint32_t litAsBinary(CNFLit lit)
{
  uint32_t absVal = std::abs(lit) << 1;
  absVal += (lit < 0 ? 1 : 0);
  return absVal;
}

void appendLitsToTraceFile(std::vector<CNFLit> const& lits, FILE* stream)
{
  for (CNFLit lit : lits) {
    appendToTraceFile<uint32_t>(litAsBinary(lit), stream);
  }
  appendToTraceFile<uint32_t>(0, stream);
}

void storeCmd(AddClauseCmd const& cmd, FILE* stream)
{
  appendToTraceFile<uint8_t>(addClauseCmdId, stream);
  appendLitsToTraceFile(cmd.clauseToAdd, stream);
}

void storeCmd(AssumeCmd const& cmd, FILE* stream)
{
  appendToTraceFile<uint8_t>(assumeCmdId, stream);
  appendLitsToTraceFile(cmd.assumptions, stream);
}

void storeCmd(SolveCmd const& cmd, FILE* stream)
{
  if (!cmd.expectedResult.has_value()) {
    appendToTraceFile<uint8_t>(solveWithoutExpectedResultCmdId, stream);
  }
  else {
    uint8_t const cmdIdWithValue =
        *(cmd.expectedResult) ? solveWithTrueResultCmdId : solveWithFalseResultCmdId;
    appendToTraceFile<uint8_t>(cmdIdWithValue, stream);
  }
}

void storeCmd(HavocCmd const& cmd, FILE* stream)
{
  uint8_t cmdId = cmd.beforeInit ? havocInitCmdId : havocCmdId;
  appendToTraceFile<uint8_t>(cmdId, stream);
  appendToTraceFile<uint64_t>(cmd.seed, stream);
}
}

void storeTrace(FuzzTrace::const_iterator first,
                FuzzTrace::const_iterator last,
                std::filesystem::path const& filename)
{
  FILE* output = fopen(filename.string().c_str(), "w");
  if (output == nullptr) {
    throw IOException{"Could not open file " + filename.string()};
  }
  auto closeOutput = gsl::finally([output]() { fclose(output); });

  try {
    appendToTraceFile<uint32_t>(magicCookie, output);
    for (FuzzTrace::const_iterator cmd = first; cmd != last; ++cmd) {
      std::visit([&output](auto&& x) { storeCmd(x, output); }, *cmd);
    }
  }
  catch (IOException const&) {
    throw IOException{"I/O error while writing to " + filename.string()};
  }
}

namespace {
std::vector<CNFLit> readCNFLits(FILE* input)
{
  std::vector<uint32_t> buffer;

  bool clauseEndReached = false;
  while (!clauseEndReached) {
    uint32_t nextLit = 0;
    if (fread(&nextLit, sizeof(uint32_t), 1, input) != 1) {
      throw IOException{"Unexpected end of literal sequence"};
    }

    clauseEndReached = (nextLit == 0);
    if (!clauseEndReached > 0) {
      buffer.push_back(nextLit);
    }
  }

  std::transform(buffer.begin(), buffer.end(), buffer.begin(), fromSmallEndian<uint32_t>);

  std::vector<CNFLit> result;
  for (uint32_t i : buffer) {
    int sign = ((i & 1) == 1 ? -1 : 1);
    result.push_back(static_cast<int32_t>(i / 2) * sign);
  }

  return result;
}

auto readHavocSeed(FILE* input) -> uint64_t
{
  uint64_t result = 0;
  size_t numRead = fread(&result, sizeof(uint64_t), 1, input);
  if (numRead != 1) {
    throw IOException{"Unexpected end of file"};
  }
  return fromSmallEndian(result);
}

auto decodeSolveResult(uint8_t commandID) -> std::optional<bool>
{
  if (commandID == solveWithoutExpectedResultCmdId) {
    return std::nullopt;
  }
  else if (commandID == solveWithFalseResultCmdId) {
    return false;
  }
  else {
    assert(commandID == solveWithTrueResultCmdId);
    return true;
  }
}

std::optional<FuzzCmd> readFuzzCmd(FILE* input, LoaderStrictness strictness)
{
  uint8_t command = 0;
  size_t const numRead = fread(&command, 1, 1, input);
  if (numRead == 0) {
    return std::nullopt;
  }

  if (strictness == LoaderStrictness::PERMISSIVE) {
    command = command % (maxCmdId + 1);
  }

  if (command == addClauseCmdId) {
    AddClauseCmd result;
    result.clauseToAdd = readCNFLits(input);
    return result;
  }
  else if (command == assumeCmdId) {
    AssumeCmd result;
    result.assumptions = readCNFLits(input);
    return result;
  }
  else if (command == solveWithoutExpectedResultCmdId || command == solveWithFalseResultCmdId ||
           command == solveWithTrueResultCmdId) {
    SolveCmd result;
    result.expectedResult = decodeSolveResult(command);
    return result;
  }
  else if (command == havocInitCmdId || command == havocCmdId) {
    // command == 6: pre-init havoc cmd
    // command == 7: regular havoc cmd
    HavocCmd result;
    result.seed = readHavocSeed(input);
    result.beforeInit = (command == havocInitCmdId);
    return result;
  }
  else {
    throw IOException{"Invalid fuzz command"};
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

auto loadTrace(std::filesystem::path const& filename, LoaderStrictness strictness) -> FuzzTrace
{
  FuzzTrace result;

  FILE* input = fopen(filename.string().c_str(), "r");
  if (input == nullptr) {
    throw IOException("Could not open file " + filename.string());
  }
  auto closeInput = gsl::finally([input]() { fclose(input); });

  return loadTrace(*input, strictness);
}

auto loadTrace(FILE& stream, LoaderStrictness strictness) -> FuzzTrace
{
  FuzzTrace result;

  if (!readMagicCookie(&stream) && strictness == LoaderStrictness::STRICT) {
    throw IOException{"Bad file format: magic cookie not found"};
  }

  try {
    std::optional<FuzzCmd> currentCmd = readFuzzCmd(&stream, strictness);
    while (currentCmd.has_value()) {
      result.push_back(*currentCmd);
      currentCmd = readFuzzCmd(&stream, strictness);
    }
  }
  catch (IOException const&) {
    if (strictness == LoaderStrictness::STRICT) {
      throw;
    }
  }

  return result;
}
}
