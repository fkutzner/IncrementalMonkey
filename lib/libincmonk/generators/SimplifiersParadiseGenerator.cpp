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

#include <libincmonk/CNF.h>
#include <libincmonk/FastRand.h>
#include <libincmonk/FuzzTrace.h>
#include <libincmonk/generators/SimplifiersParadiseGenerator.h>

#include <algorithm>
#include <random>
#include <vector>

namespace incmonk {
namespace {

class LiteralFactory {
public:
  CNFLit newLit() noexcept { return ++m_nextLit; }

  CNFLit currentMaxLit() noexcept { return m_nextLit - 1; }

private:
  CNFLit m_nextLit = 1;
};

auto randomSplit(CNFClause&& clause, uint64_t seed) -> std::pair<CNFClause, CNFClause>
{
  assert(clause.size() >= 2);

  CNFClause::size_type const index = clause.size() == 2 ? 1 : (1 + (seed % (clause.size() - 2)));
  CNFClause result{clause.begin() + index, clause.end()};
  clause.resize(index);
  return {std::move(clause), std::move(result)};
}

namespace complicators {
auto splitOffDefinition(CNFClause const& clause, LiteralFactory& litFactory, uint64_t seed)
    -> std::vector<CNFClause>
{
  if (clause.size() < 2) {
    return {clause};
  }

  auto [base, gateInputs] = randomSplit(CNFClause{clause}, seed);

  CNFLit substitution = litFactory.newLit();
  base.emplace_back(substitution);

  std::vector<CNFClause> result;
  result.emplace_back(std::move(base));

  for (CNFLit lit : gateInputs) {
    result.push_back({substitution, -lit});
  }

  gateInputs.push_back(-substitution);
  result.emplace_back(std::move(gateInputs));

  return result;
}

auto createSubsumed(CNFClause const& clause, LiteralFactory&, uint64_t) -> std::vector<CNFClause>
{
  std::vector<CNFClause> result;
  result.push_back(clause);

  CNFClause subsumed = clause;
  CNFLit const min = *std::min_element(clause.begin(), clause.end());

  for (CNFLit embellishment = min / 2; embellishment > 0; embellishment /= 2) {
    subsumed.push_back(embellishment);
  }

  result.emplace_back(std::move(subsumed));

  return result;
}

auto hideInSSR(CNFClause const& clause, LiteralFactory& litFactory, uint64_t)
    -> std::vector<CNFClause>
{
  CNFLit const min = *std::min_element(clause.begin(), clause.end());
  CNFLit const resolveAt = (min > 1) ? min / 2 : litFactory.newLit();

  std::vector<CNFClause> result{clause, clause};
  result[0].push_back(-resolveAt);

  for (CNFLit embellishment = resolveAt / 2; embellishment > 0; embellishment /= 2) {
    result[1].push_back(embellishment);
  }

  return result;
}

auto introduceFailedLiteral(CNFClause const& clause, LiteralFactory& litFactory, uint64_t)
    -> std::vector<CNFClause>
{
  std::vector<CNFClause> result{clause};

  CNFLit origFailed = litFactory.newLit();

  CNFClause failedFwd = clause;
  failedFwd.push_back(origFailed);
  result.push_back(failedFwd);

  for (CNFLit l : clause) {
    result.push_back({-origFailed, -l});
  }

  CNFLit conseqFailed1 = litFactory.newLit();
  CNFLit conseqFailed2 = litFactory.newLit();
  CNFLit conseqFailed3 = litFactory.newLit();

  result.push_back({origFailed, -conseqFailed1});
  result.push_back({origFailed, -conseqFailed2});
  result.push_back({conseqFailed1, -conseqFailed3});
  result.push_back({conseqFailed2, -conseqFailed3});

  return result;
}

auto addTrivialRedundancies(CNFClause const& clause, LiteralFactory&, uint64_t seed)
    -> std::vector<CNFClause>
{
  if (seed % 64 == 0) {
    std::vector<CNFClause> result{clause};
    result.back().push_back(clause[0]);
    result.push_back(clause);
    result.back().push_back(-clause[0]);
    return result;
  }
  else {
    return {clause};
  }
}
}

auto selectComplicatorFn(uint64_t randomValue)
{
  constexpr std::array compFns{complicators::createSubsumed,
                               complicators::hideInSSR,
                               complicators::splitOffDefinition,
                               complicators::introduceFailedLiteral,
                               complicators::addTrivialRedundancies};
  return compFns[randomValue % compFns.size()];
}

auto complicate(uint64_t seed,
                uint64_t softMaxSize,
                LiteralFactory& litFactory,
                std::vector<CNFClause>&& problem) -> std::vector<CNFClause>
{
  XorShiftRandomBitGenerator rng{seed};

  while (problem.size() < softMaxSize) {
    std::swap(problem[rng() % problem.size()], problem.back());

    auto complicatorFn = selectComplicatorFn(rng());
    std::vector<CNFClause> replacement = complicatorFn(problem.back(), litFactory, rng());

    problem.pop_back();
    problem.insert(problem.end(), replacement.begin(), replacement.end());
  }

  return std::move(problem);
}

auto createRootProblem(LiteralFactory& litFactory) -> std::vector<CNFClause>
{
  CNFClause root;
  std::generate_n(std::back_inserter(root), 10, [&]() { return litFactory.newLit(); });
  return {std::move(root)};
}

auto createSimplifiersParadiseProblem(uint64_t seed, size_t softMaxSize, LiteralFactory& litFactory)
    -> std::vector<CNFClause>
{
  std::vector<CNFClause> rootProblem = createRootProblem(litFactory);
  return complicate(seed, softMaxSize, litFactory, std::move(rootProblem));
}

class SimplifiersParadiseGen final : public FuzzTraceGenerator {
public:
  SimplifiersParadiseGen(SimplifiersParadiseParams params)
    : m_rng{params.seed}, m_params{std::move(params)}
  {
  }

  auto generate() -> FuzzTrace override
  {
    size_t const size = std::floor(m_params.numClausesDistribution(m_rng));
    LiteralFactory litFactory;

    FuzzTrace problem;
    for (CNFClause& clause : createSimplifiersParadiseProblem(m_rng(), size, litFactory)) {
      problem.push_back(AddClauseCmd{std::move(clause)});
    }

    FuzzTrace result = insertSolveCmds(
        std::move(problem), m_params.solveCmdSchedule, litFactory.currentMaxLit(), m_rng());
    if (m_params.havocSchedule.has_value()) {
      return insertHavocCmds(std::move(result), *m_params.havocSchedule, m_rng());
    }
    else {
      return result;
    }
  }

private:
  XorShiftRandomBitGenerator m_rng;
  SimplifiersParadiseParams m_params;
};

}

auto createSimplifiersParadiseGen(SimplifiersParadiseParams params)
    -> std::unique_ptr<FuzzTraceGenerator>
{
  return std::make_unique<SimplifiersParadiseGen>(std::move(params));
}
}
