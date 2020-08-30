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

#include "FileUtils.h"

#include <libincmonk/IOUtils.h>

#include <gsl/gsl_util>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <random>

namespace fs = std::filesystem;

namespace incmonk {

PathWithDeleter::PathWithDeleter(fs::path const& path) : m_path{path} {}

auto PathWithDeleter::getPath() const noexcept -> fs::path const&
{
  return m_path;
}

PathWithDeleter::~PathWithDeleter()
{
  if (!m_path.empty()) {
    std::error_code ec;
    fs::remove(m_path, ec);
    if (ec) {
      std::cerr << "Warning: could not remove " << m_path << "\n";
    }
  }
}

PathWithDeleter::PathWithDeleter(PathWithDeleter&& rhs)
{
  if (!m_path.empty()) {
    fs::remove(m_path);
  }
  m_path = rhs.m_path;
  rhs.m_path.clear();
}

auto PathWithDeleter::operator=(PathWithDeleter&& rhs) -> PathWithDeleter&
{
  if (!m_path.empty()) {
    fs::remove(m_path);
  }
  m_path = rhs.m_path;
  rhs.m_path.clear();
  return *this;
}


namespace {
auto createTempFilename(fs::path const& dir) -> fs::path
{
  static std::mt19937 rng{32};
  static std::uniform_int_distribution<> numbers;

  fs::path candidate;
  do {
    candidate = dir / ("incmonk-tmp-" + std::to_string(numbers(rng)));
  } while (fs::exists(candidate));

  return candidate;
}
}

auto createTempFile() -> PathWithDeleter
{
  fs::path filename = createTempFilename(fs::temp_directory_path());
  std::ofstream create{filename.string().c_str()};
  return PathWithDeleter{filename};
}

auto createTempDir() -> PathWithDeleter
{
  fs::path dir = createTempFilename(fs::temp_directory_path());
  fs::create_directory(dir);
  return PathWithDeleter{dir};
}

auto slurpUInt32File(std::filesystem::path const& path) -> std::optional<std::vector<uint32_t>>
{
  FILE* input = fopen(path.string().c_str(), "r+");
  if (input == nullptr) {
    return std::nullopt;
  }

  auto fileCloser = gsl::finally([input]() { fclose(input); });

  std::vector<uint32_t> result;
  size_t nRead = 0;

  do {
    std::array<uint32_t, 256> buffer;
    nRead = fread(buffer.data(), 4, 256, input);
    std::copy(buffer.begin(), buffer.begin() + nRead, std::back_inserter(result));
  } while (nRead != 0);

  std::transform(result.begin(), result.end(), result.begin(), fromSmallEndian);
  return result;
}

void writeUInt32VecToFile(std::vector<uint32_t> const& data, std::filesystem::path const& path)
{
  std::vector<uint32_t> smallEndianData = data;
  std::transform(
      smallEndianData.begin(), smallEndianData.end(), smallEndianData.begin(), toSmallEndian);

  FILE* output = fopen(path.string().c_str(), "w");
  if (output == nullptr) {
    throw TestIOException("Failed to open file " + path.string());
  }
  auto fileCloser = gsl::finally([output]() { fclose(output); });

  std::size_t written =
      fwrite(smallEndianData.data(), smallEndianData.size() * sizeof(uint32_t), 1, output);

  if (written != 1) {
    throw TestIOException("Failed to write " + path.string());
  }
}
}
