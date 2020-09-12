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

#include <libincmonk/generators/CommunityAttachmentGenerator.h>
#include <libincmonk/generators/SimplifiersParadiseGenerator.h>

#include <istream>
#include <stdexcept>
#include <string>

namespace incmonk {

/// \brief Main Incremental Monkey config structure
///
/// This structure contains configuration for all of monkey's subsystems.
struct Config {
  std::string configName;
  std::string fuzzerID;
  uint64_t seed = 0;
  std::optional<uint64_t> timeout;

  CommunityAttachmentModelParams communityAttachmentModelParams;
  SimplifiersParadiseParams simplifiersParadiseParams;
};

class ConfigParseError : public std::runtime_error {
public:
  ConfigParseError(char const* what) : std::runtime_error(what) {}
  virtual ~ConfigParseError() = default;
};

/// \brief Returns the default Incremental Monkey configuration.
///
/// \throw ConfigParseError   when the builtin configuration contains errors
auto getDefaultConfig(uint64_t seed) -> Config;

/// \brief Creates a configuration updated with partial TOML configuration
///
/// \param toExtend   The configuration to extend
/// \param tomlStream A stream containing a partial, serialized Incremental
///                   Monkey config
auto extendConfigViaTOML(Config const& toExtend, std::istream& tomlStream) -> Config;


/// \brief Returns the default configuration as a TOML string
auto getDefaultConfigTOML() -> std::string;
}