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

#include "openfst/lib/arc-arena.h"

#include <memory>
#include <vector>

#include "gtest/gtest.h"
#include "benchmark/benchmark.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/expander-fst.h"
#include "openfst/lib/symbol-table.h"

namespace fst {
namespace {

void PushTestArcs(int n, ArcArena<StdArc>* arena) {
  for (int i = 0; i < n; ++i) {
    arena->PushArc({i, i, StdArc::Weight::Zero(), i});
  }
}

TEST(ArcArenaTest, SingleBlock) {
  ArcArena<StdArc> arena;
  arena.ReserveArcs(10);
  PushTestArcs(23, &arena);
  const StdArc* arcs1 = arena.GetArcs();
  arena.ReserveArcs(10);
  PushTestArcs(5, &arena);
  const StdArc* arcs2 = arena.GetArcs();
  arena.ReserveArcs(10);
  PushTestArcs(3, &arena);
  const StdArc* arcs3 = arena.GetArcs();
  EXPECT_EQ(arcs2, arcs1 + 23);
  EXPECT_EQ(arcs3, arcs2 + 5);
}

TEST(ArcArenaTest, BigBlocks) {
  ArcArena<StdArc> arena;
  PushTestArcs(700, &arena);
  const StdArc* arcs1 = arena.GetArcs();
  PushTestArcs(500, &arena);
  const StdArc* arcs2 = arena.GetArcs();
  PushTestArcs(300, &arena);
  const StdArc* arcs3 = arena.GetArcs();
  for (int i = 0; i < 700; ++i) {
    EXPECT_EQ(i, arcs1[i].ilabel);
  }
  for (int i = 0; i < 500; ++i) {
    EXPECT_EQ(i, arcs2[i].ilabel);
  }
  for (int i = 0; i < 300; ++i) {
    EXPECT_EQ(i, arcs3[i].ilabel);
  }
}

TEST(ArcArenaTest, BigBlocksReuse) {
  ArcArena<StdArc> arena;
  for (int i = 0; i < 3; ++i) {
    arena.ReserveArcs(700);
    PushTestArcs(700, &arena);
    const StdArc* arcs1 = arena.GetArcs();
    arena.ReserveArcs(500);
    PushTestArcs(500, &arena);
    const StdArc* arcs2 = arena.GetArcs();
    arena.ReserveArcs(300);
    PushTestArcs(300, &arena);
    const StdArc* arcs3 = arena.GetArcs();
    for (int j = 0; j < 700; ++j) {
      EXPECT_EQ(j, arcs1[j].ilabel);
    }
    for (int j = 0; i < 500; ++i) {
      EXPECT_EQ(j, arcs2[j].ilabel);
    }
    for (int j = 0; j < 300; ++j) {
      EXPECT_EQ(j, arcs3[j].ilabel);
    }
    EXPECT_GE(arena.Size(), 700 + 500 + 300);
    EXPECT_LE(arena.Size(), 2 * (700 + 500 + 300));
    arena.Clear();
  }
}

// Add 2 arcs on first call, 1 arc on second to verify caching behavior.
class TestExpand {
 public:
  using Arc = StdArc;
  using StateId = StdArc::StateId;

  TestExpand() : first_(11, true) {}

  const SymbolTable* InputSymbols() { return nullptr; }
  const SymbolTable* OutputSymbols() { return nullptr; }

  StateId NumStates() const { return 11; }
  StateId Start() const { return 0; }

  template <class State>
  void Expand(StateId state_id, State* state) {
    if (first_[state_id]) {
      first_[state_id] = false;
      state->AddArc(StdArc(0, 0, StdArc::Weight::One(), ++next_));
    }
    state->AddArc(StdArc(0, 0, StdArc::Weight::One(), ++next_));
  }

 private:
  std::vector<bool> first_;
  int next_ = 0;
};

TEST(ArcArenaStateStoreTest, Copy) {
  auto arena = std::make_unique<ArcArenaStateStore<StdArc>>();
  TestExpand expander;
  EXPECT_EQ(arena->FindOrExpand(expander, 0)->NumArcs(), 2);

  ArcArenaStateStore<StdArc> copy(*arena);
  EXPECT_EQ(copy.FindOrExpand(expander, 0)->NumArcs(), 2);    // cached
  EXPECT_EQ(copy.FindOrExpand(expander, 1)->NumArcs(), 2);    // uncached
  EXPECT_EQ(arena->FindOrExpand(expander, 1)->NumArcs(), 1);  // uncached

  // Make sure they didn't use the same arc memory.
  const StdArc* arcs = arena->Find(1)->Arcs();
  EXPECT_EQ(arcs[0].nextstate, 5);

  // Destroy the original store and verify that cached states in the copy
  // (and in a copy-assigned store) remain valid and do not point into the
  // destroyed store's states_ deque.
  ArcArenaStateStore<StdArc> assigned;
  assigned = copy;
  arena.reset();

  ASSERT_NE(copy.Find(0), nullptr);
  EXPECT_EQ(copy.Find(0)->NumArcs(), 2);
  EXPECT_EQ(copy.Find(0)->Arcs()[0].nextstate, 1);
  EXPECT_EQ(copy.Find(0)->Arcs()[1].nextstate, 2);

  const StdArc* copy_arcs = copy.Find(1)->Arcs();
  EXPECT_EQ(copy_arcs[0].nextstate, 3);
  EXPECT_EQ(copy_arcs[1].nextstate, 4);

  ASSERT_NE(assigned.Find(0), nullptr);
  EXPECT_EQ(assigned.Find(0)->NumArcs(), 2);
  EXPECT_EQ(assigned.Find(1)->NumArcs(), 2);
}

TEST(ArcArenaStateStoreTest, UseInExpanderFst) {
  auto expander = std::make_shared<TestExpand>();
  using Cache = ArcArenaStateStore<StdArc>;
  auto cache = std::make_unique<Cache>();
  EXPECT_EQ(cache->FindOrExpand(*expander, 0)->NumArcs(), 2);
  ExpanderFst<TestExpand, Cache> uncached(expander);
  ExpanderFst<TestExpand, Cache> cached(expander, *cache);
  cache.reset();
  EXPECT_EQ(uncached.NumArcs(0), 1);
  EXPECT_EQ(cached.NumArcs(0), 2);
}

}  // namespace
}  // namespace fst

static void BM_ArenaCreateDestroy(benchmark::State& state) {
  int iter_count = 0;
  for (auto s : state) {
    fst::ArcArena<fst::StdArc> arena;
    for (int j = 0; j < 1024; j++) {
      arena.PushArc(
          {iter_count, iter_count, fst::StdArc::Weight::Zero(), iter_count});
      ++iter_count;
    }
  }
}
BENCHMARK(BM_ArenaCreateDestroy);

static void BM_ArenaAddClear(benchmark::State& state) {
  fst::ArcArena<fst::StdArc> arena;
  int iter_count = 0;
  for (auto s : state) {
    for (int j = 0; j < 1024; j++) {
      arena.PushArc(
          {iter_count, iter_count, fst::StdArc::Weight::Zero(), iter_count});
    }
    arena.Clear();
    ++iter_count;
  }
}
BENCHMARK(BM_ArenaAddClear);
