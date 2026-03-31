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

#include "openfst/lib/expander-fst.h"

#include <memory>

#include "gtest/gtest.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/cache.h"
#include "openfst/lib/equal.h"
#include "openfst/lib/expanded-fst.h"
#include "openfst/lib/expander-cache.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/symbol-table.h"
#include "openfst/lib/vector-fst.h"

namespace fst {

class ExpanderFstTest : public testing::Test {};

class TestExpand {
 public:
  using Arc = StdArc;
  using StateId = StdArc::StateId;

  TestExpand() = default;

  const SymbolTable* InputSymbols() { return nullptr; }
  const SymbolTable* OutputSymbols() { return nullptr; }

  StateId NumStates() const { return 11; }
  StateId Start() const { return 0; }

  template <class State>
  void Expand(StateId state_id, State* state) {
    if (state_id < 9) {
      state->AddArc(StdArc(0, 0, StdArc::Weight::One(), state_id + 1));
    }
    if (state_id < 10) {
      state->AddArc(StdArc(0, 0, StdArc::Weight::One(), state_id + 2));
    }
    if (state_id == 10) {
      state->SetFinal(StdArc::Weight(1.0f));
    }
  }
};

class CountingExpand {
 public:
  using Arc = StdArc;
  using StateId = StdArc::StateId;

  CountingExpand() = default;

  const SymbolTable* InputSymbols() { return nullptr; }
  const SymbolTable* OutputSymbols() { return nullptr; }

  StateId NumStates() const { return 11; }
  StateId Start() const { return 0; }

  template <class State>
  void Expand(StateId state_id, State* state) {
    ++expansions_;
    if (state_id < 9) {
      state->AddArc(StdArc(0, 0, StdArc::Weight::One(), state_id + 1));
    }
    if (state_id < 10) {
      state->AddArc(StdArc(0, 0, StdArc::Weight::One(), state_id + 2));
    }
    if (state_id == 10) {
      state->SetFinal(StdArc::Weight(1.0f));
    }
  }

  int expansions() const { return expansions_; }
  void reset_expansions() { expansions_ = 0; }

