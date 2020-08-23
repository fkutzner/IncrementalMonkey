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

#include <libincmonk/SolveCmdScheduler.h>

#include <libincmonk/CNF.h>
#include <libincmonk/FastRand.h>
#include <libincmonk/FuzzTrace.h>

#include <random>


namespace incmonk {
auto insertSolveCmds(FuzzTrace&& trace, CNFLit maxLit, uint64_t seed) -> FuzzTrace
{
  // TODO: configurable parameter distributions

  XorShiftRandomBitGenerator rng{seed};

  // TODO: add more interesting cases (consecutive solve calls,
  // assumption-free solve calls, all solves at end, ...)
  // TODO: tune parameters, these are arbitrary
  std::uniform_real_distribution<double> dist{0.0, 1.0};
  std::uniform_int_distribution<CNFLit> assumptionDist{1, std::abs(maxLit)};
  double const assumeProb = 0.1;
  double const solveProb = 0.01;

  FuzzTrace result;

  for (FuzzTrace::size_type idx = 0, end = trace.size(); idx < end; ++idx) {
    result.push_back(std::move(trace[idx]));
    if (dist(rng) < assumeProb) {
      int32_t sign = (dist(rng) < 0.5 ? 1 : -1);
      CNFLit assumption = sign * assumptionDist(rng);
      result.push_back(AssumeCmd{{assumption}});
    }
    if (dist(rng) < solveProb) {
      result.push_back(SolveCmd{});
    }
  }

  result.push_back(SolveCmd{});
  return result;
}

}