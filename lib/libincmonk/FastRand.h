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
#include <limits>

namespace incmonk {

/**
 * \brief A UniformRandomBitGenerator implementing Marsaglia's 64-bit xorshift generator.
 * 
 * See Sebastiano Vigna, "An experimental exploration of Marsaglia's xorshift generators, scrambled"
 * (http://vigna.di.unimi.it/ftp/papers/xorshift.pdf)
 */
class XorShiftRandomBitGenerator {
public:
  using result_type = uint64_t;

  explicit XorShiftRandomBitGenerator(uint64_t seed) noexcept;
  auto operator()() noexcept -> result_type;

  constexpr static auto min() -> result_type;
  constexpr static auto max() -> result_type;

private:
  std::uint64_t m_state;
};

XorShiftRandomBitGenerator::XorShiftRandomBitGenerator(uint64_t seed) noexcept : m_state{seed} {
}

constexpr auto XorShiftRandomBitGenerator::min() -> result_type {
  return std::numeric_limits<uint64_t>::min();
}

constexpr auto XorShiftRandomBitGenerator::max() -> result_type {
  return std::numeric_limits<uint64_t>::max();
}

auto XorShiftRandomBitGenerator::operator()() noexcept -> result_type {
  constexpr uint64_t mult = 2685821657736338717ull;
  m_state ^= m_state >> 12;
  m_state ^= m_state << 25;
  m_state ^= m_state >> 27;
  m_state = m_state * mult;
  return m_state;
}

}