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
// Unit test for the Union-Find algorithm.
//
// The test generates random set unions and then checks if the result is
// identical to the result of explicitly unioning the sets.

#include "openfst/lib/union-find.h"

#include <cstdint>
#include <random>
#include <set>
#include <vector>

#include "gtest/gtest.h"
#include "openfst/compat/seed_sequences.h"
#include "absl/random/random.h"
#include "benchmark/benchmark.h"

constexpr int kNumElements = 37;  // # of elements in the test set.
constexpr int kMaxUnions = 37;    // Max # of Union operations.
constexpr int kNumTestRuns = 10;  // Number of separate random test runs.

namespace {
int Rand(int n, absl::BitGen& bit_gen) { return absl::Uniform(bit_gen, 1, n); }
}  // namespace

class UnionFindTest : public testing::Test {
 protected:
  using UnionFind = fst::UnionFind<int>;

  // Single test run: generates a random partition of a set of n
  // elements by performing k random union operations.
  void TestRun(int n, int k) {
    UnionFind uf(2, -1);                  // The union-find disjoint set
                                          // forest.
    std::vector<std::set<int>> sets0(n);  // Original sets.
    std::vector<std::set<int>*> sets(n);  // Pointers to elements of sets0.

    // Initialization.
    for (int i = 0; i < n; ++i) {
      sets[i] = &sets0[i];
      sets[i]->insert(i);
      uf.MakeSet(i);
    }

    // Main loop: performs random union operations.
    for (int i = 0; i < k; ++i) {
      // Select two random elements.
      int p = Rand(n, bit_gen_);
      int q = Rand(n, bit_gen_);

      // Union-Find operation.
      uf.Union(p, q);

      std::set<int>* setsq = sets[q];
      // Insert contents of sets[q] into sets[p] and make
      // sets[i] point to sets[p] for each i in sets[q].
      for (std::set<int>::const_iterator it = setsq->begin();
           it != setsq->end(); ++it) {
        sets[p]->insert(*it);
        sets[*it] = sets[p];
      }
    }

    // Convert disjoint set forest into an explicit set representation
    // (uf_sets[i] <- the set 'i' belongs to).
    std::vector<std::set<int>> uf_sets0(n);
    std::vector<std::set<int>*> uf_sets(n);

    for (int i = 0; i < n; ++i) {
      int r = uf.FindSet(i);
      uf_sets[i] = &uf_sets0[r];
      uf_sets[i]->insert(i);
    }

    // Check that the structures produced by Union-Find are identical
    // to the explicitly constructed sets.
    for (int i = 0; i < n; ++i) {
      ASSERT_TRUE(*sets[i] == *uf_sets[i]);
    }
  }

  absl::BitGen bit_gen_{fst::MakeTaggedSeedSeq(
      "UnionFindTest")};
};

TEST_F(UnionFindTest, T1) {
  for (int i = 0; i < kNumTestRuns; ++i) {
    TestRun(kNumElements, Rand(kMaxUnions, bit_gen_));
  }
}

// Test if the sets are all distinct
TEST_F(UnionFindTest, MakeAllSet) {
  UnionFind uf(0, -1);  // The union-find disjoint set
  uf.MakeAllSet(kNumElements);
  // The loop is quadratic, but for a small kNumElements (37), that shd be fine
  for (int i = 0; i < kNumElements; ++i) {
    for (int j = i + 1; j < kNumElements; ++j) {
      ASSERT_TRUE(uf.FindSet(i) != uf.FindSet(j));
    }
  }
}

TEST(UnionFindPathTest, FindSetCorrectlyUpdatesRootForAllElementsInAPath) {
  fst::UnionFind<int> forest(0, -1);  // The union-find disjoint set

  for (int i = 0; i < 16; i++) {
    forest.MakeSet(i);
  }

  {
    forest.Union(0, 1);
    forest.Union(2, 3);
    forest.Union(1, 3);

    forest.Union(6, 4);
    forest.Union(7, 5);
    forest.Union(5, 4);

    forest.Union(4, 1);
  }

  {
    forest.Union(9, 8);
    forest.Union(11, 10);
    forest.Union(10, 8);

    forest.Union(14, 12);
    forest.Union(15, 13);
    forest.Union(13, 12);

    forest.Union(12, 8);
  }

  forest.Union(8, 1);
  EXPECT_EQ(forest.Parent(15), 13);
  EXPECT_EQ(forest.Parent(13), 12);
  EXPECT_EQ(forest.Parent(12), 8);
  EXPECT_EQ(forest.Parent(8), 3);
  EXPECT_EQ(forest.FindSet(0), forest.FindSet(15));
  EXPECT_EQ(forest.Parent(15), 3);
  EXPECT_EQ(forest.Parent(13), 3);
  EXPECT_EQ(forest.Parent(12), 3);
  EXPECT_EQ(forest.Parent(8), 3);
}

namespace {

static void BM_UnionFind(benchmark::State& state) {
  const int elements = state.range(0);

  fst::UnionFind<int> uf(elements, -1);
  absl::BitGen bit_gen(fst::MakeTaggedSeedSeq(
      "BM_UNION_FIND"));

  // Multiply by 10 to run for longer than the benchmark usually wants
  // us to. We get slightly better numerical stability that way.
  for (auto s : state) {
    int a = Rand(elements, bit_gen);
    int b = Rand(elements, bit_gen);
    if (uf.FindSet(a) == -1) uf.MakeSet(a);
    if (uf.FindSet(b) == -1) uf.MakeSet(b);
    uf.Union(a, b);
  }
  state.SetItemsProcessed(10 * state.iterations());
}
BENCHMARK(BM_UnionFind)->Range(8, 1 << 16);

}  // namespace
