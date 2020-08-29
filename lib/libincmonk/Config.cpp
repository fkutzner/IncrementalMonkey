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

#include <libincmonk/Config.h>
#include <libincmonk/generators/CommunityAttachmentGenerator.h>

#include <tomlplusplus/toml.hpp>

#include <cstdint>
#include <random>
#include <vector>

namespace incmonk {
namespace {

auto createPWDist(std::vector<double> const& vals, std::vector<double> const& weights)
    -> std::piecewise_linear_distribution<double>
{
  return std::piecewise_linear_distribution<double>(vals.begin(), vals.end(), weights.begin());
}

auto createDefaultCAModelParams(uint64_t seed) -> CommunityAttachmentModelParams
{
  // clang-format off
  std::vector<double> const       problemSizes = {200.0, 400.0, 600.0, 800.0, 1000.0, 1200.0};
  std::vector<double> const problemSizeWeights = {  0.0,   1.0,   0.0,   0.0,    1.0,    0.0};

  std::vector<double> const       clauseSizes = {2.0, 4.0, 10.0};
  std::vector<double> const clauseSizeWeights = {0.0, 1.0,  0.0};

  std::vector<double> const       varsPerClauses = {0.05, 0.25};
  std::vector<double> const varsPerClauseWeights = {1.0,  1.0};

  std::vector<double> const      modularities = {0.0, 0.7, 0.8, 1.0};
  std::vector<double> const modularityWeights = {0.0, 0.0, 1.0, 0.0};
  // clang-format on

  CommunityAttachmentModelParams result;
  result.numClausesDistribution = createPWDist(problemSizes, problemSizeWeights);
  result.clauseSizeDistribution = createPWDist(clauseSizes, clauseSizeWeights);
  result.numVariablesPerClauseDistribution = createPWDist(varsPerClauses, varsPerClauseWeights);
  result.modularityDistribution = createPWDist(modularities, modularityWeights);
  result.seed = seed;
  return result;
}

}

auto getConfig(std::string const&, uint64_t seed) -> Config
{
  Config result;
  result.configName = "Default";
  result.seed = seed;
  result.communityAttachmentModelParams = createDefaultCAModelParams(seed);
  return result;
}
}