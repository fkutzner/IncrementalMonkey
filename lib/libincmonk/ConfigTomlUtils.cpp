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

#include <tomlplusplus/toml.hpp>

#include <sstream>
#include <string>

namespace incmonk {

namespace {
auto createTOMLConfigParseErrorWhat(std::string_view reason, toml::node const& location)
    -> std::string
{
  std::stringstream result;
  result << "Error at " << location.source().begin << ": " << reason;
  return result.str();
}
}


TOMLConfigParseError::TOMLConfigParseError(std::string_view reason, toml::node const& location)
  : m_message(createTOMLConfigParseErrorWhat(reason, location))
{
}


auto TOMLConfigParseError::what() const noexcept -> char const*
{
  return m_message.c_str();
}


void throwingCheckType(toml::node const& node,
                       toml::node_type expectedType,
                       std::string_view errorMessage)
{
  if (node.type() != expectedType) {
    throw TOMLConfigParseError{errorMessage, node};
  }
}


namespace {
auto createPWDist(std::vector<double> const& vals, std::vector<double> const& weights)
    -> std::piecewise_linear_distribution<double>
{
  return std::piecewise_linear_distribution<double>(vals.begin(), vals.end(), weights.begin());
}
}


auto parseFloatArray(toml::node const& node) -> std::vector<double>
{
  std::vector<double> result;

  throwingCheckType(node, toml::node_type::array, "value is not a floating-point array");

  toml::array const* nodeContents = node.as_array();
  if (nodeContents->empty()) {
    return {};
  }

  if (!nodeContents->is_homogeneous(toml::node_type::floating_point)) {
    throw TOMLConfigParseError{"value is not a floating-point array", node};
  }

  for (toml::node const& node : *nodeContents) {
    result.push_back(**node.as_floating_point());
  }

  return result;
}


auto parseZippedFloatArray(toml::node const& node)
    -> std::pair<std::vector<double>, std::vector<double>>
{
  std::vector<double> leftResult;
  std::vector<double> rightResult;

  throwingCheckType(node, toml::node_type::array, "element is not an array of float intervals");

  toml::array const* nodeContents = node.as_array();
  if (nodeContents->empty()) {
    return {};
  }

  if (!nodeContents->is_homogeneous(toml::node_type::array)) {
    throw TOMLConfigParseError{"element is not an array of float intervals", node};
  }

  for (toml::node const& childNode : *nodeContents) {
    std::vector<double> vals = parseFloatArray(childNode);
    if (vals.size() != 2) {
      throw TOMLConfigParseError{
          "interval element does not contain exactly two floating-point numbers", childNode};
    }

    leftResult.push_back(vals[0]);
    rightResult.push_back(vals[1]);
  }

  return std::make_pair(leftResult, rightResult);
}


auto createIntervalParser(ClosedInterval& target) -> TOMLNodeParserFn
{
  return [&target](toml::node const& node) {
    std::vector<double> bounds = parseFloatArray(node);
    if (bounds.size() != 2) {
      throw TOMLConfigParseError{"interval settings must have exactly two elements", node};
    }

    target = ClosedInterval{bounds[0], bounds[1]};
  };
}


auto createPiecewiseLinearDistParser(std::piecewise_linear_distribution<double>& target)
    -> TOMLNodeParserFn
{
  return [&target](toml::node const& node) {
    auto [values, weights] = parseZippedFloatArray(node);
    target = createPWDist(values, weights);
  };
}

}