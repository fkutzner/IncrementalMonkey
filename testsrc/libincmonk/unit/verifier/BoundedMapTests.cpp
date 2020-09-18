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

#include <libincmonk/verifier/BoundedMap.h>

#include <libincmonk/verifier/Traits.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace incmonk::verifier {
struct IntBox {
  int i = 0;
};

template <>
struct Key<IntBox> {
  static auto get(IntBox const& box) -> std::size_t
  {
    if (box.i == 0) {
      return 0;
    }
    else {
      return 2 * static_cast<std::size_t>(std::abs(box.i)) + (box.i > 0 ? 1 : 0) - 1;
    }
  }
};

TEST(BoundedMapTests, reservesEnoughSpaceForSmallestVar)
{
  BoundedMap<IntBox, std::string> underTest{IntBox{0}};
  ASSERT_THAT(underTest, ::testing::SizeIs(1));
  underTest[IntBox{0}] = "foo";
  ASSERT_THAT(underTest[IntBox{0}], ::testing::Eq("foo"));
}

TEST(BoundedMapTests, reservesEnoughSpaceForVarWithIndex2)
{
  BoundedMap<IntBox, std::string> underTest{IntBox{1}};
  ASSERT_THAT(underTest, ::testing::SizeIs(3));

  underTest[IntBox{3}] = "foo";
  ASSERT_THAT(underTest[IntBox{3}], ::testing::Eq("foo"));
}

TEST(BoundedMapTests, reservesEnoughSpaceDuringResize)
{
  BoundedMap<IntBox, std::string> underTest{IntBox{0}};
  underTest.increaseSizeTo(IntBox{5});
  ASSERT_THAT(underTest, ::testing::SizeIs(11));

  underTest[IntBox{5}] = "foo";
  ASSERT_THAT(underTest[IntBox{5}], ::testing::Eq("foo"));
}


}