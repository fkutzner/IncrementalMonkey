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

#include <libincmonk/InterspersionSchedulers.h>

#include <libincmonk/CNF.h>
#include <libincmonk/FastRand.h>
#include <libincmonk/FuzzTrace.h>

#include <random>


namespace incmonk {

namespace {
auto isBeginOfPhase(FuzzCmd const& cmd)
{
  return std::get_if<SolveCmd>(&cmd) != nullptr;
}
}

auto insertSolveCmds(FuzzTrace&& trace,
                     SolveCmdScheduleParams const& stochParams,
                     CNFLit maxLit,
                     uint64_t seed) -> FuzzTrace
{
  XorShiftRandomBitGenerator rng{seed};
  std::uniform_int_distribution<int> assumptionSignDist{0, 1};
  std::uniform_int_distribution<CNFLit> assumptionVarDist{1, std::abs(maxLit)};

  RandomDensityEventSchedule solveCmds{seed + 1, stochParams.density};
  RandomDensityEventSchedule assumeCmds{seed + 2, stochParams.assumptionDensity};
  RandomDensityEventSchedule phasesWithAssumptions{seed + 2, stochParams.assumptionPhaseDensity};

  FuzzTrace result;

  bool assumptionInsertionActive = phasesWithAssumptions.next();
  for (FuzzTrace::size_type idx = 0, end = trace.size(); idx < end; ++idx) {
    if (isBeginOfPhase(trace[idx])) {
      assumptionInsertionActive = phasesWithAssumptions.next();
    }

    result.push_back(std::move(trace[idx]));
    if (assumptionInsertionActive && assumeCmds.next()) {
      int32_t sign = 1 - assumptionSignDist(rng) * 2;
      CNFLit assumption = sign * assumptionVarDist(rng);
      result.push_back(AssumeCmd{{assumption}});
    }
    if (solveCmds.next()) {
      result.push_back(SolveCmd{});
    }
  }

  result.push_back(SolveCmd{});
  return result;
}


auto insertHavocCmds(FuzzTrace&& trace, HavocCmdScheduleParams const& stochParams, uint64_t seed)
    -> FuzzTrace
{
  XorShiftRandomBitGenerator rng{seed};
  std::uniform_int_distribution<uint64_t> havocValueDist;
  RandomDensityEventSchedule havocsWithinPhases{seed + 1, stochParams.density};
  RandomDensityEventSchedule phasesWithHavocs{seed + 2, stochParams.phaseDensity};

  FuzzTrace result;
  result.push_back(HavocCmd{havocValueDist(rng), true});

  bool havocActive = phasesWithHavocs.next();
  for (FuzzTrace::size_type idx = 0, end = trace.size(); idx < end; ++idx) {
    if (isBeginOfPhase(trace[idx])) {
      havocActive = phasesWithHavocs.next();
    }

    result.push_back(std::move(trace[idx]));
    if (havocActive && havocsWithinPhases.next()) {
      result.push_back(HavocCmd{havocValueDist(rng), false});
    }
  }

  return result;
}

}