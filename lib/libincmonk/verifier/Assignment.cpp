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

#include <libincmonk/verifier/Assignment.h>

namespace incmonk::verifier {

Assignment::Range::Range(Assignment::const_iterator start, Assignment::const_iterator stop) noexcept
  : m_start(start), m_stop(stop)
{
}

auto Assignment::Range::begin() const noexcept -> Assignment::const_iterator
{
  return m_start;
}

auto Assignment::Range::end() const noexcept -> Assignment::const_iterator
{
  return m_stop;
}

auto Assignment::Range::size() const noexcept -> Assignment::size_type
{
  return m_stop - m_start;
}

Assignment::Assignment(Lit maxLit)
  : m_assignmentMap{maxLit.getVar(), t_indet}
  , m_assignmentStack{}
  , m_numAssignments{0}
  , m_maxVar{maxLit.getVar()}
{
  m_assignmentStack.resize(m_maxVar.getRawValue() + 1);
}

void Assignment::clear(size_type start) noexcept
{
  if (start >= m_numAssignments) {
    return;
  }

  for (Lit toClear : range(start)) {
    m_assignmentMap[toClear.getVar()] = t_indet;
  }
  m_numAssignments = start;
}

auto Assignment::size() const noexcept -> size_type
{
  return m_numAssignments;
}

auto Assignment::empty() const noexcept -> bool
{
  return m_numAssignments == 0;
}

void Assignment::increaseSizeTo(Lit newMaxLit)
{
  newMaxLit = maxLit(newMaxLit.getVar());
  assert(newMaxLit.getVar().getRawValue() >= m_maxVar.getRawValue());
  m_assignmentStack.resize(newMaxLit.getVar().getRawValue() + 1);
  m_assignmentMap.increaseSizeTo(newMaxLit.getVar());
  m_maxVar = newMaxLit.getVar();
}
}