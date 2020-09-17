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

#include "Utils.h"

#include <libincmonk/FuzzTrace.h>

#include <algorithm>
#include <filesystem>

namespace incmonk {
namespace {
void wrapVarsAt16M(std::vector<CNFLit>& lits)
{
  constexpr CNFLit maxAbsLit = (1 << 24) - 1;
  for (CNFLit& lit : lits) {
    lit = std::clamp(lit, -maxAbsLit, maxAbsLit);
  }
}

void wrapVarsAt16M(FuzzTrace& trace)
{
  for (FuzzCmd& traceElement : trace) {
    std::visit(
        [](auto&& cmd) {
          using CmdT = std::decay_t<decltype(cmd)>;
          if constexpr (std::is_same_v<AddClauseCmd, CmdT>) {
            wrapVarsAt16M(cmd.clauseToAdd);
          }
          else if constexpr (std::is_same_v<AssumeCmd, CmdT>) {
            wrapVarsAt16M(cmd.assumptions);
          }
        },
        traceElement);
  }
}
}

auto loadTraceFromFileOrStdin(std::filesystem::path const& path, bool parsePermissive) -> FuzzTrace
{
  LoaderStrictness const strictness =
      parsePermissive ? LoaderStrictness::PERMISSIVE : LoaderStrictness::PERMISSIVE;

  FuzzTrace result;
  if (path == std::filesystem::path{"-"}) {
    result = loadTrace(*stdin, strictness);
  }
  else {
    result = loadTrace(path, strictness);
  }

  if (parsePermissive) {
    wrapVarsAt16M(result);
  }

  return result;
}
}