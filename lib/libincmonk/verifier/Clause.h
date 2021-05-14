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

#include <libincmonk/verifier/Traits.h>

#include <cstdint>
#include <gsl/span>
#include <iterator>
#include <limits>
#include <memory>
#include <optional>
#include <ostream>
#include <unordered_map>
#include <vector>

namespace incmonk::verifier {

class Var {
public:
  constexpr explicit Var(uint32_t id);
  constexpr Var();

  constexpr auto getRawValue() const -> uint32_t;

  constexpr auto operator==(Var rhs) const -> bool;
  constexpr auto operator!=(Var rhs) const -> bool;
  constexpr auto operator<(Var rhs) const -> bool;
  constexpr auto operator<=(Var rhs) const -> bool;
  constexpr auto operator>(Var rhs) const -> bool;
  constexpr auto operator>=(Var rhs) const -> bool;

private:
  uint32_t m_rawValue;
};

template <>
struct Key<Var> {
  constexpr static auto get(Var const& item) -> std::size_t;
};

auto operator""_Var(unsigned long long cnfValue) -> Var;
auto operator<<(std::ostream& stream, Var var) -> std::ostream&;


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
  constexpr auto operator<(Lit rhs) const -> bool;
  constexpr auto operator<=(Lit rhs) const -> bool;
  constexpr auto operator>(Lit rhs) const -> bool;
  constexpr auto operator>=(Lit rhs) const -> bool;

private:
  uint32_t m_rawValue;
};

auto operator""_Lit(unsigned long long cnfValue) -> Lit;
auto operator<<(std::ostream& stream, Lit lit) -> std::ostream&;

template <>
struct Key<Lit> {
  constexpr static auto get(Lit const& item) -> std::size_t;
};

auto maxLit(Var var) noexcept -> Lit;


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

private:
  friend class ClauseCollection;
  Clause(size_type size, ClauseVerificationState initialState, ProofSequenceIdx addIdx) noexcept;

  size_type m_size;
  uint32_t m_flags;
  ProofSequenceIdx m_pointOfAdd;
  Lit m_firstLit;
};

auto operator<<(std::ostream& stream, Clause const& clause) -> std::ostream&;

class ClauseFinder;
class ClauseOccurrences;

class ClauseCollection final {
public:
  ClauseCollection();
  ~ClauseCollection();

  class Ref {
  public:
    auto operator==(Ref rhs) const noexcept -> bool;
    auto operator!=(Ref rhs) const noexcept -> bool;

  private:
    std::size_t m_offset = 0;
    friend class ClauseCollection;
    friend class RefIterator;
    friend struct std::hash<Ref>;
  };

  class RefIterator {
  public:
    using value_type = Ref;
    using reference = Ref&;
    using pointer = Ref*;
    using difference_type = intptr_t;
    using iterator_category = std::input_iterator_tag;

    RefIterator(char const* m_allocatorMemory, std::size_t highWaterMark) noexcept;
    RefIterator() noexcept;

    auto operator*() const noexcept -> Ref const&;
    auto operator->() const noexcept -> Ref const*;
    auto operator++(int) noexcept -> RefIterator;
    auto operator++() noexcept -> RefIterator&;

    auto operator==(RefIterator const& rhs) const noexcept -> bool;
    auto operator!=(RefIterator const& rhs) const noexcept -> bool;

    RefIterator(RefIterator const& rhs) noexcept = default;
    RefIterator(RefIterator&& rhs) noexcept = default;
    auto operator=(RefIterator const& rhs) noexcept -> RefIterator& = default;
    auto operator=(RefIterator&& rhs) noexcept -> RefIterator& = default;

  private:
    char const* m_clausePtr;
    std::size_t m_distanceToEnd;
    Ref m_currentRef;
  };

  using LitSpan = gsl::span<Lit const>;
  using OccRng = gsl::span<Ref const>;


  auto add(LitSpan lits, ClauseVerificationState initialState, ProofSequenceIdx addIdx) -> Ref;
  auto resolve(Ref cref) noexcept -> Clause&;
  auto resolve(Ref cref) const noexcept -> Clause const&;
  auto find(LitSpan lits) const noexcept -> std::optional<Ref>;
  auto getOccurrences(Lit lit) const noexcept -> OccRng;

  auto begin() const noexcept -> RefIterator;
  auto end() const noexcept -> RefIterator;

  auto getMaxVar() const noexcept -> Var;


  ClauseCollection(ClauseCollection&&) noexcept;
  auto operator=(ClauseCollection &&) -> ClauseCollection&;

  ClauseCollection(ClauseCollection const&) = delete;
  auto operator=(ClauseCollection const&) -> ClauseCollection& = delete;

private:
  void resize(std::size_t newSize);
  auto isValidRef(Ref cref) const noexcept -> bool;

  char* m_memory = nullptr;
  std::size_t m_currentSize = 0;
  std::size_t m_highWaterMark = 0;

  Var m_maxVar = 0_Var;

  std::vector<Ref> m_deletedClauses;
  mutable std::unique_ptr<ClauseFinder> m_clauseFinder;
  mutable std::unique_ptr<ClauseOccurrences> m_clauseOccurrences;
};

using CRef = ClauseCollection::Ref;
using OptCRef = std::optional<CRef>;
}

#include "ClauseImpl.h"
