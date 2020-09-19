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

#include <stdlib.h>

namespace incmonk::verifier {

auto operator<<(std::ostream& stream, Lit lit) -> std::ostream&
{
  if (!lit.isPositive()) {
    stream << "-";
  }
  stream << lit.getVar();
  return stream;
}

auto operator<<(std::ostream& stream, Var var) -> std::ostream&
{
  stream << var.getRawValue();
  return stream;
}

auto operator<<(std::ostream& stream, Clause const& clause) -> std::ostream&
{
  stream << "({";

  if (!clause.empty()) {
    stream << clause[0];
  }

  for (Lit lit : clause.getLiterals().subspan(1)) {
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

auto maxLit(Var var) noexcept -> Lit
{
  return Lit{var, true};
}

std::size_t sizeOfClauseInMem(Clause::size_type size) noexcept
{
  if (size <= 1) {
    return sizeof(Clause);
  }
  else {
    return sizeof(Clause) + sizeof(Lit) * (size - 1);
  }
}

ClauseCollection::ClauseCollection()
{
  resize(1 << 20);
}

ClauseCollection::~ClauseCollection()
{
  free(m_memory);
  m_memory = nullptr;
}

void ClauseCollection::resize(std::size_t newSize)
{
  char* newMemory = reinterpret_cast<char*>(realloc(m_memory, newSize));
  if (newMemory == nullptr) {
    throw std::bad_alloc{};
  }
  m_currentSize = newSize;
  m_memory = newMemory;
}

auto ClauseCollection::add(gsl::span<Lit const> lits,
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

auto ClauseCollection::resolve(Ref cref) noexcept -> Clause&
{
  assert(isValidRef(cref));
  char* clausePtr = m_memory + 4 * cref.m_offset;
  return *(reinterpret_cast<Clause*>(clausePtr));
}

auto ClauseCollection::resolve(Ref cref) const noexcept -> Clause const&
{
  assert(isValidRef(cref));
  char const* clausePtr = m_memory + 4 * cref.m_offset;
  return *(reinterpret_cast<Clause const*>(clausePtr));
}

auto ClauseCollection::begin() const noexcept -> RefIterator
{
  return RefIterator{m_memory, m_highWaterMark};
}

auto ClauseCollection::end() const noexcept -> RefIterator
{
  return RefIterator{};
}

auto ClauseCollection::isValidRef(Ref cref) const noexcept -> bool
{
  return (4 * cref.m_offset) < m_highWaterMark;
}

auto ClauseCollection::RefIterator::operator++() noexcept -> RefIterator&
{
  Clause const* clause = reinterpret_cast<Clause const*>(m_clausePtr);
  std::size_t clauseBytes = sizeOfClauseInMem(clause->size());

  m_clausePtr += clauseBytes;
  assert(clauseBytes <= m_distanceToEnd);
  m_distanceToEnd -= clauseBytes;
  m_currentRef.m_offset += (clauseBytes / 4);

  if (m_distanceToEnd == 0) {
    m_clausePtr = nullptr;
  }

  return *this;
}
}