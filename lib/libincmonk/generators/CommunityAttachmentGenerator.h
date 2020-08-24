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

#pragma once

#include <libincmonk/generators/FuzzTraceGenerator.h>

#include <memory>
#include <random>

namespace incmonk {

struct CommunityAttachmentModelParams {
  std::piecewise_linear_distribution<double> numClausesDistribution;
  std::piecewise_linear_distribution<double> clauseSizeDistribution;
  std::piecewise_linear_distribution<double> numVariablesPerClauseDistribution;
  std::piecewise_linear_distribution<double> modularityDistribution;
  uint64_t seed = 1;
};

/**
 * Trace generator based on:
 * 
 * Jesús Giráldez-Cru and Jordi Levy. "A modularity-based random SAT instances generator."
 * IJCAI'15: Proceedings of the 24th International Conference on Artificial Intelligence
 * 
 * For each generated trace, the community attachment parameters are chosen as follows:
 * 
 * - the numberOfClauses is picked from params.numClausesDistribution (rounded to nearest
 *   integer)
 * 
 * - the clauseSize is picked from params.clauseSizeDistribution, but a minimum of 2 is used.
 * 
 * - the numberOfVars/numberOfClauses ratio is picked from numVariablesPerClauseDistribution
 *   (clamped to [0.0, 1.0]). The actual numberOfVars is computed using this ratio, but
 *   is at least clauseSize^2 (required by the community attachment model).
 * 
 * - the modularity is picked from params.modularityDistribution and clamped to [0.0, 1.0].
 * 
 */
auto createCommunityAttachmentGen(CommunityAttachmentModelParams params)
    -> std::unique_ptr<FuzzTraceGenerator>;
}
