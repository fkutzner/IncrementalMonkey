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

#include <libincmonk/ConfigTomlUtils.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>


namespace incmonk {

TEST(ConfigTomlUtilsTests_throwingCheckType, WhenNodeHasExpectedType_ThenNothingIsThrown)
{
  toml::value<double> floatingPointNode{1.0};
  throwingCheckType(
      floatingPointNode, toml::node_type::floating_point, "this should not be thrown");
}

TEST(ConfigTomlUtilsTests_throwingCheckType, WhenNodeHasUnexpectedType_ThenErrorIsThrown)
{
  toml::value<double> floatingPointNode{1.0};
  EXPECT_THROW(
      throwingCheckType(floatingPointNode, toml::node_type::array, "this should not be thrown"),
      TOMLConfigParseError);
}

TEST(ConfigTomlUtilsTests_parseFloatArray, WhenNodeIsNotArray_ThenErrorIsThrown)
{
  toml::value<double> floatingPointNode{1.0};
  EXPECT_THROW(parseFloatArray(floatingPointNode), TOMLConfigParseError);
}

TEST(ConfigTomlUtilsTests_parseFloatArray, WhenNodeContainsNonFloats_ThenErrorIsThrown)
{
  toml::array arrayNode{1.0, "foo"};
  EXPECT_THROW(parseFloatArray(arrayNode), TOMLConfigParseError);
}

TEST(ConfigTomlUtilsTests_parseFloatArray, WhenNodeIsEmpty_ThenEmptyFloatVecIsReturned)
{
  toml::array arrayNode{};
  EXPECT_THAT(parseFloatArray(arrayNode), ::testing::IsEmpty());
}

TEST(ConfigTomlUtilsTests_parseFloatArray, WhenNodeContainsSingleFloat_ThenFloatVecIsReturned)
{
  toml::array arrayNode{1.0};
  EXPECT_THAT(parseFloatArray(arrayNode), ::testing::ElementsAre(1.0));
}

TEST(ConfigTomlUtilsTests_parseFloatArray, WhenNodeContainsMultipleFloats_ThenFloatVecIsReturned)
{
  toml::array arrayNode{1.0, 2.0, 3.0};
  EXPECT_THAT(parseFloatArray(arrayNode), ::testing::ElementsAre(1.0, 2.0, 3.0));
}

TEST(ConfigTomlUtilsTests_parseZippedFloatArray, WhenNodeIsNotArray_ThenErrorIsThrown)
{
  toml::value<double> floatingPointNode{1.0};
  EXPECT_THROW(parseZippedFloatArray(floatingPointNode), TOMLConfigParseError);
}

TEST(ConfigTomlUtilsTests_parseZippedFloatArray, WhenNodeIsNotArrayOfArrays_ThenErrorIsThrown)
{
  toml::array arrayNode{1.0, 2.0, 3.0};
  EXPECT_THROW(parseZippedFloatArray(arrayNode), TOMLConfigParseError);
}

TEST(ConfigTomlUtilsTests_parseZippedFloatArray, WhenNodeIsNotArrayOfSize2Arrays_ThenErrorIsThrown)
{
  toml::array arrayNode{toml::array{1.0}, toml::array{2.0, 3.0}};
  EXPECT_THROW(parseZippedFloatArray(arrayNode), TOMLConfigParseError);
}

TEST(ConfigTomlUtilsTests_parseZippedFloatArray,
     WhenNodeIsNotArrayOfSize2FloatArrays_ThenErrorIsThrown)
{
  toml::array arrayNode{toml::array{1.0, "foo"}, toml::array{2.0, 3.0}};
  EXPECT_THROW(parseZippedFloatArray(arrayNode), TOMLConfigParseError);
}

TEST(ConfigTomlUtilsTests_parseZippedFloatArray, WhenNodeIsEmpty_ThenEmptyFloatVecsAreReturned)
{
  toml::array arrayNode{};
  auto [lhs, rhs] = parseZippedFloatArray(arrayNode);
  EXPECT_THAT(lhs, ::testing::IsEmpty());
  EXPECT_THAT(rhs, ::testing::IsEmpty());
}

TEST(ConfigTomlUtilsTests_parseZippedFloatArray,
     WhenNodeContainsSingleFloatPair_ThenFloatVecsAreReturned)
{
  toml::array arrayNode{};
  arrayNode.push_back(toml::array{1.0, 3.0});
  auto [lhs, rhs] = parseZippedFloatArray(arrayNode);
  EXPECT_THAT(lhs, ::testing::ElementsAre(1.0));
  EXPECT_THAT(rhs, ::testing::ElementsAre(3.0));
}

TEST(ConfigTomlUtilsTests_parseZippedFloatArray,
     WhenNodeContainsMultipleFloatPairs_ThenFloatVecsAreReturned)
{
  toml::array arrayNode{toml::array{1.0, 3.0}, toml::array{0.5, 0.0}};
  auto [lhs, rhs] = parseZippedFloatArray(arrayNode);
  EXPECT_THAT(lhs, ::testing::ElementsAre(1.0, 0.5));
  EXPECT_THAT(rhs, ::testing::ElementsAre(3.0, 0.0));
}

TEST(ConfigTomlUtilsTests_createIntervalParser, WhenNodeIsNotFloatArray_ThenErrorIsThrown)
{
  toml::value<double> floatingPointNode{1.0};
  ClosedInterval dummyTarget;
  auto parserUnderTest = createIntervalParser(dummyTarget);
  EXPECT_THROW(parserUnderTest(floatingPointNode), TOMLConfigParseError);
}

TEST(ConfigTomlUtilsTests_createIntervalParser, WhenNodeContainsNonFloatValues_ThenErrorIsThrown)
{
  toml::array arrayNode{1.0, "foo"};
  ClosedInterval dummyTarget;
  auto parserUnderTest = createIntervalParser(dummyTarget);
  EXPECT_THROW(parserUnderTest(arrayNode), TOMLConfigParseError);
}

TEST(ConfigTomlUtilsTests_createIntervalParser, WhenNodeDoesNotContain2Values_ThenErrorIsThrown)
{
  ClosedInterval dummyTarget;
  auto parserUnderTest = createIntervalParser(dummyTarget);
  EXPECT_THROW(parserUnderTest(toml::array{}), TOMLConfigParseError);
  EXPECT_THROW(parserUnderTest(toml::array{1.0}), TOMLConfigParseError);
  EXPECT_THROW(parserUnderTest(toml::array{1.0, 2.0, 3.0}), TOMLConfigParseError);
}

TEST(ConfigTomlUtilsTests_createIntervalParser, WhenNodeContainsExactly2Values_ThenIntervalIsFilled)
{
  ClosedInterval target;
  auto parserUnderTest = createIntervalParser(target);

  parserUnderTest(toml::array{-2.5, 3.5});
  EXPECT_THAT(target, ::testing::Eq(ClosedInterval{-2.5, 3.5}));
}

TEST(ConfigTomlUtilsTests_createPiecewiseLinearDistParser, WhenNodeIsNotArray_ThenErrorIsThrown)
{
  std::piecewise_linear_distribution<double> target;
  auto parserUnderTest = createPiecewiseLinearDistParser(target);
  EXPECT_THROW(parserUnderTest(toml::value<double>(1.0)), TOMLConfigParseError);
}

TEST(ConfigTomlUtilsTests_createPiecewiseLinearDistParser,
     WhenNodeIsNotArrayOfArrays_ThenErrorIsThrown)
{
  std::piecewise_linear_distribution<double> target;
  auto parserUnderTest = createPiecewiseLinearDistParser(target);
  EXPECT_THROW(parserUnderTest(toml::array{1.0}), TOMLConfigParseError);
}

TEST(ConfigTomlUtilsTests_createPiecewiseLinearDistParser,
     WhenNodeIsNotArrayOfIntervals_ThenErrorIsThrown)
{
  std::piecewise_linear_distribution<double> target;
  auto parserUnderTest = createPiecewiseLinearDistParser(target);
  EXPECT_THROW(parserUnderTest(toml::array{toml::array{1.0, "foo"}, toml::array{1.0, "foo"}}),
               TOMLConfigParseError);
  EXPECT_THROW(parserUnderTest(toml::array{toml::array{1.0, 2.0}, toml::array{1.0}}),
               TOMLConfigParseError);
  EXPECT_THROW(parserUnderTest(toml::array{toml::array{1.0, 2.0}, "foo"}), TOMLConfigParseError);
}

TEST(ConfigTomlUtilsTests_createPiecewiseLinearDistParser, WhenNodeIsEmpty_ThenTargetIsUnchanged)
{
  std::piecewise_linear_distribution<double> target;
  auto parserUnderTest = createPiecewiseLinearDistParser(target);
  parserUnderTest(toml::array{});
  EXPECT_THAT(target, ::testing::Eq(std::piecewise_linear_distribution<double>{}));
}

TEST(ConfigTomlUtilsTests_createPiecewiseLinearDistParser,
     WhenNodeContainsIntervals_ThenTargetIsConfigured)
{
  std::piecewise_linear_distribution<double> target;
  auto parserUnderTest = createPiecewiseLinearDistParser(target);
  parserUnderTest(toml::array{toml::array{1.0, 2.0}, toml::array{4.0, 0.0}, toml::array{5.0, 2.0}});
  EXPECT_THAT(target.min(), ::testing::Eq(1.0));
  EXPECT_THAT(target.max(), ::testing::Eq(5.0));
}
}