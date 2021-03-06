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
 * \brief Functions for adding solve, assume and havoc commands to FuzzTrace objects,
 *   used by FuzzTrace generators
 */

#pragma once

#include <libincmonk/FuzzTrace.h>
#include <libincmonk/StochasticsUtils.h>

#include <cstdint>

namespace incmonk {

struct SolveCmdScheduleParams {
  /// Density of solve calls within the trace
  ClosedInterval density{0.0, 0.02};

  /// Density of assumptions within phases (solve-to-solve regions
  /// within the trace)
  ClosedInterval assumptionDensity{0.0, 0.2};

  /// Density of solve-to-solve regions where assumption insertion
  /// is active
  ClosedInterval assumptionPhaseDensity{0.0, 1.0};
};

/**
 * \brief Creates a trace with additional random solve and assume commands
 * 
 * \param trace       The trace to which the commands will be added
 * \param stochParams Parameters for stochastic choices
 * \param maxLit      The literal with the greatest variable occurring in `trace`
 * \param seed        RNG seed
 * 
 * \returns A trace containing all clauses of `trace`, preserving their order,
 *   interspersed with random assume and solve commands
 */
auto insertSolveCmds(FuzzTrace&& trace,
                     SolveCmdScheduleParams const& stochParams,
                     CNFLit maxLit,
                     uint64_t seed) -> FuzzTrace;


struct HavocCmdScheduleParams {
  /// Density of havoc commands within phases (solve-to-solve regions
  /// within the trace) where insertion is active
  ClosedInterval density{0.0, 0.05};

  /// Density of phases where havoc commands are inserted
  ClosedInterval phaseDensity{0.0, 1.0};
};

/**
 * \brief Creates a trace with additional random havoc commands
 * 
 * \param trace       The trace to which the commands will be added
 * \param stochParams Parameters for stochastic choices
 * \param seed        RNG seed
 * 
 * \returns A trace containing all clauses of `trace`, preserving their order,
 *   interspersed with random regular havoc commands, starting with
 *   a pre-init havoc command.
 */
auto insertHavocCmds(FuzzTrace&& trace, HavocCmdScheduleParams const& stochParams, uint64_t seed)
    -> FuzzTrace;
}