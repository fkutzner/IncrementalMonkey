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

#include <libincmonk/generators/FuzzTraceGenerator.h>
#include <libincmonk/generators/MuxGenerator.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <unordered_map>

namespace incmonk {
namespace {
class FakeTraceGenerator final : public FuzzTraceGenerator {
public:
  // Distinguishing fuzz trace generators by the length
  // of their traces
  FakeTraceGenerator(size_t traceLength) : m_traceLength{traceLength} {}

  auto generate() -> FuzzTrace override
  {
    FuzzTrace result;
    for (size_t i = 0; i < m_traceLength; ++i) {
      result.emplace_back(SolveCmd{});
    }
    return result;
  }

private:
  size_t m_traceLength;
};
}

class MuxGeneratorTests : public ::testing::TestWithParam<std::vector<double> /* weights*/> {
public:
  virtual ~MuxGeneratorTests() = default;
};

namespace {
template <typename V>
auto normalize(std::unordered_map<size_t, V> const& map) -> std::unordered_map<size_t, double>
{
  double total = 0.0;
  for (auto [k, v] : map) {
    total += static_cast<double>(v);
  }

  std::unordered_map<size_t, double> result;
  for (auto [k, v] : map) {
    result[k] = static_cast<double>(v) / total;
  }

  return result;
}

auto isEq(std::unordered_map<size_t, double> const& lhs,
          std::unordered_map<size_t, double> const& rhs) -> bool
{
  constexpr double absError = 0.01;

  for (auto [k, v] : lhs) {
    double rhsVal = 0.0;
    if (auto rhsIt = rhs.find(k); rhsIt != rhs.end()) {
      rhsVal = rhsIt->second;
    }

    if ((v - absError) > rhsVal || (v + absError) < rhsVal) {
      return false;
    }
  }

  for (auto [k, v] : rhs) {
    if (auto lhsIt = lhs.find(k); lhsIt == rhs.end() && (v < -absError || v > absError)) {
      return false;
    }
  }

  return true;
}
}

MATCHER_P(NormalizedValueRangesEq, expected, "")
{
  std::unordered_map<size_t, double> normalizedArg = normalize(arg);
  std::unordered_map<size_t, double> normalizedExpected = normalize(expected);
  return isEq(normalizedArg, normalizedExpected);
}

TEST_P(MuxGeneratorTests, GeneratorsAreSelectedAccordingToWeights)
{
  std::vector<double> weights = GetParam();
  std::unordered_map<size_t, double> expectedWeightByTraceSize;

  std::vector<MuxGeneratorSpec> generators;
  for (size_t idx = 0, end = weights.size(); idx < end; ++idx) {
    expectedWeightByTraceSize[idx] = weights[idx];
    generators.emplace_back(weights[idx], std::make_unique<FakeTraceGenerator>(idx));
  }

  auto underTest = createMuxGenerator(std::move(generators), 100ull);

  std::unordered_map<size_t, size_t> occurrencesByTraceSize;
  constexpr size_t rounds = 10000;
  for (size_t i = 0; i < rounds; ++i) {
    FuzzTrace result = underTest->generate();
    occurrencesByTraceSize[result.size()] += 1;
  }

  EXPECT_THAT(occurrencesByTraceSize, NormalizedValueRangesEq(expectedWeightByTraceSize));
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(, MuxGeneratorTests,
  ::testing::Values(
    std::vector<double>{1.0},
    std::vector<double>{1.0, 0.5},
    std::vector<double>{0.0, 0.5},
    std::vector<double>{1.0, 0.0, 0.5},
    std::vector<double>{10.0, 0.1, 0.5}
  )
);
// clang-format on

}