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

#include <cstdint>
#include <filesystem>
#include <optional>
#include <stdexcept>
#include <variant>
#include <vector>

namespace incmonk {
class PathWithDeleter {
public:
  PathWithDeleter(std::filesystem::path const& path);
  auto getPath() const noexcept -> std::filesystem::path const&;
  ~PathWithDeleter();

  PathWithDeleter(PathWithDeleter&& rhs);
  auto operator=(PathWithDeleter&& rhs) -> PathWithDeleter&;
  PathWithDeleter(PathWithDeleter const&) = delete;
  auto operator=(PathWithDeleter const&) -> PathWithDeleter& = delete;

private:
  std::filesystem::path m_path;
};

auto createTempFile() -> PathWithDeleter;
auto createTempDir() -> PathWithDeleter;

class TestIOException : public std::runtime_error {
public:
  TestIOException(std::string const& what) : std::runtime_error(what) {}
  virtual ~TestIOException() = default;
};

using BinaryTrace = std::vector<std::variant<uint8_t, uint16_t, uint32_t, uint64_t>>;

void assertFileContains(std::filesystem::path const& path, BinaryTrace const& expected);
void writeBinaryTrace(std::filesystem::path const& path, BinaryTrace const& trace);

//auto slurpUInt32File(std::filesystem::path const& path) -> std::optional<std::vector<uint32_t>>;
//void writeUInt32VecToFile(std::vector<uint32_t> const& data, std::filesystem::path const& path);
}