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

#include <libincmonk/generators/GiraldezLevyGenerator.h>

#include <libincmonk/CNF.h>

#include <algorithm>
#include <random>

namespace incmonk {
namespace {
class GiraldezLevyGen final : public FuzzTraceGenerator {
public:
  GiraldezLevyGen(uint32_t seed) { m_rng.seed(seed); }


  // Fills communityIndices with the index for each literal of the clause
  // under construction. Afterwards, communityIndices[i] = x means that
  // clauseUnderConstruction[i] will get a variable in community x.
  void selectCommunities(std::vector<int32_t>& communityIndices,
                         int32_t numCommunities,
                         double modularity)
  {
    std::uniform_real_distribution<> sameCommunityDistr{0.0, 1.0};
    std::uniform_int_distribution<int32_t> communityIdDistr{0, numCommunities - 1};

    double p = modularity + 1.0 / static_cast<double>(numCommunities);

    if (sameCommunityDistr(m_rng) <= p) {
      // All literals of the clause will belong to the same community
      int32_t community = communityIdDistr(m_rng);
      std::fill(communityIndices.begin(), communityIndices.end(), community);
    }
    else {
      // Communities of literals will be pairwise distinct
      for (int32_t& c : communityIndices) {
        do {
          c = communityIdDistr(m_rng);
        } while (m_communityStampBuffer[c] != 0);
        m_communityStampBuffer[c] = 1;
      }

      for (int32_t c : communityIndices) {
        m_communityStampBuffer[c] = 0;
      }
    }
  }

  bool hasDuplicates(std::vector<CNFLit>& vec)
  {
    if (vec.size() < 4) {
      std::sort(vec.begin(), vec.end());
      return std::adjacent_find(vec.begin(), vec.end()) != vec.end();
    }
    else {
      bool result = false;
      for (CNFLit lit : vec) {
        assert(lit > 0);
        if (m_variableStampBuffer[lit] == 1) {
          result = false;
          break;
        }
      }

      for (CNFLit lit : vec) {
        assert(lit > 0);
        m_variableStampBuffer[lit] = 0;
      }

      return result;
    }
  }

  void generateClause(std::vector<CNFLit>& targetBuffer,
                      std::vector<int32_t> const& communityIndices,
                      int32_t numVariables,
                      int32_t numCommunities)
  {
    do {
      for (int i = 0; i < targetBuffer.size(); ++i) {
        double nc = static_cast<double>(numVariables) / static_cast<double>(numCommunities);

        // community indices {0, ..., c-1}, while in the paper they are {1, ..., c}
        uint32_t lowerBound = std::floor(communityIndices[i] * nc) + 1;
        uint32_t upperBound = std::floor((communityIndices[i] + 1) * nc);
        std::uniform_int_distribution<int32_t> varDist(lowerBound, upperBound);
        targetBuffer[i] = varDist(m_rng);
      }
    } while (hasDuplicates(targetBuffer));

    std::uniform_int_distribution<int32_t> signDistr{0, 1};
    for (CNFLit& lit : targetBuffer) {
      lit = signDistr(m_rng) == 1 ? lit : -lit;
    }
  }

  auto generate(uint32_t numClauses,       // m > 0
                uint32_t numVariables,     // n > 0
                uint32_t numCommunities,   // c > 0
                uint32_t numLitsPerClause, // k > 0
                double modularity          // Q
                ) -> FuzzTrace
  {
    FuzzTrace result;

    std::vector<int32_t> communityIndices, clauseBuffer;
    communityIndices.resize(numLitsPerClause);
    clauseBuffer.resize(numLitsPerClause);

    m_communityStampBuffer.clear();
    m_communityStampBuffer.resize(numCommunities);
    m_variableStampBuffer.clear();
    m_variableStampBuffer.resize(numVariables + 1);

    for (uint32_t j = 1; j <= numClauses; ++j) {
      selectCommunities(communityIndices, numCommunities, modularity);
      generateClause(clauseBuffer, communityIndices, numVariables, numCommunities);
      result.push_back(AddClauseCmd{clauseBuffer});
    }

    return result;
  }

  auto generate() -> FuzzTrace override
  {
    // numLitsPerClause <= c <= numVars/numLitsPerClause
    return generate(1000, 100, 10, 3, 0.8);
  }

  virtual ~GiraldezLevyGen() = default;

private:
  std::mt19937 m_rng;
  std::vector<int32_t> m_communityStampBuffer;
  std::vector<int32_t> m_variableStampBuffer;
};
}

auto createGiraldezLevyGen(uint32_t seed) -> std::unique_ptr<FuzzTraceGenerator>
{
  return std::make_unique<GiraldezLevyGen>(seed);
}
}