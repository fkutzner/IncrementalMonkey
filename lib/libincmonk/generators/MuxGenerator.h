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
 * \brief Weighted FuzzTrace generator combination
 */

#pragma once

#include <libincmonk/generators/FuzzTraceGenerator.h>

#include <memory>
#include <vector>

namespace incmonk {
struct MuxGeneratorSpec {
  MuxGeneratorSpec(double weightParm, std::unique_ptr<FuzzTraceGenerator> generatorParm)
    : weight{weightParm}, generator{std::move(generatorParm)}
  {
  }

  /// The generator's weight. Example: if a generator A has weight 1.0 and a generator B
  /// has weight 2.0, then the probability of using B is twice the probability of using A.
  double weight = 0.0;
  std::unique_ptr<FuzzTraceGenerator> generator;
};

/**
 * Creates a FuzzTraceGenerator using provided fuzz trace generators for trace generation,
 * selecting generators at random using weights.
 * 
 * \param specs   the generators to be used for problem generation, with probability weights.
 *                `specs` must contain at least one element. The total of weights must be
 *                greater than 0.
 * \param seed    the random seed used for generator selection.
 * 
 * \returns       The multiplexing generator.
 */
auto createMuxGenerator(std::vector<MuxGeneratorSpec>&& specs, uint64_t seed)
    -> std::unique_ptr<FuzzTraceGenerator>;
}