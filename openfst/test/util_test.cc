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
#include <ios>
#include <istream>
#include <list>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "gtest/gtest.h"
#include "absl/container/flat_hash_map.h"
#include "absl/flags/flag.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"

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

TEST(WriteReadTest, List) {
  std::list<int> a{10, 20, 30}, b;
  EXPECT_NE(a, b);
  WriteRead(a, &b);
  EXPECT_EQ(a, b);
}

TEST(WriteReadTest, Set) {
  std::set<int> a{10, 20, 30}, b;
  EXPECT_NE(a, b);
  WriteRead(a, &b);
  EXPECT_EQ(a, b);
}

TEST(WriteReadTest, UnorderedSet) {
  std::unordered_set<int> a{10, 20, 30}, b;
  EXPECT_NE(a, b);
  WriteRead(a, &b);
  EXPECT_EQ(a, b);
}

TEST(WriteReadTest, UnorderedMap) {
  std::unordered_map<int, int> a{{1, 10}, {2, 20}}, b;
  EXPECT_NE(a, b);
  WriteRead(a, &b);
  EXPECT_EQ(a, b);
}

TEST(WriteReadTest, FlatHashMap) {
  absl::flat_hash_map<int, int> a{{1, 10}, {2, 20}}, b;
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

TEST(SpanInStreamTest, EmptyStream) {
  SpanInStream in(absl::string_view(""));
  EXPECT_EQ(in.tellg(), 0);
  EXPECT_EQ(in.get(), std::char_traits<char>::eof());
  EXPECT_TRUE(in.eof());
  EXPECT_TRUE(in.fail());
  EXPECT_EQ(in.rdbuf()->in_avail(), 0);
}

TEST(SpanInStreamTest, FormattedReading) {
  SpanInStream in("hello 42 3.14");
  std::string word;
  int integer_val = 0;
  double float_val = 0.0;
  in >> word >> integer_val >> float_val;
  EXPECT_EQ(word, "hello");
  EXPECT_EQ(integer_val, 42);
  EXPECT_DOUBLE_EQ(float_val, 3.14);
}

TEST(SpanInStreamTest, BulkRead) {
  const std::string data = "abcdefghijklmnopqrstuvwxyz";
  SpanInStream in(data);
  char buf[11] = {};

  in.read(buf, 10);
  EXPECT_EQ(in.gcount(), 10);
  buf[10] = '\0';
  EXPECT_STREQ(buf, "abcdefghij");

  in.read(buf, 10);
  EXPECT_EQ(in.gcount(), 10);
  buf[10] = '\0';
  EXPECT_STREQ(buf, "klmnopqrst");

  in.read(buf, 10);
  EXPECT_EQ(in.gcount(), 6);
  buf[6] = '\0';
  EXPECT_STREQ(buf, "uvwxyz");
  EXPECT_TRUE(in.eof());
  EXPECT_TRUE(in.fail());
}

TEST(SpanInStreamTest, Seeking) {
  SpanInStream in("0123456789");
  EXPECT_EQ(in.tellg(), 0);

  in.seekg(5, std::ios_base::beg);
  EXPECT_EQ(in.tellg(), 5);
  EXPECT_EQ(in.get(), '5');

  in.seekg(2, std::ios_base::cur);
  EXPECT_EQ(in.tellg(), 8);
  EXPECT_EQ(in.get(), '8');

  in.seekg(-3, std::ios_base::end);
  EXPECT_EQ(in.tellg(), 7);
  EXPECT_EQ(in.get(), '7');

  // Seek out of bounds sets failbit.
  in.seekg(-1, std::ios_base::beg);
  EXPECT_TRUE(in.fail());
  in.clear();

  in.seekg(20, std::ios_base::beg);
  EXPECT_TRUE(in.fail());
}

TEST(SpanInStreamTest, UngetAndPutback) {
  SpanInStream in("abc");
  EXPECT_EQ(in.get(), 'a');
  in.unget();
  EXPECT_EQ(in.get(), 'a');
  EXPECT_EQ(in.get(), 'b');
  in.putback('b');
  EXPECT_EQ(in.get(), 'b');
  EXPECT_EQ(in.get(), 'c');
  EXPECT_EQ(in.get(), std::char_traits<char>::eof());
}

TEST(SpanInStreamTest, SpanCharAndUint8) {
  const std::vector<char> chars = {'t', 'e', 's', 't'};
  SpanInStream in_chars(absl::MakeConstSpan(chars));
  std::string s;
  in_chars >> s;
  EXPECT_EQ(s, "test");

  const std::vector<uint8_t> bytes = {'b', 'y', 't', 'e', 's'};
  SpanInStream in_bytes(absl::MakeConstSpan(bytes));
  std::string b;
  in_bytes >> b;
  EXPECT_EQ(b, "bytes");
}

TEST(SpanInStreamTest, OpenFstReadTypeIntegration) {
  std::ostringstream out;
  const int32_t orig_val = 12345;
  const std::string orig_str = "openfst_zero_copy";
  WriteType(out, orig_val);
  WriteType(out, orig_str);

  const std::string serialized = out.str();
  SpanInStream in(serialized);
  int32_t read_val = 0;
  std::string read_str;
  ReadType(in, &read_val);
  ReadType(in, &read_str);

  EXPECT_EQ(read_val, orig_val);
  EXPECT_EQ(read_str, orig_str);
}

struct DummyWeight {
  float value = 0.0f;

  static DummyWeight NoWeight() { return DummyWeight{-1.0f}; }

  friend std::istream& operator>>(std::istream& is, DummyWeight& w) {
    return is >> w.value;
  }
};

TEST(StrToWeightTest, ValidWeight) {
  DummyWeight w = StrToWeight<DummyWeight>("1.5");
  EXPECT_FLOAT_EQ(w.value, 1.5f);
}

TEST(StrToWeightTest, NegativeWeight) {
  DummyWeight w = StrToWeight<DummyWeight>("-2.25");
  EXPECT_FLOAT_EQ(w.value, -2.25f);
}

TEST(StrToWeightTest, InvalidWeight) {
  const bool old_fst_error_fatal = absl::GetFlag(FLAGS_fst_error_fatal);
  absl::SetFlag(&FLAGS_fst_error_fatal, false);
  DummyWeight w = StrToWeight<DummyWeight>("not_a_number");
  EXPECT_FLOAT_EQ(w.value, -1.0f);
  absl::SetFlag(&FLAGS_fst_error_fatal, old_fst_error_fatal);
}

}  // namespace
}  // namespace fst
