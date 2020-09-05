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

#include <libincmonk/StochasticsUtils.h>

namespace incmonk {

ClosedInterval::ClosedInterval(double min, double max) noexcept : m_min{min}, m_max{max}
{
  if (m_max < m_min) {
    m_max = 0;
    m_min = 0;
  }
}

auto ClosedInterval::min() const noexcept -> double
{
  return m_min;
}

auto ClosedInterval::max() const noexcept -> double
{
  return m_max;
}

auto ClosedInterval::size() const noexcept -> double
{
  return m_max - m_min;
}


RandomDensityEventSchedule::RandomDensityEventSchedule(uint64_t seed, ClosedInterval densities)
  : m_dist{0.0, 1.0}, m_rng{seed}, m_density{densities.min() + m_dist(m_rng) * densities.size()}
{
}


auto RandomDensityEventSchedule::next() -> bool
{
  return m_dist(m_rng) <= m_density;
}
}