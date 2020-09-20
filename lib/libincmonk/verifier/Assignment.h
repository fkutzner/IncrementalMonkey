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

#include <libincmonk/TBool.h>
#include <libincmonk/verifier/BoundedMap.h>
#include <libincmonk/verifier/Clause.h>

#include <cassert>
#include <vector>

namespace incmonk::verifier {

class Assignment {
private:
  using AssignmentStack = std::vector<Lit>;

public:
  using size_type = AssignmentStack::size_type;
  using const_iterator = AssignmentStack::const_iterator;

  class Range {
  public:
    Range(Assignment::const_iterator start, Assignment::const_iterator stop) noexcept;
    auto begin() const noexcept -> Assignment::const_iterator;
    auto end() const noexcept -> Assignment::const_iterator;
    auto size() const noexcept -> size_type;

  private:
    const_iterator m_start;
    const_iterator m_stop;
  };

  explicit Assignment(Lit maxLit);

  void add(Lit lit) noexcept;
  auto get(Lit lit) const noexcept -> TBool;
  auto range(size_type start = 0) const noexcept -> Range;
  void clear(size_type start = 0) noexcept;
  auto size() const noexcept -> size_type;
  auto empty() const noexcept -> bool;

  void increaseSizeTo(Lit newMaxLit);

  Assignment(Assignment const& rhs) noexcept = delete;
  Assignment(Assignment&& rhs) noexcept = default;
  auto operator=(Assignment const& rhs) noexcept -> Assignment& = delete;
  auto operator=(Assignment&& rhs) noexcept -> Assignment& = default;

private:
  BoundedMap<Var, TBool> m_assignmentMap;
  AssignmentStack m_assignmentStack;
  size_type m_numAssignments;
  Var m_maxVar;
};

//
// Implementation
//

inline void Assignment::add(Lit lit) noexcept
{
  assert(m_assignmentMap[lit.getVar()] == t_indet);
  assert(m_numAssignments < m_assignmentStack.size());

  m_assignmentMap[lit.getVar()] = lit.isPositive() ? t_true : t_false;
  m_assignmentStack[m_numAssignments] = lit;
  ++m_numAssignments;
}

inline auto Assignment::get(Lit lit) const noexcept -> TBool
{
  assert(lit.getVar().getRawValue() <= m_maxVar.getRawValue());
  TBool varAssignment = m_assignmentMap[lit.getVar()];
  return lit.isPositive() ? varAssignment : !varAssignment;
}

inline auto Assignment::range(size_type start) const noexcept -> Range
{
  auto const begin = m_assignmentStack.begin();
  auto const end = m_assignmentStack.begin() + m_numAssignments;

  if (start >= m_numAssignments) {
    return Range{end, end};
  }
  return Range{begin + start, end};
}
}