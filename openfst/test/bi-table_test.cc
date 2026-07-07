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
// Unit tests for bidirectional mapping tables (BiTable).

#include "openfst/lib/bi-table.h"

#include <functional>
#include <string>

#include "gtest/gtest.h"
#include "absl/hash/hash.h"

namespace fst {
namespace {

TEST(BiTableTest, HashBiTableBasic) {
  HashBiTable<int, std::string> table;
  EXPECT_EQ(table.Size(), 0);

  // Find without insert should return -1 when missing.
  EXPECT_EQ(table.FindId("foo", false), -1);

  // Find with insert.
  const int id_foo = table.FindId("foo", true);
  EXPECT_EQ(id_foo, 0);
  EXPECT_EQ(table.Size(), 1);
  EXPECT_EQ(table.FindEntry(id_foo), "foo");

  const int id_bar = table.FindId("bar", true);
  EXPECT_EQ(id_bar, 1);
  EXPECT_EQ(table.Size(), 2);
  EXPECT_EQ(table.FindEntry(id_bar), "bar");

  // Re-finding existing entry without insert or with insert gives same ID.
  EXPECT_EQ(table.FindId("foo", false), id_foo);
  EXPECT_EQ(table.FindId("foo", true), id_foo);
  EXPECT_EQ(table.Size(), 2);

  // Copy constructor test.
  HashBiTable<int, std::string> copy(table);
  EXPECT_EQ(copy.Size(), 2);
  EXPECT_EQ(copy.FindId("foo", false), id_foo);
  EXPECT_EQ(copy.FindId("bar", false), id_bar);
  EXPECT_EQ(copy.FindEntry(id_foo), "foo");

  // Clear test.
  table.Clear();
  EXPECT_EQ(table.Size(), 0);
  EXPECT_EQ(table.FindId("foo", false), -1);
}

template <HSType hs_type>
void TestCompactHashBiTable() {
  using Table = CompactHashBiTable<int, std::string, absl::Hash<std::string>,
                                   std::equal_to<std::string>, hs_type>;
  Table table;
  EXPECT_EQ(table.Size(), 0);
  EXPECT_EQ(table.FindId("foo", false), Table::kNoId);

  const int id_foo = table.FindId("foo", true);
  EXPECT_EQ(id_foo, 0);
  EXPECT_EQ(table.Size(), 1);
  EXPECT_EQ(table.FindEntry(id_foo), "foo");

  const int id_bar = table.FindId("bar", true);
  EXPECT_EQ(id_bar, 1);
  EXPECT_EQ(table.Size(), 2);
  EXPECT_EQ(table.FindEntry(id_bar), "bar");

  EXPECT_EQ(table.FindId("foo", false), id_foo);
  EXPECT_EQ(table.FindId("foo", true), id_foo);
  EXPECT_EQ(table.Size(), 2);

  Table copy(table);
  EXPECT_EQ(copy.Size(), 2);
  EXPECT_EQ(copy.FindId("foo", false), id_foo);
  EXPECT_EQ(copy.FindId("bar", false), id_bar);

  table.Clear();
  EXPECT_EQ(table.Size(), 0);
  EXPECT_EQ(table.FindId("foo", false), Table::kNoId);
}

TEST(BiTableTest, CompactHashBiTableFlat) { TestCompactHashBiTable<HS_FLAT>(); }

TEST(BiTableTest, CompactHashBiTableStl) { TestCompactHashBiTable<HS_STL>(); }

struct IntFingerprint {
  ssize_t operator()(int x) const { return x; }
};

TEST(BiTableTest, VectorBiTableBasic) {
  VectorBiTable<int, int, IntFingerprint> table;
  EXPECT_EQ(table.Size(), 0);

  EXPECT_EQ(table.FindId(10, false), -1);

  const int id_10 = table.FindId(10, true);
  EXPECT_EQ(id_10, 0);
  EXPECT_EQ(table.Size(), 1);
  EXPECT_EQ(table.FindEntry(id_10), 10);

  const int id_5 = table.FindId(5, true);
  EXPECT_EQ(id_5, 1);
  EXPECT_EQ(table.Size(), 2);
  EXPECT_EQ(table.FindEntry(id_5), 5);

  EXPECT_EQ(table.FindId(10, false), id_10);
  EXPECT_EQ(table.FindId(10, true), id_10);
  EXPECT_EQ(table.Size(), 2);

  VectorBiTable<int, int, IntFingerprint> copy(table);
  EXPECT_EQ(copy.Size(), 2);
  EXPECT_EQ(copy.FindId(10, false), id_10);
  EXPECT_EQ(copy.FindEntry(id_5), 5);
}

struct SmallIntSelector {
  bool operator()(int x) const { return x >= 0 && x < 100; }
};

TEST(BiTableTest, VectorHashBiTableBasic) {
  using Table = VectorHashBiTable<int, int, SmallIntSelector, IntFingerprint>;
  Table table;
  EXPECT_EQ(table.Size(), 0);

  // 42 is routed to the vector storage path.
  EXPECT_EQ(table.FindId(42, false), Table::kNoId);
  const int id_42 = table.FindId(42, true);
  EXPECT_EQ(id_42, 0);
  EXPECT_EQ(table.FindEntry(id_42), 42);

  // 200 is routed to the compact hash storage path.
  EXPECT_EQ(table.FindId(200, false), Table::kNoId);
  const int id_200 = table.FindId(200, true);
  EXPECT_EQ(id_200, 1);
  EXPECT_EQ(table.FindEntry(id_200), 200);

  EXPECT_EQ(table.Size(), 2);
  EXPECT_EQ(table.FindId(42, false), id_42);
  EXPECT_EQ(table.FindId(200, false), id_200);

  Table copy(table);
  EXPECT_EQ(copy.Size(), 2);
  EXPECT_EQ(copy.FindId(42, false), id_42);
  EXPECT_EQ(copy.FindId(200, false), id_200);
}

TEST(BiTableTest, ErasableBiTableBasic) {
  ErasableBiTable<int, std::string, absl::Hash<std::string>> table;
  EXPECT_EQ(table.Size(), 0);

  EXPECT_EQ(table.FindId("foo", false), -1);

  const int id_foo = table.FindId("foo", true);
  EXPECT_EQ(id_foo, 0);
  EXPECT_EQ(table.Size(), 1);
  EXPECT_EQ(table.FindEntry(id_foo), "foo");

  const int id_bar = table.FindId("bar", true);
  EXPECT_EQ(id_bar, 1);
  EXPECT_EQ(table.Size(), 2);
  EXPECT_EQ(table.FindEntry(id_bar), "bar");

  // Erasing back element (id 1). Deque front is still "foo", so size remains 2
  // but "bar" lookup returns -1.
  table.Erase(1);
  EXPECT_EQ(table.Size(), 2);
  EXPECT_EQ(table.FindId("bar", false), -1);
  EXPECT_EQ(table.FindId("foo", false), 0);

  // Now erase front element (id 0). Both front entries are now empty, so deque
  // pops both and becomes empty.
  table.Erase(0);
  EXPECT_EQ(table.Size(), 0);
  EXPECT_EQ(table.FindId("foo", false), -1);
}

}  // namespace
}  // namespace fst
