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

#include <gsl/gsl_util>

#include <fstream>
#include <random>

#include <iostream>

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
    fs::remove(m_path);
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


auto createTempFile() -> PathWithDeleter
{
  static std::mt19937 rng{32};
  std::uniform_int_distribution<> numbers;

  fs::path tempDir = fs::temp_directory_path();
  fs::path candidate;
  do {
    candidate = tempDir / ("incmonk-tmp-" + std::to_string(numbers(rng)));
  } while (fs::exists(candidate));

  std::ofstream create{candidate.string().c_str()};
  return PathWithDeleter{candidate};
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
    // TODO: deal with endianness
    std::copy(buffer.begin(), buffer.begin() + nRead, std::back_inserter(result));
  } while (nRead != 0);

  return result;
}

void writeUInt32VecToFile(std::vector<uint32_t> data, std::filesystem::path const& path)
{
  // TODO: deal with endianness

  FILE* output = fopen(path.string().c_str(), "w");
  if (output == nullptr) {
    throw TestIOException("Failed to open file " + path.string());
  }
  auto fileCloser = gsl::finally([output]() { fclose(output); });

  std::size_t written = fwrite(data.data(), data.size(), 1, output);
  if (written != 1) {
    throw TestIOException("Failed to write " + path.string());
  }
}
}