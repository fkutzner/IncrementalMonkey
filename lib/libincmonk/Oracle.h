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
 * \brief Interface for test oracles, which are used for validating the results
 *   produced by the solver under test
 */

#pragma once

#include <libincmonk/FuzzTrace.h>
#include <libincmonk/TBool.h>

#include <memory>
#include <vector>

namespace incmonk {

/**
 * \brief The test oracle
 * 
 * Fills in the expected results in SolveCmd trace elements using a known-good
 * SAT solver.
 */
class Oracle {
public:
  /**
   * \brief Fills in the expected results in SolveCmd trace elements in [start, stop)
   */
  virtual void solve(FuzzTrace::iterator start, FuzzTrace::iterator stop) = 0;

  /**
   * \brief Checks whether the problem so far added to the oracle is satisfiable
   *        under the assumptions `assumptions`.
   * 
   * \returns t_indet if the oracle could not determine satisfiability. Otherwise,
   *          t_true if and only if `toCheck` satisfies the clauses so far added to
   *          the oracle.
   */
  virtual auto probe(std::vector<CNFLit> const& assumptions) -> TBool = 0;


  /**
   * \brief Returns the assumptions that would be used in the next solve call.
   * 
   * \returns The assumptions as described above. The variables contained in the result
   *   are distinct.
   */
  virtual auto getCurrentAssumptions() const -> std::vector<CNFLit> = 0;


  /**
   * \brief Returns the maxium variable so far seen in trace clauses or assumptions.
   */
  virtual auto getMaxSeenLit() const -> CNFLit = 0;

  /**
   * \brief Clears the assumptions that would be used in the next solve call.
   */
  virtual void clearAssumptions() = 0;

  virtual ~Oracle() = default;
};

/**
 * \brief Creates a test oracle.
 */
auto createOracle() -> std::unique_ptr<Oracle>;
}
