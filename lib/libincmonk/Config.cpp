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
#include <libincmonk/ConfigTomlUtils.h>
#include <libincmonk/generators/CommunityAttachmentGenerator.h>

#include <cstdint>
#include <istream>
#include <unordered_map>
#include <vector>

#include <tomlplusplus/toml.hpp>


namespace incmonk {
namespace {

constexpr char const* defaultConfig = R"z(
[[community_attachment_generator]]
# Distributions are specified as piecewise linear distributions, given as pairs [value, weight]
# See https://en.cppreference.com/w/cpp/numeric/random/piecewise_linear_distribution

num_clauses_distribution = [[200.0, 0.0], [400.0, 1.0], [600.0, 0.0], [800.0, 0.0], [1000.0, 1.0], [1200.0, 0.0]]
clause_size_distribution = [[2.0, 0.0], [4.0, 1.0], [10.0, 0.0]]
num_vars_per_num_clauses_distribution = [[0.05, 1.0], [0.25, 1.0]]
modularities_distribution = [[0.7, 0.0], [0.8, 0.0], [1.0, 0.0]]

# The average density of ipasir_solve calls (among clause additions) is picked
# at random the interval given in solve_density_interval.
solve_density_interval = [0.001, 0.05]

# The average density of solve-to-solve phases containing ipasir_assume calls
# is picked at random from the interval given in assumption_phase_density_interval.
assumption_phase_density_interval = [0.5, 0.7]

# The average density of ipasir_assume calls (among clause additions) is picked
# at random from the interval given in assumption_density_interval.
assumption_density_interval = [0.0, 0.2]

# The average density of solve-to-solve phases containing incmonk_havoc calls
# is picked at random from the interval given in havoc_phase_density_interval.
havoc_phase_density_interval = [0.0, 1.0]

# The average density of incmonk_havoc (among clause additions, solve calls,
# assume calls) is picked at random from the interval given in havoc_density_interval.
havoc_density_interval = [0.0, 0.1]
)z";


auto createCAModelParamsParsers(CommunityAttachmentModelParams& target)
    -> std::unordered_map<std::string, TOMLNodeParserFn>
{
  // clang-format off
  return {
    {"havoc_phase_density_interval", createIntervalParser(target.havocSchedule->phaseDensity)},
    {"havoc_density_interval", createIntervalParser(target.havocSchedule->density)},
    {"solve_density_interval", createIntervalParser(target.solveCmdSchedule.density)},
    {"assumption_density_interval", createIntervalParser(target.solveCmdSchedule.assumptionDensity)},
    {"assumption_phase_density_interval", createIntervalParser(target.solveCmdSchedule.assumptionPhaseDensity)},
    {"num_clauses_distribution", createPiecewiseLinearDistParser(target.numClausesDistribution)},
    {"clause_size_distribution", createPiecewiseLinearDistParser(target.clauseSizeDistribution)},
    {"num_vars_per_num_clauses_distribution",createPiecewiseLinearDistParser(target.numVariablesPerClauseDistribution)},
    {"modularities_distribution", createPiecewiseLinearDistParser(target.modularityDistribution)}
  };
  // clang-format on
}

void overrideCAModelParams(toml::node const& caConfig, CommunityAttachmentModelParams& target)
{
  auto const parsers = createCAModelParamsParsers(target);
  target.havocSchedule = target.havocSchedule.value_or(HavocCmdScheduleParams{});

  throwingCheckType(caConfig, toml::node_type::array, "invalid document structure");

  for (toml::node const& configTable : *caConfig.as_array()) {
    throwingCheckType(configTable, toml::node_type::table, "invalid document structure");

    for (auto configItem : *configTable.as_table()) {
      if (auto parser = parsers.find(configItem.first); parser != parsers.end()) {
        parser->second(configItem.second);
      }
      else {
        throw TOMLConfigParseError{"invalid key " + configItem.first, configItem.second};
      }
    }
  }
}

void applyTOMLConfig(toml::table const& config, Config& target)
{
  try {
    for (auto toplevelItem : config) {
      if (toplevelItem.first == "community_attachment_generator") {
        overrideCAModelParams(toplevelItem.second, target.communityAttachmentModelParams);
      }
      else {
        throw TOMLConfigParseError{"invalid item " + toplevelItem.first, config};
      }
    }
  }
  catch (TOMLConfigParseError const& exception) {
    throw ConfigParseError{exception.what()};
  }
}
}

auto getDefaultConfig(uint64_t seed) -> Config
{
  Config result;
  result.configName = "Default";
  result.seed = seed;
  result.communityAttachmentModelParams.seed = seed + 10;

  try {
    applyTOMLConfig(toml::parse(defaultConfig), result);
  }
  catch (toml::parse_error const& exception) {
    throw ConfigParseError{exception.what()};
  }
  return result;
}

auto extendConfigViaTOML(Config const& toExtend, std::istream& tomlStream) -> Config
{
  Config result = toExtend;
  try {
    applyTOMLConfig(toml::parse(tomlStream), result);
  }
  catch (toml::parse_error const& exception) {
    throw ConfigParseError{exception.what()};
  }
  return result;
}

auto getDefaultConfigTOML() -> std::string
{
  return defaultConfig;
}
}