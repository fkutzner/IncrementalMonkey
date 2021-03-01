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

/**
 * \file
 * 
 * \brief Types for ternary logic
 */

#pragma once

#include <libincmonk/CNF.h>

#include <cstdint>
#include <ostream>

namespace incmonk {

/// \brief Ternary bool data type
class TBool {
public:
  constexpr TBool() : m_value{0} {}

  constexpr auto operator==(TBool const& rhs) const noexcept
  {
    return this == &rhs || m_value == rhs.m_value;
  }
  constexpr auto operator!=(TBool const& rhs) const noexcept { return !(*this == rhs); }

  constexpr auto operator!() const -> TBool { return TBool{static_cast<int8_t>(-m_value)}; }

  static constexpr auto createTrue() -> TBool { return TBool{1}; }

  static constexpr auto createFalse() -> TBool { return TBool{-1}; }

  static constexpr auto createIndet() -> TBool { return TBool{}; }

private:
  constexpr explicit TBool(int8_t const& value) : m_value{value} {}

  int8_t m_value;
};

constexpr TBool t_false = TBool::createFalse(); //< Ternary false
constexpr TBool t_true = TBool::createTrue();   //< Ternary true
constexpr TBool t_indet = TBool::createIndet(); //< Ternary indeterminate

inline auto operator<<(std::ostream& target, TBool tbool) -> std::ostream&
{
  if (tbool == t_false) {
    target << "false";
  }
  else if (tbool == t_true) {
    target << "true";
  }
  else {
    target << "indet";
  }
  return target;
}

}