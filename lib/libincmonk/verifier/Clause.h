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

#include <libincmonk/CNF.h>

#include <gsl/span>
#include <limits>
#include <optional>
#include <ostream>

namespace incmonk::verifier {

enum class ClauseVerificationState : uint8_t {
  /// The clause is part of the problem instance, no verification required
  Irrendundant = 0,

  /// The clause is a lemma and has not yet been determined to be relevant for the proof
  Passive = 1,

  /// The clause is a lemma and has been determinined relevant for the proof. RAT property
  /// verification is pending
  VerificationPending = 2,

  /// The clause is a lemma and its RAT property has been verified.
  Verified = 3
};

using ProofSequenceIdx = uint32_t;

class Clause final {
public:
  using size_type = uint32_t;
  using iterator = CNFLit*;             // TODO: write proper iterator
  using const_iterator = CNFLit const*; // TODO: write proper const_iterator

  auto operator[](size_type idx) noexcept -> CNFLit&;
  auto operator[](size_type idx) const noexcept -> CNFLit const&;

  auto getLiterals() noexcept -> gsl::span<CNFLit>;
  auto getLiterals() const noexcept -> gsl::span<CNFLit const>;

  auto size() const noexcept -> size_type;
  auto empty() const noexcept -> bool;

  void setState(ClauseVerificationState state) noexcept;
  auto getState() const noexcept -> ClauseVerificationState;

  auto getAddIdx() const noexcept -> ProofSequenceIdx;
  auto getDelIdx() const noexcept -> ProofSequenceIdx;
  void setDelIdx(ProofSequenceIdx idx) noexcept;

private:
  friend class ClauseAllocator;
  Clause(size_type size, ClauseVerificationState initialState, ProofSequenceIdx addIdx) noexcept;

  size_type m_size;
  uint32_t m_flags;
  ProofSequenceIdx m_pointOfAdd;
  ProofSequenceIdx m_pointOfDel;
  CNFLit m_firstLit;
};

auto operator<<(std::ostream& stream, Clause const& clause) -> std::ostream&;

class BinaryClause final {
public:
  BinaryClause(CNFLit other, ClauseVerificationState initialState, ProofSequenceIdx addIdx);

  void setState(ClauseVerificationState state) noexcept;
  auto getState() const noexcept -> ClauseVerificationState;

  auto getOtherLit() noexcept -> CNFLit&;
  auto getOtherLit() const noexcept -> CNFLit const&;

  auto getAddIdx() const noexcept -> ProofSequenceIdx;
  auto getDelIdx() const noexcept -> ProofSequenceIdx;
  void setDelIdx(ProofSequenceIdx idx) noexcept;

private:
  uint32_t m_flags = 0;
  ProofSequenceIdx m_pointOfAdd;
  ProofSequenceIdx m_pointOfDel;
  CNFLit m_otherLit;
};

auto operator<<(std::ostream& stream, BinaryClause const& clause) -> std::ostream&;


class ClauseAllocator final {
public:
  ClauseAllocator();
  ~ClauseAllocator();

  class Ref {
  private:
    std::size_t m_offset;
    friend class ClauseAllocator;
  };

  auto allocate(gsl::span<CNFLit const> lits,
                ClauseVerificationState initialState,
                ProofSequenceIdx addIdx) -> Ref;
  auto resolve(Ref cref) noexcept -> Clause&;
  auto resolve(Ref cref) const noexcept -> Clause const&;

private:
  void resize(std::size_t newSize);

  char* m_memory = nullptr;
  std::size_t m_currentSize = 0;
  std::size_t m_highWaterMark = 0;
};

using CRef = ClauseAllocator::Ref;

// Implementation

inline auto Clause::operator[](size_type idx) noexcept -> CNFLit&
{
  return *((&m_firstLit) + idx);
}

inline auto Clause::operator[](size_type idx) const noexcept -> CNFLit const&
{
  return *((&m_firstLit) + idx);
}

inline auto Clause::getLiterals() noexcept -> gsl::span<CNFLit>
{
  return gsl::span<CNFLit>{&m_firstLit, (&m_firstLit) + m_size};
}

inline auto Clause::getLiterals() const noexcept -> gsl::span<CNFLit const>
{
  return gsl::span<CNFLit const>{&m_firstLit, (&m_firstLit) + m_size};
}

inline auto Clause::size() const noexcept -> size_type
{
  return m_size;
}

inline auto Clause::empty() const noexcept -> bool
{
  return m_size == 0;
}

inline void Clause::setState(ClauseVerificationState state) noexcept
{
  uint8_t rawState = static_cast<uint8_t>(state);
  m_flags = (m_flags & 0xFFFFFFFC) | rawState;
}

inline auto Clause::getState() const noexcept -> ClauseVerificationState
{
  return static_cast<ClauseVerificationState>(m_flags & 3);
}

inline auto Clause::getAddIdx() const noexcept -> ProofSequenceIdx
{
  return m_pointOfAdd;
}

inline auto Clause::getDelIdx() const noexcept -> ProofSequenceIdx
{
  return m_pointOfDel;
}

inline void Clause::setDelIdx(ProofSequenceIdx idx) noexcept
{
  m_pointOfDel = idx;
}

inline Clause::Clause(size_type size,
                      ClauseVerificationState initialState,
                      ProofSequenceIdx addIdx) noexcept
  : m_size{size}
  , m_pointOfAdd{addIdx}
  , m_pointOfDel{std::numeric_limits<ProofSequenceIdx>::max()}
  , m_firstLit{0}
{
  setState(initialState);
}

inline BinaryClause::BinaryClause(CNFLit other,
                                  ClauseVerificationState initialState,
                                  ProofSequenceIdx addIdx)
  : m_pointOfAdd{addIdx}
  , m_pointOfDel{std::numeric_limits<ProofSequenceIdx>::max()}
  , m_otherLit{other}
{
  setState(initialState);
}

inline void BinaryClause::setState(ClauseVerificationState state) noexcept
{
  uint8_t rawState = static_cast<uint8_t>(state);
  m_flags = (m_flags & 0xFFFFFF00) | rawState;
}

inline auto BinaryClause::getState() const noexcept -> ClauseVerificationState
{
  return static_cast<ClauseVerificationState>(m_flags & 3);
}

inline auto BinaryClause::getOtherLit() noexcept -> CNFLit&
{
  return m_otherLit;
}

inline auto BinaryClause::getOtherLit() const noexcept -> CNFLit const&
{
  return m_otherLit;
}

inline auto BinaryClause::getAddIdx() const noexcept -> ProofSequenceIdx
{
  return m_pointOfAdd;
}

inline auto BinaryClause::getDelIdx() const noexcept -> ProofSequenceIdx
{
  return m_pointOfDel;
}

inline void BinaryClause::setDelIdx(ProofSequenceIdx idx) noexcept
{
  m_pointOfDel = idx;
}
}