 private:
  int expansions_ = 0;
};

TEST_F(ExpanderFstTest, SimpleExpander) {
  ExpanderFst<TestExpand> fst(std::make_shared<TestExpand>());
  ASSERT_EQ(11, CountStates(fst));
  EXPECT_EQ(2, fst.NumArcs(0));
  EXPECT_EQ(2, fst.NumArcs(1));

  ArcIterator<ExpanderFst<TestExpand>> custom_ai(fst, fst.Start());

  // Test arc iterator state ref-counting.
  ArcIterator<StdFst> aiter1(fst, 0);
  ArcIterator<StdFst> aiter2(fst, 1);
  EXPECT_EQ(1, aiter1.Value().nextstate);
  EXPECT_EQ(2, aiter2.Value().nextstate);

  VectorFst<StdArc> vfst(fst);
  EXPECT_TRUE(Equal(fst, vfst));
}

TEST_F(ExpanderFstTest, CopyExpanderFst) {
  using TestFst = ExpanderFst<TestExpand>;
  TestFst fst(std::make_shared<TestExpand>());
  std::unique_ptr<TestFst> copy(fst.Copy());
  EXPECT_EQ(CountStates(fst), CountStates(*copy));

  ExpanderFst<TestExpand> other_copy = fst;
  EXPECT_EQ(CountStates(fst), CountStates(other_copy));
}

// Check difficult order of ArcIterator references.
TEST_F(ExpanderFstTest, NoGcKeepOneExpanderCache) {
  using Cache = NoGcKeepOneExpanderCache<StdArc>;
  ExpanderFst<TestExpand, Cache> fst(std::make_shared<TestExpand>());
  using ArcIter = ArcIterator<StdFst>;
  {
    ArcIter a_0(fst, 0);
    {
      ArcIter a_1(fst, 1);
      ArcIter a_2(fst, 2);
    }
    ArcIter a_2(fst, 2);
    ArcIter a_1(fst, 1);
    EXPECT_EQ(1, a_0.Value().nextstate);
    EXPECT_EQ(2, a_1.Value().nextstate);
    EXPECT_EQ(3, a_2.Value().nextstate);
  }
  ArcIter a_0(fst, 0);
  ArcIter a_1(fst, 1);
  EXPECT_EQ(1, a_0.Value().nextstate);
  EXPECT_EQ(2, a_1.Value().nextstate);
}

TEST_F(ExpanderFstTest, ArcIteratorSpecialization) {
  // TODO: Is there a way to check from the test that the correct
  // specializer got picked? Otherwise this test doesn't necessarily test the
  // code it is supposed to test.
  ExpanderFst<TestExpand> fst(std::make_shared<TestExpand>());

  ArcIterator<StdFst> aiter_expect(fst, 3);
  ArcIterator<ExpanderFst<TestExpand>> aiter_test(fst, 3);
  EXPECT_EQ(aiter_test.Done(), aiter_expect.Done());
  EXPECT_EQ(aiter_test.Position(), aiter_expect.Position());
  EXPECT_EQ(aiter_test.Value().nextstate, aiter_expect.Value().nextstate);

  aiter_expect.Next();
  aiter_test.Next();
  EXPECT_EQ(aiter_test.Done(), aiter_expect.Done());
  EXPECT_EQ(aiter_test.Position(), aiter_expect.Position());
  EXPECT_EQ(aiter_test.Value().nextstate, aiter_expect.Value().nextstate);

  aiter_expect.Reset();
  aiter_test.Reset();
  EXPECT_EQ(aiter_test.Done(), aiter_expect.Done());
  EXPECT_EQ(aiter_test.Position(), aiter_expect.Position());
  EXPECT_EQ(aiter_test.Value().nextstate, aiter_expect.Value().nextstate);

  aiter_expect.Seek(1);
  aiter_test.Seek(1);
  EXPECT_EQ(aiter_test.Done(), aiter_expect.Done());
  EXPECT_EQ(aiter_test.Position(), aiter_expect.Position());
  EXPECT_EQ(aiter_test.Value().nextstate, aiter_expect.Value().nextstate);
}

TEST_F(ExpanderFstTest, HashExpanderCache) {
  ExpanderFst<TestExpand> expect(std::make_shared<TestExpand>());
  VectorFst<StdArc> vfst(expect);

  ExpanderFst<TestExpand, HashExpanderCache<StdArc>> fst(
      std::make_shared<TestExpand>());
  EXPECT_TRUE(Equal(fst, vfst));  // uncached
  EXPECT_TRUE(Equal(fst, vfst));  // cached
}

TEST_F(ExpanderFstTest, VectorExpanderCacheAssignment) {
  auto expander = std::make_shared<CountingExpand>();
  ExpanderFst<CountingExpand> fst_dense(expander);
  VectorFst<StdArc> vfst_dense(fst_dense);
  expander->reset_expansions();

  VectorExpanderCache<StdArc> sparse_cache;
  sparse_cache.FindOrExpand(*expander, 0);
  sparse_cache.FindOrExpand(*expander, 2);
  expander->reset_expansions();

  // Assign sparse cache to dense fst.
  *fst_dense.GetCache() = sparse_cache;
  fst_dense.GetCache()->FindOrExpand(*expander, 1);
  EXPECT_EQ(expander->expansions(), 1);
}

TEST_F(ExpanderFstTest, HashExpanderCacheAssignment) {
  auto expander = std::make_shared<CountingExpand>();
  ExpanderFst<CountingExpand, HashExpanderCache<StdArc>> fst_dense(expander);
  VectorFst<StdArc> vfst_dense(fst_dense);
  expander->reset_expansions();

  HashExpanderCache<StdArc> sparse_cache;
  sparse_cache.FindOrExpand(*expander, 0);
  sparse_cache.FindOrExpand(*expander, 2);
  expander->reset_expansions();

  // Assign sparse cache to dense fst.
  *fst_dense.GetCache() = sparse_cache;
  fst_dense.GetCache()->FindOrExpand(*expander, 1);
  EXPECT_EQ(expander->expansions(), 1);
}

TEST_F(ExpanderFstTest, WrapCacheStoreTest) {
  ExpanderFst<TestExpand> expect(std::make_shared<TestExpand>());
  VectorFst<StdArc> vfst(expect);

  using Cache = ExpanderCacheStore<HashCacheStore<CacheState<StdArc>>>;
  ExpanderFst<TestExpand, Cache> fst(std::make_shared<TestExpand>());
  ASSERT_EQ(11, CountStates(fst));
  ASSERT_EQ(fst.NumArcs(0), vfst.NumArcs(0));
  ASSERT_EQ(fst.NumInputEpsilons(0), vfst.NumInputEpsilons(0));
  EXPECT_TRUE(Equal(fst, vfst));  // uncached
  EXPECT_TRUE(Equal(fst, vfst));  // cached
}

}  // namespace fst
