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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <ostream>
#include <sstream>


using ::testing::Eq;

MATCHER_P2(PiecewiseLinearDistInBounds, minResult, maxResult, "")
{
  return arg.min() == minResult && arg.max() == maxResult;
}

namespace incmonk {

std::ostream& operator<<(std::ostream& strm, ClosedInterval const& intv)
{
  strm << intv.min() << ":" << intv.max();
  return strm;
}

TEST(ConfigTests_getDefaultConfig, DefaultConfigContainsExpectedValues)
{
  Config result = getDefaultConfig(100);
  EXPECT_THAT(result.seed, Eq(100ull));
  EXPECT_THAT(result.configName, Eq("Default"));
  EXPECT_THAT(result.timeout, Eq(std::nullopt));

  CommunityAttachmentModelParams const& caParams = result.communityAttachmentModelParams;
  EXPECT_THAT(caParams.numClausesDistribution, PiecewiseLinearDistInBounds(200.0, 1200.0));
  EXPECT_THAT(caParams.clauseSizeDistribution, PiecewiseLinearDistInBounds(2.0, 10.0));
  EXPECT_THAT(caParams.numVariablesPerClauseDistribution, PiecewiseLinearDistInBounds(0.05, 0.25));
  EXPECT_THAT(caParams.modularityDistribution, PiecewiseLinearDistInBounds(0.7, 1.0));

  EXPECT_THAT(caParams.solveCmdSchedule.density, Eq(ClosedInterval{0.001, 0.05}));
  EXPECT_THAT(caParams.solveCmdSchedule.assumptionPhaseDensity, Eq(ClosedInterval{0.5, 0.7}));
  EXPECT_THAT(caParams.solveCmdSchedule.assumptionDensity, Eq(ClosedInterval{0.0, 0.2}));
  ASSERT_TRUE(caParams.havocSchedule.has_value());
  EXPECT_THAT(caParams.havocSchedule->density, Eq(ClosedInterval{0.0, 0.1}));
  EXPECT_THAT(caParams.havocSchedule->phaseDensity, Eq(ClosedInterval{0.0, 1.0}));

  SimplifiersParadiseParams const& spParams = result.simplifiersParadiseParams;
  EXPECT_THAT(spParams.numClausesDistribution, PiecewiseLinearDistInBounds(200.0, 1200.0));
  EXPECT_THAT(spParams.solveCmdSchedule.density, Eq(ClosedInterval{0.001, 0.05}));
  EXPECT_THAT(spParams.solveCmdSchedule.assumptionPhaseDensity, Eq(ClosedInterval{0.5, 0.7}));
  EXPECT_THAT(spParams.solveCmdSchedule.assumptionDensity, Eq(ClosedInterval{0.0, 0.2}));
  ASSERT_TRUE(spParams.havocSchedule.has_value());
  EXPECT_THAT(spParams.havocSchedule->density, Eq(ClosedInterval{0.0, 0.1}));
  EXPECT_THAT(spParams.havocSchedule->phaseDensity, Eq(ClosedInterval{0.0, 1.0}));
}

TEST(ConfigTests_extendConfigViaTOML, WhenTOMLIsInvalid_ThenErrorIsThrown)
{
  Config dummyConfig;
  std::stringstream input{"some string which is invalid TOML"};
  EXPECT_THROW(extendConfigViaTOML(dummyConfig, input), ConfigParseError);
}

TEST(ConfigTests_extendConfigViaTOML, WhenTOMLContainsUnknownKey_ThenErrorIsThrown)
{
  Config dummyConfig;
  std::stringstream input{"[[community_attachment_generator]]\nfoo=\"bar\""};
  EXPECT_THROW(extendConfigViaTOML(dummyConfig, input), ConfigParseError);
}

TEST(ConfigTests_extendConfigViaTOML, WhenTOMLContainsUnknownSection_ThenErrorIsThrown)
{
  Config dummyConfig;
  std::stringstream input{"[[foo]]"};
  EXPECT_THROW(extendConfigViaTOML(dummyConfig, input), ConfigParseError);
}

TEST(ConfigTests_extendConfigViaTOML, WhenTOMLContainsValidCASetting_ThenItIsApplied)
{
  Config config;
  config.communityAttachmentModelParams.solveCmdSchedule.density = ClosedInterval{0.0, 0.4};
  std::stringstream input{"[[community_attachment_generator]]\nsolve_density_interval=[0.3,0.5]"};

  Config result = extendConfigViaTOML(config, input);
  EXPECT_THAT(result.communityAttachmentModelParams.solveCmdSchedule.density,
              ::testing::Eq(ClosedInterval{0.3, 0.5}));
}

TEST(ConfigTests_extendConfigViaTOML, WhenTOMLContainsValidSPSetting_ThenItIsApplied)
{
  Config config;
  config.simplifiersParadiseParams.solveCmdSchedule.density = ClosedInterval{0.0, 0.4};
  std::stringstream input{"[[simplifiers_paradise_generator]]\nsolve_density_interval=[0.3,0.5]"};

  Config result = extendConfigViaTOML(config, input);
  EXPECT_THAT(result.simplifiersParadiseParams.solveCmdSchedule.density,
              ::testing::Eq(ClosedInterval{0.3, 0.5}));
}
}