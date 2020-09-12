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

#include <libincmonk/FastRand.h>
#include <libincmonk/generators/MuxGenerator.h>

#include <cassert>
#include <random>

namespace incmonk {
namespace {

auto getTotalOfWeights(std::vector<MuxGeneratorSpec> const& specs) -> double
{
  double result = 0.0;
  for (MuxGeneratorSpec const& spec : specs) {
    result += spec.weight;
  }
  assert(result > 0.0);
  return result;
}

class MuxGenerator final : public FuzzTraceGenerator {
public:
  MuxGenerator(std::vector<MuxGeneratorSpec>&& specs, uint64_t seed)
    : m_rng{seed}, m_selectionDist{0.0, getTotalOfWeights(specs)}, m_specs{std::move(specs)}
  {
  }

  auto generate() -> FuzzTrace override
  {
    if (m_specs.empty()) {
      return {};
    }

    double selectingWeight = m_selectionDist(m_rng);
    return getGenerator(selectingWeight).generate();
  }

  virtual ~MuxGenerator() = default;

private:
  auto getGenerator(double selectionWeight) -> FuzzTraceGenerator&
  {
    double currentWeight = 0.0;
    for (MuxGeneratorSpec& spec : m_specs) {
      currentWeight += spec.weight;
      if (selectionWeight <= currentWeight) {
        return *(spec.generator);
      }
    }

    assert(false && "Error: failed to look up generator");
    return *(m_specs[0].generator);
  }

  XorShiftRandomBitGenerator m_rng;
  std::uniform_real_distribution<double> m_selectionDist;
  std::vector<MuxGeneratorSpec> m_specs;
};
}


auto createMuxGenerator(std::vector<MuxGeneratorSpec>&& specs, uint64_t seed)
    -> std::unique_ptr<FuzzTraceGenerator>
{
  assert(!specs.empty());
  return std::make_unique<MuxGenerator>(std::move(specs), seed);
}
}