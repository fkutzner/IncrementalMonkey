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

#pragma once

#include <libincmonk/StochasticsUtils.h>

#include <exception>
#include <functional>
#include <random>
#include <utility>
#include <vector>

#include <tomlplusplus/toml.hpp>

namespace incmonk {

using TOMLNodeParserFn = std::function<void(toml::node const&)>;

class TOMLConfigParseError : std::exception {
public:
  TOMLConfigParseError(std::string_view reason, toml::node const& location);
  virtual ~TOMLConfigParseError() = default;

  auto what() const noexcept -> char const*;

private:
  std::string m_message;
};

/// \brief Checks the type of the given node
///
/// If `node` is not of type `expectedType`, a TOMLConfigParseError is thrown
/// with error message `errorMessage`.
void throwingCheckType(toml::node const& node,
                       toml::node_type expectedType,
                       std::string_view errorMessage);

/// \brief Parses the given node as an array of floating-point values.
///
/// \param node   The node to be parsed
/// \returns      A vector containing the elements given in `node`
///
/// \throws TOMLConfigParseError    if `node` is not an array of floating-point values
auto parseFloatArray(toml::node const& node) -> std::vector<double>;

/// \brief Parses the given node as an array of floating-point pair arrays,
///    i.e. `[[x_1,y_1], [x_2, y_2], ...]`
///
/// \param node   The node to be parsed
/// \returns      A pair of vectors `x_1, x_2, ...` and `y_1, y_2, ...`
///
/// \throws TOMLConfigParseError    if `node` is not an array of size-2
///                                 floating-point arrays
auto parseZippedFloatArray(toml::node const& node)
    -> std::pair<std::vector<double>, std::vector<double>>;

/// \brief Creates a TOMLNodeParserFn parsing the node as a size-2
///   floating-point array
///
/// \param target   The ClosedInterval where the result is stored
///
/// The returned function throws TOMLConfigParseError on parse failures.
auto createIntervalParser(ClosedInterval& target) -> TOMLNodeParserFn;

/// \brief Creates a TOMLNodeParserFn parsing the node as piecewise linear
///   distribution on `double` values
///
/// The returned function parses nodes representing [[x_1,y_1], [x_2, y_2], ...]
/// where `x_i` and `y_i` are floating-point values.
///
/// \param target   The ClosedInterval where the result is stored
///
/// The returned function throws TOMLConfigParseError on parse failures.
auto createPiecewiseLinearDistParser(std::piecewise_linear_distribution<double>& target)
    -> TOMLNodeParserFn;
}
