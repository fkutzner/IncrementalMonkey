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

#include "GenTrace.h"

#include <libincmonk/Config.h>
#include <libincmonk/FuzzTrace.h>

#include <fstream>
#include <iostream>
#include <memory>


namespace incmonk {

namespace {
auto getConfig(GenTraceParams const& params) -> Config
{
  Config cfg = getDefaultConfig(params.seed);
  if (params.disableHavoc) {
    cfg.communityAttachmentModelParams.havocSchedule = std::nullopt;
    cfg.simplifiersParadiseParams.havocSchedule = std::nullopt;
  }

  if (params.configFile.has_value()) {
    std::ifstream inputFile{*params.configFile};
    if (!inputFile) {
      throw std::runtime_error("could not open the config file");
    }
    return extendConfigViaTOML(cfg, inputFile);
  }
  else {
    return cfg;
  }
}
}

auto genTraceMain(GenTraceParams const& params) -> int
{
  std::unique_ptr<Config> cfg;
  try {
    cfg = std::make_unique<Config>(getConfig(params));
  }
  catch (std::runtime_error const& error) {
    std::cerr << "Failed loading the configuration: " << error.what() << "\n";
    return EXIT_FAILURE;
  }

  std::unique_ptr<FuzzTraceGenerator> generator;
  if (params.generator == "cam") {
    generator = createCommunityAttachmentGen(std::move(cfg->communityAttachmentModelParams));
  }
  else {
    assert(params.generator == "simp-para");
    generator = createSimplifiersParadiseGen(std::move(cfg->simplifiersParadiseParams));
  }

  FuzzTrace const toDump = generator->generate();

  try {
    storeTrace(toDump.begin(), toDump.end(), params.outputFile);
  }
  catch (IOException const& e) {
    std::cerr << "Writing the trace failed: " << e.what();
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
}