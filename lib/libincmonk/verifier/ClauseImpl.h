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

#include <libincmonk/verifier/Clause.h>

#include <cassert>
#include <cstdint>
#include <gsl/span>
#include <iterator>
#include <limits>
#include <optional>
#include <ostream>


//
// Implementation of inline function declared in Clause.h
//

namespace incmonk::verifier {

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

constexpr auto Key<Var>::get(Var const& item) -> std::size_t
{
  return item.getRawValue();
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

constexpr auto Key<Lit>::get(Lit const& item) -> std::size_t
{
  return item.getRawValue();
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


inline auto ClauseCollection::Ref::operator==(Ref rhs) const noexcept -> bool
{
  return m_offset == rhs.m_offset;
}

inline auto ClauseCollection::Ref::operator!=(Ref rhs) const noexcept -> bool
{
  return !(*this == rhs);
}

inline ClauseCollection::RefIterator::RefIterator(char const* m_allocatorMemory,
                                                  std::size_t highWaterMark) noexcept
  : m_clausePtr{m_allocatorMemory}, m_distanceToEnd{highWaterMark}, m_currentRef{}
{
  if (m_distanceToEnd == 0) {
    m_clausePtr = nullptr;
  }
  // Otherwise, the iterator is valid and currentRef is 0, referring to the first clause
}

inline ClauseCollection::RefIterator::RefIterator() noexcept
  : m_clausePtr{nullptr}, m_distanceToEnd{0}, m_currentRef{}
{
}

inline auto ClauseCollection::RefIterator::operator*() const noexcept -> Ref const&
{
  assert(m_clausePtr != nullptr && "Dereferencing a non-dereferencaable RefIterator");
  return m_currentRef;
}

inline auto ClauseCollection::RefIterator::operator->() const noexcept -> Ref const*
{
  assert(m_clausePtr != nullptr && "Dereferencing a non-dereferencaable RefIterator");
  return &m_currentRef;
}

inline auto ClauseCollection::RefIterator::operator++(int) noexcept -> RefIterator
{
  RefIterator old = *this;
  ++(*this);
  return old;
}

inline auto ClauseCollection::RefIterator::operator==(RefIterator const& rhs) const noexcept -> bool
{
  return (this == &rhs) || (m_clausePtr == nullptr && rhs.m_clausePtr == nullptr) ||
         (m_clausePtr == rhs.m_clausePtr && m_distanceToEnd == rhs.m_distanceToEnd &&
          m_currentRef == rhs.m_currentRef);
}

inline auto ClauseCollection::RefIterator::operator!=(RefIterator const& rhs) const noexcept -> bool
{
  return !(*this == rhs);
}

}