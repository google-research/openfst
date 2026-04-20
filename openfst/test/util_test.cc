// Copyright 2026 The OpenFst Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.
//
// Tests STL serialization.

#include "openfst/lib/util.h"

#include <array>
#include <cstdint>
#include <map>
#include <sstream>
#include <vector>

#include "gtest/gtest.h"

namespace fst {
namespace {

template <class T>
void WriteRead(const T& src, T* dst) {
  std::ostringstream out;
  WriteType(out, src);
  std::istringstream in(out.str());
  ReadType(in, dst);
}

TEST(WriteReadTest, Int) {
  int v = 0;
  WriteRead(1, &v);
  EXPECT_EQ(v, 1);
}

enum Enum { kZero, kOne };

TEST(WriteReadTest, Enum) {
  Enum v = kZero;
  WriteRead(kOne, &v);
  EXPECT_EQ(v, kOne);
}

enum class EnumClass { kZero, kOne };

TEST(WriteReadTest, EnumClass) {
  EnumClass v = EnumClass::kZero;
  WriteRead(EnumClass::kOne, &v);
  EXPECT_EQ(v, EnumClass::kOne);
}

TEST(WriteReadTest, MapIntVectorInt) {
  std::map<int, std::vector<int>> a, b;
  for (int i = 0; i < 32; ++i) {
    a[1 << i].push_back(i);
  }
  EXPECT_NE(a, b);
  WriteRead(a, &b);
  EXPECT_EQ(a, b);
}

TEST(WriteReadTest, VectorMapIntInt) {
  std::vector<std::map<int, int>> a, b;
  for (int i = 0; i < 32; ++i) {
    std::map<int, int> m;
    m[1 << i] = i;
    a.push_back(m);
  }
  EXPECT_NE(a, b);
  WriteRead(a, &b);
  EXPECT_EQ(a, b);
}

TEST(WriteReadTest, Array) {
  std::array<int32_t, 3> a{10, 20, 30}, b{0, 0, 0};
  EXPECT_NE(a, b);
  WriteRead(a, &b);
  EXPECT_EQ(a, b);
}

TEST(CompactSetTest, DefaultConstructor) {
  CompactSet<int, -1> values;
  EXPECT_TRUE(values.Begin() == values.End());
  EXPECT_EQ(values.LowerBound(), -1);
  EXPECT_EQ(values.UpperBound(), -1);
}

TEST(CompactSetTest, InsertAndMember) {
  CompactSet<int, -1> values;
  values.Insert(5);
  EXPECT_FALSE(values.Begin() == values.End());
  EXPECT_TRUE(values.Member(5));
  EXPECT_FALSE(values.Member(4));
  EXPECT_EQ(values.LowerBound(), 5);
  EXPECT_EQ(values.UpperBound(), 5);

  values.Insert(3);
  EXPECT_TRUE(values.Member(3));
  EXPECT_EQ(values.LowerBound(), 3);
  EXPECT_EQ(values.UpperBound(), 5);

  values.Insert(7);
  EXPECT_TRUE(values.Member(7));
  EXPECT_EQ(values.LowerBound(), 3);
  EXPECT_EQ(values.UpperBound(), 7);
}

TEST(CompactSetTest, Find) {
  CompactSet<int, -1> values;
  values.Insert(5);
  auto it = values.Find(5);
  EXPECT_NE(it, values.End());
  EXPECT_EQ(*it, 5);

  it = values.Find(4);
  EXPECT_EQ(it, values.End());
}

TEST(CompactSetTest, Erase) {
  CompactSet<int, -1> values;
  values.Insert(3);
  values.Insert(5);
  values.Insert(7);

  values.Erase(3);
  EXPECT_FALSE(values.Member(3));
  EXPECT_EQ(values.LowerBound(), 4);

  values.Erase(7);
  EXPECT_FALSE(values.Member(7));
  EXPECT_EQ(values.UpperBound(), 6);

  values.Erase(5);
  EXPECT_FALSE(values.Member(5));
  EXPECT_EQ(values.LowerBound(), -1);
  EXPECT_EQ(values.UpperBound(), -1);
}

TEST(CompactSetTest, Clear) {
  CompactSet<int, -1> values;
  values.Insert(3);
  values.Insert(5);
  values.Clear();
  EXPECT_TRUE(values.Begin() == values.End());
  EXPECT_EQ(values.LowerBound(), -1);
  EXPECT_EQ(values.UpperBound(), -1);
}

TEST(CompactSetTest, DenseRange) {
  CompactSet<int, -1> values;
  values.Insert(3);
  values.Insert(4);
  values.Insert(5);
  EXPECT_TRUE(values.Member(3));
  EXPECT_TRUE(values.Member(4));
  EXPECT_TRUE(values.Member(5));
  EXPECT_FALSE(values.Member(2));
  EXPECT_FALSE(values.Member(6));
}

}  // namespace
}  // namespace fst
