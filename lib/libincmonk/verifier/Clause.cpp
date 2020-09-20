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

#include <libincmonk/FastRand.h>
#include <libincmonk/verifier/BoundedMap.h>
#include <libincmonk/verifier/Clause.h>

#include <tsl/hopscotch_set.h>

#include <algorithm>
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


namespace {
auto hashLits(gsl::span<Lit const> lits) noexcept -> uint64_t
{
  // TODO: replace by proper order-independent hash function
  uint64_t result = XorShiftRandomBitGenerator{lits.size()}();
  for (Lit lit : lits) {
    result = result ^ XorShiftRandomBitGenerator { lit.getRawValue() }();
  }
  return result;
}

class CRefHash {
public:
  explicit CRefHash(ClauseCollection const& clauses) : m_clauses{&clauses} {}

  auto operator()(CRef cref) const noexcept -> std::size_t
  {
    Clause const& clause = m_clauses->resolve(cref);
    return hashLits(clause.getLiterals());
  }

  auto operator()(gsl::span<Lit const> lits) const noexcept -> std::size_t
  {
    return hashLits(lits);
  }

private:
  ClauseCollection const* m_clauses;
};

class CRefEq {
public:
  using is_transparent = void;

  explicit CRefEq(ClauseCollection const& clauses) : m_clauses{&clauses} {}

  auto operator()(CRef lhs, CRef rhs) const noexcept { return lhs == rhs; }

  auto operator()(gsl::span<Lit const> lhs, gsl::span<Lit const> rhs) const noexcept
  {
    if (lhs.size() != rhs.size()) {
      return false;
    }

    if (&*lhs.begin() == &*rhs.begin()) {
      return true;
    }

    return std::is_permutation(lhs.begin(), lhs.end(), rhs.begin());
  }

  auto operator()(CRef lhs, gsl::span<Lit const> rhs) const noexcept
  {
    return (*this)(m_clauses->resolve(lhs).getLiterals(), rhs);
  }

  auto operator()(gsl::span<Lit const> lhs, CRef rhs) const noexcept
  {
    return (*this)(lhs, m_clauses->resolve(rhs).getLiterals());
  }

private:
  ClauseCollection const* m_clauses;
};
}

class ClauseFinder {
public:
  ClauseFinder(ClauseCollection const& clauses) : m_refs{100, CRefHash{clauses}, CRefEq{clauses}}
  {
    for (CRef cref : clauses) {
      add(cref);
    }
  }

  auto find(gsl::span<Lit const> lits) const noexcept -> std::optional<CRef>
  {
    if (auto it = m_refs.find(lits); it != m_refs.end()) {
      return *it;
    }
    return std::nullopt;
  }

  void add(CRef cref) { m_refs.insert(cref); }

private:
  tsl::hopscotch_set<CRef, CRefHash, CRefEq> m_refs;
};

class ClauseOccurrences {
public:
  ClauseOccurrences(ClauseCollection const& clauses) : m_occurrences{1_Lit}, m_maxLit{1_Lit}
  {
    for (CRef cref : clauses) {
      add(cref, clauses.resolve(cref).getLiterals());
    }
  }

  void add(CRef cref, gsl::span<Lit const> lits)
  {
    for (Lit lit : lits) {
      // TODO: Lit, Var comparison operators
      if (lit.getVar().getRawValue() > m_maxLit.getVar().getRawValue()) {
        m_maxLit = maxLit(lit.getVar());
        m_occurrences.increaseSizeTo(m_maxLit);
      }
      m_occurrences[lit].push_back(cref);
    }
  }

  auto get(Lit lit) const noexcept -> gsl::span<CRef const>
  {
    if (lit.getVar().getRawValue() > m_maxLit.getVar().getRawValue()) {
      return {};
    }
    return m_occurrences[lit];
  }

private:
  BoundedMap<Lit, std::vector<CRef>> m_occurrences;
  Lit m_maxLit;
};


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

auto ClauseCollection::add(LitSpan lits,
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

  if (m_clauseFinder != nullptr) {
    m_clauseFinder->add(result);
  }

  if (m_clauseOccurrences != nullptr) {
    m_clauseOccurrences->add(result, lits);
  }

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

void ClauseCollection::markDeleted(Ref cref, ProofSequenceIdx atIdx)
{
  assert(isValidRef(cref));
  Clause& clause = resolve(cref);
  assert(clause.getDelIdx() < std::numeric_limits<ProofSequenceIdx>::max());

  if (!m_deletedClauses.empty()) {
    assert(resolve(m_deletedClauses.back()).getDelIdx() <= atIdx);
  }

  clause.setDelIdx(atIdx);
  m_deletedClauses.push_back(cref);
}

auto ClauseCollection::getDeletedClausesOrdered() const noexcept -> DeletedClausesRng
{
  return m_deletedClauses;
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

ClauseCollection::ClauseCollection(ClauseCollection&& rhs) noexcept
{
  *this = std::move(rhs);
}

auto ClauseCollection::operator=(ClauseCollection&& rhs) -> ClauseCollection&
{
  this->m_memory = rhs.m_memory;
  this->m_currentSize = rhs.m_currentSize;
  this->m_highWaterMark = rhs.m_highWaterMark;
  this->m_deletedClauses = std::move(rhs.m_deletedClauses);
  this->m_clauseFinder = std::move(rhs.m_clauseFinder);

  rhs.m_memory = nullptr;
  rhs.m_currentSize = 0;
  rhs.m_highWaterMark = 0;
  rhs.m_deletedClauses.clear();
  return *this;
}

auto ClauseCollection::find(LitSpan lits) const noexcept -> std::optional<Ref>
{
  if (m_clauseFinder == nullptr) {
    m_clauseFinder = std::make_unique<ClauseFinder>(*this);
  }
  return m_clauseFinder->find(lits);
}

auto ClauseCollection::getOccurrences(Lit lit) const noexcept -> OccRng
{
  if (m_clauseOccurrences == nullptr) {
    m_clauseOccurrences = std::make_unique<ClauseOccurrences>(*this);
  }
  return m_clauseOccurrences->get(lit);
}
}