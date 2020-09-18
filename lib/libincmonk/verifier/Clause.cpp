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

#include <libincmonk/verifier/Clause.h>

namespace incmonk::verifier {

auto operator<<(std::ostream& stream, Clause const& clause) -> std::ostream&
{
  stream << "({";

  if (!clause.empty()) {
    stream << clause[0];
  }

  for (CNFLit lit : clause.getLiterals().subspan(1)) {
    stream << ", " << lit;
  }
  stream << "}; addIdx=" << clause.getAddIdx() << ", delIdx=" << clause.getDelIdx()
         << ", state=" << static_cast<uint32_t>(clause.getState()) << ")";
  return stream;
}

auto operator<<(std::ostream& stream, BinaryClause const& clause) -> std::ostream&
{
  stream << "(otherLit= " << clause.getOtherLit() << ", delIdx=" << clause.getDelIdx()
         << ", state=" << static_cast<uint32_t>(clause.getState()) << ")";
  return stream;
}

std::size_t sizeOfClauseInMem(Clause::size_type size) noexcept
{
  if (size <= 1) {
    return sizeof(Clause);
  }
  else {
    return sizeof(Clause) + sizeof(CNFLit) * (size - 1);
  }
}

ClauseAllocator::ClauseAllocator()
{
  resize(1 << 20);
}

ClauseAllocator::~ClauseAllocator()
{
  free(m_memory);
  m_memory = nullptr;
}

void ClauseAllocator::resize(std::size_t newSize)
{
  m_currentSize = newSize;
  m_memory = reinterpret_cast<char*>(realloc(m_memory, m_currentSize));
}

auto ClauseAllocator::allocate(gsl::span<CNFLit const> lits,
                               ClauseVerificationState initialState,
                               ProofSequenceIdx addIdx) -> Ref
{
  auto const numLits = lits.size();
  std::size_t sizeInMem = sizeOfClauseInMem(numLits);
  if (m_highWaterMark + sizeInMem > m_currentSize) {
    resize(std::max(m_highWaterMark + sizeInMem, 2 * m_currentSize));
  }

  Clause* resultClause = new (m_memory + m_highWaterMark) Clause(numLits, initialState, addIdx);
  for (Clause::size_type idx = 0; idx < numLits; ++idx) {
    (*resultClause)[idx] = lits[idx];
  }

  Ref result;
  result.m_offset = m_highWaterMark / 4;
  m_highWaterMark += sizeInMem;
  return result;
}

auto ClauseAllocator::resolve(Ref cref) noexcept -> Clause&
{
  char* clausePtr = m_memory + 4 * cref.m_offset;
  return *(reinterpret_cast<Clause*>(clausePtr));
}

auto ClauseAllocator::resolve(Ref cref) const noexcept -> Clause const&
{
  char const* clausePtr = m_memory + 4 * cref.m_offset;
  return *(reinterpret_cast<Clause const*>(clausePtr));
}

}