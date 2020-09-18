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
#include <gsl/span>
#include <limits>
#include <optional>
#include <ostream>

namespace incmonk::verifier {

class Var {
public:
  constexpr explicit Var(uint32_t id);
  constexpr Var();

  constexpr auto getRawValue() const -> uint32_t;

  constexpr auto operator==(Var rhs) const -> bool;
  constexpr auto operator!=(Var rhs) const -> bool;

private:
  uint32_t m_rawValue;
};


class Lit {
public:
  constexpr explicit Lit(Var v, bool positive);
  constexpr Lit();

  constexpr auto getRawValue() const -> uint32_t;
  constexpr auto getVar() const -> Var;
  constexpr auto isPositive() const -> bool;
  constexpr auto operator-() const -> Lit;

  constexpr auto operator==(Lit rhs) const -> bool;
  constexpr auto operator!=(Lit rhs) const -> bool;

private:
  uint32_t m_rawValue;
};

auto operator""_Lit(unsigned long long cnfValue) -> Lit;
auto operator<<(std::ostream& stream, Lit lit) -> std::ostream&;
auto operator<<(std::ostream& stream, Var var) -> std::ostream&;


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
  using iterator = Lit*;             // TODO: write proper iterator
  using const_iterator = Lit const*; // TODO: write proper const_iterator

  auto operator[](size_type idx) noexcept -> Lit&;
  auto operator[](size_type idx) const noexcept -> Lit const&;

  auto getLiterals() noexcept -> gsl::span<Lit>;
  auto getLiterals() const noexcept -> gsl::span<Lit const>;

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
  Lit m_firstLit;
};

auto operator<<(std::ostream& stream, Clause const& clause) -> std::ostream&;

class BinaryClause final {
public:
  BinaryClause(Lit other, ClauseVerificationState initialState, ProofSequenceIdx addIdx);

  void setState(ClauseVerificationState state) noexcept;
  auto getState() const noexcept -> ClauseVerificationState;

  auto getOtherLit() noexcept -> Lit&;
  auto getOtherLit() const noexcept -> Lit const&;

  auto getAddIdx() const noexcept -> ProofSequenceIdx;
  auto getDelIdx() const noexcept -> ProofSequenceIdx;
  void setDelIdx(ProofSequenceIdx idx) noexcept;

private:
  uint32_t m_flags = 0;
  ProofSequenceIdx m_pointOfAdd;
  ProofSequenceIdx m_pointOfDel;
  Lit m_otherLit;
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

  auto allocate(gsl::span<Lit const> lits,
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

constexpr Var::Var(uint32_t id) : m_rawValue(id) {}

constexpr Var::Var() : m_rawValue{0} {}

constexpr auto Var::getRawValue() const -> uint32_t
{
  return m_rawValue;
}

constexpr auto Var::operator==(Var rhs) const -> bool
{
  return m_rawValue == rhs.m_rawValue;
}

constexpr auto Var::operator!=(Var rhs) const -> bool
{
  return m_rawValue != rhs.m_rawValue;
}

constexpr Lit::Lit(Var v, bool positive) : m_rawValue{(v.getRawValue() << 1) + (positive ? 1 : 0)}
{
}

constexpr Lit::Lit() : m_rawValue{0} {}

constexpr auto Lit::getRawValue() const -> uint32_t
{
  return m_rawValue;
}

constexpr auto Lit::getVar() const -> Var
{
  return Var{m_rawValue >> 1};
}

constexpr auto Lit::isPositive() const -> bool
{
  return (m_rawValue & 1) == 1;
}

constexpr auto Lit::operator-() const -> Lit
{
  return Lit{getVar(), !isPositive()};
}

constexpr auto Lit::operator==(Lit rhs) const -> bool
{
  return m_rawValue == rhs.m_rawValue;
}

constexpr auto Lit::operator!=(Lit rhs) const -> bool
{
  return m_rawValue != rhs.m_rawValue;
}

inline auto operator""_Lit(unsigned long long cnfValue) -> Lit
{
  Var var{static_cast<uint32_t>(cnfValue < 0 ? -cnfValue : cnfValue)};
  return Lit{var, cnfValue > 0};
}


inline auto Clause::operator[](size_type idx) noexcept -> Lit&
{
  return *((&m_firstLit) + idx);
}

inline auto Clause::operator[](size_type idx) const noexcept -> Lit const&
{
  return *((&m_firstLit) + idx);
}

inline auto Clause::getLiterals() noexcept -> gsl::span<Lit>
{
  return gsl::span<Lit>{&m_firstLit, (&m_firstLit) + m_size};
}

inline auto Clause::getLiterals() const noexcept -> gsl::span<Lit const>
{
  return gsl::span<Lit const>{&m_firstLit, (&m_firstLit) + m_size};
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
  , m_firstLit{Var{0}, false}
{
  setState(initialState);
}

inline BinaryClause::BinaryClause(Lit other,
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

inline auto BinaryClause::getOtherLit() noexcept -> Lit&
{
  return m_otherLit;
}

inline auto BinaryClause::getOtherLit() const noexcept -> Lit const&
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