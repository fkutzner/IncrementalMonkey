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

#include <libincmonk/generators/RandomSATGenerator.h>

#include <libincmonk/CNF.h>

#include <random>

namespace incmonk {
namespace {
class RandomSATTraceGen final : public FuzzTraceGenerator {
public:
  RandomSATTraceGen(uint32_t seed) { m_rng.seed(seed); }

  auto generate() -> FuzzTrace override
  {
    std::uniform_int_distribution<int32_t> numClauseDist{350, 450};
    std::uniform_int_distribution<int32_t> clauseSizeDist{1, 16};
    std::uniform_int_distribution<int32_t> literalDist{1, 50};
    std::uniform_int_distribution<int32_t> signDist{1, 2};
    std::uniform_int_distribution<int32_t> phaseDist{20, 100};

    int32_t numClauses = numClauseDist(m_rng);
    int32_t phaseSize = phaseDist(m_rng);

    FuzzTrace result;
    for (int32_t i = 0; i < numClauses; ++i) {
      std::vector<CNFLit> clause;
      int32_t clauseSize = clauseSizeDist(m_rng);

      for (int32_t j = 0; j < clauseSize; ++j) {
        CNFLit literal = literalDist(m_rng);
        if (signDist(m_rng) == 2) {
          literal = -literal;
        }
        clause.push_back(literal);
      }

      result.push_back(AddClauseCmd{clause});

      --phaseSize;
      if (phaseSize == 0) {
        result.push_back(SolveCmd{});
        phaseSize = phaseDist(m_rng);
      }
    }
    result.push_back(SolveCmd{});

    return result;
  }

  virtual ~RandomSATTraceGen() = default;

private:
  std::mt19937 m_rng;
};
}

auto createRandomSATTraceGen(uint32_t seed) -> std::unique_ptr<FuzzTraceGenerator>
{
  return std::make_unique<RandomSATTraceGen>(seed);
}
}