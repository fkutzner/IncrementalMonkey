/* Copyright (c) 2017,2018,2020 Felix Kutzner (github.com/fkutzner)

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

#include <vector>

namespace incmonk::verifier {

template <typename K,
          typename V,
          typename Key = incmonk::verifier::Key<K>,
          typename Allocator = std::allocator<V>>
class BoundedMap {
private:
  using BackingType = std::vector<V, Allocator>;

public:
  using size_type = typename BackingType::size_type;

  explicit BoundedMap(K maxKey);
  BoundedMap(K maxKey, V defaultValue);

  auto operator[](const K& index) noexcept -> V&;
  auto operator[](const K& index) const noexcept -> V const&;
  auto size() const noexcept -> size_type;
  void increaseSizeTo(K maxKey);

private:
  BackingType m_values;
  V m_defaultValue;
};


// Implementation

template <typename K, typename V, typename Key, typename Allocator>
BoundedMap<K, V, Key, Allocator>::BoundedMap(K maxKey)
  : m_values(Key::get(maxKey) + 1), m_defaultValue(V{})
{
}

template <typename K, typename V, typename Key, typename Allocator>
BoundedMap<K, V, Key, Allocator>::BoundedMap(K maxKey, V defaultValue)
  : m_values(Key::get(maxKey) + 1, defaultValue), m_defaultValue(defaultValue)
{
}

template <typename K, typename V, typename Key, typename Allocator>
auto BoundedMap<K, V, Key, Allocator>::operator[](const K& index) noexcept -> V&
{
  return m_values[Key::get(index)];
}

template <typename K, typename V, typename Key, typename Allocator>
auto BoundedMap<K, V, Key, Allocator>::operator[](const K& index) const noexcept -> V const&
{
  return m_values[Key::get(index)];
}

template <typename K, typename V, typename Key, typename Allocator>
auto BoundedMap<K, V, Key, Allocator>::size() const noexcept -> size_type
{
  return m_values.size();
}

template <typename K, typename V, typename Key, typename Allocator>
void BoundedMap<K, V, Key, Allocator>::increaseSizeTo(K maxKey)
{
  auto newMaxKey = Key::get(maxKey) + 1;
  if (newMaxKey >= m_values.size()) {
    m_values.resize(newMaxKey, m_defaultValue);
  }
}
}
