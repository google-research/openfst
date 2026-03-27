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
// Unit test for composition state tables.

#include "openfst/lib/state-table.h"

#include <memory>
#include <string>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/compose-filter.h"
#include "openfst/lib/compose.h"
#include "openfst/lib/equal.h"
#include "openfst/lib/expanded-fst.h"
#include "openfst/lib/filter-state.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/matcher.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/lib/verify.h"

namespace fst {
namespace {

template <class S, class F>
class ComposeFilterStateSelector {
 public:
  explicit ComposeFilterStateSelector(const F& f) : filter_state_(f) {}

  bool operator()(const DefaultComposeStateTuple<S, F>& tuple) const {
    return tuple.GetFilterState() == filter_state_;
  }

 private:
  F filter_state_;
};

template <typename A, typename F,
          typename T = DefaultComposeStateTuple<typename A::StateId, F>>
class VectorHashComposeStateTable
    : public VectorHashStateTable<
          T, ComposeFilterStateSelector<typename A::StateId, F>,
          ComposeFingerprint<T>, ComposeFingerprint<T>> {
 public:
  using Arc = A;
  using StateId = typename A::StateId;
  using FilterState = F;
  using StateTuple = T;
  using StateTable =
      VectorHashStateTable<DefaultComposeStateTuple<StateId, F>,
                           ComposeFilterStateSelector<StateId, F>,
                           ComposeFingerprint<T>, ComposeFingerprint<T>>;

  VectorHashComposeStateTable(const Fst<A>& fst1, const Fst<A>& fst2)
      : StateTable(
            ComposeFilterStateSelector<StateId, F>(FilterState::NoState()),
            ComposeFingerprint<T>(CountStates(fst1), 0),
            ComposeFingerprint<T>(CountStates(fst1), CountStates(fst2))) {}

  explicit VectorHashComposeStateTable(
      const VectorHashComposeStateTable<A, F>& table)
      : StateTable(ComposeFilterStateSelector<StateId, F>(table.Selector()),
                   ComposeFingerprint<T>(table.Fingerprint()),
                   ComposeFingerprint<T>(table.HashFunction())) {}

  bool Error() { return false; }
};

using Arc = StdArc;
using StateId = Arc::StateId;
using Weight = Arc::Weight;
using Label = Arc::Label;
using FilterState = CharFilterState;

class StateTableTest : public testing::Test {
 protected:
  void SetUp() override {
    const std::string path =
        JoinPath(std::string("."),
                       "openfst/test/testdata/state-table");
    const std::string sttable1_name = JoinPath(path, "st1.fst");
    const std::string sttable2_name = JoinPath(path, "st2.fst");

    stfst1_.reset(VectorFst<Arc>::Read(sttable1_name));
    stfst2_.reset(VectorFst<Arc>::Read(sttable2_name));
  }

  std::unique_ptr<const VectorFst<Arc>> stfst1_;
  std::unique_ptr<const VectorFst<Arc>> stfst2_;
};

// Checks that composition using the generic (hash-based) state table is
// equal to the product (vector-based) state table.
TEST_F(StateTableTest, ProductStateTableTest) {
  using M = Matcher<Fst<Arc>>;
  ComposeFst<Arc> cfst1(*stfst1_, *stfst2_);
  ComposeFstOptions<
      Arc, M, SequenceComposeFilter<M>,
      ProductComposeStateTable<
          Arc, DefaultComposeStateTuple<Arc::StateId, FilterState>>>
      opts;

  ComposeFst<Arc> cfst2(*stfst1_, *stfst2_, opts);
  ASSERT_TRUE(Verify(cfst1));
  ASSERT_TRUE(Verify(cfst2));
  ASSERT_TRUE(Equal(cfst1, cfst2));
}

// Checks that composition using the generic state table is
// equal to the erasable state table.
TEST_F(StateTableTest, ErasableStateTableTest) {
  using M = Matcher<Fst<Arc>>;
  using StateTuple = DefaultComposeStateTuple<Arc::StateId, FilterState>;
  ComposeFst<Arc> cfst1(*stfst1_, *stfst2_);
  ComposeFstOptions<Arc, M, SequenceComposeFilter<M>,
                    ErasableComposeStateTable<Arc, StateTuple>>
      opts;
  opts.state_table =
      new ErasableComposeStateTable<Arc, StateTuple>(*stfst1_, *stfst2_);
  ComposeFst<Arc> cfst2(*stfst1_, *stfst2_, opts);
  ASSERT_TRUE(Verify(cfst1));
  ASSERT_TRUE(Verify(cfst2));
  // Erase state table elements
  for (StateIterator<Fst<Arc>> siter(cfst2); !siter.Done(); siter.Next())
    opts.state_table->Erase(siter.Value());
  // Test safe and correct since cfst2 results are now cached.
  ASSERT_TRUE(Equal(cfst1, cfst2));
}

// Checks that composition using the generic state table is
// eual to the composition using the two-level state table.
TEST_F(StateTableTest, VectorHashStateTableTest) {
  using M = Matcher<Fst<Arc>>;
  ComposeFst<Arc> cfst1(*stfst1_, *stfst2_);
  ComposeFstOptions<Arc, M, SequenceComposeFilter<M>,
                    VectorHashComposeStateTable<Arc, FilterState>>
      opts;
  opts.state_table =
      new VectorHashComposeStateTable<Arc, FilterState>(*stfst1_, *stfst2_);
  ComposeFst<Arc> cfst2(*stfst1_, *stfst2_, opts);
  ASSERT_TRUE(Verify(cfst1));
  ASSERT_TRUE(Verify(cfst2));
  ASSERT_TRUE(Equal(cfst1, cfst2));
}

}  // namespace
}  // namespace fst
