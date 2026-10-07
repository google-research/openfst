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
// Unit test for lookahead matchers and filters.

#include "openfst/test/lookahead_test.h"

#include <cstdint>

#include "gtest/gtest.h"
#include "absl/flags/flag.h"
#include "openfst/lib/accumulator.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/compact-fst.h"
#include "openfst/lib/const-fst.h"
#include "openfst/lib/fst-decl.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/label-reachable.h"
#include "openfst/lib/lookahead-matcher.h"
#include "openfst/lib/matcher-fst.h"
#include "openfst/lib/matcher.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/test/compactors.h"

namespace fst {
namespace {

using Arc = StdArc;
using StateId = Arc::StateId;
using Weight = Arc::Weight;
using Label = Arc::Label;

template <typename Arc>
using TrivialCompactArcFst =
    CompactArcFst<Arc, TrivialArcCompactor<Arc>, uint32_t>;

template <typename Arc>
using TrivialCompactFst = CompactFst<Arc, TrivialCompactor<Arc>>;

using LookAheadTypes =
    ::testing::Types<ConstFst<Arc>, VectorFst<Arc>, TrivialCompactArcFst<Arc>,
                     TrivialCompactFst<Arc>>;

INSTANTIATE_TYPED_TEST_SUITE_P(LookAhead, LookAheadTest, LookAheadTypes);

class SafeStateMatcher : public MatcherBase<Arc> {
 public:
  using FST = VectorFst<Arc>;
  using Label = Arc::Label;
  using StateId = Arc::StateId;
  using Weight = Arc::Weight;

  SafeStateMatcher(const FST& fst, MatchType match_type)
      : matcher_(fst, match_type) {}

  SafeStateMatcher(const FST* fst, MatchType match_type)
      : matcher_(fst, match_type) {}

  SafeStateMatcher(const SafeStateMatcher& matcher, bool safe = false)
      : matcher_(matcher.matcher_, safe) {}

  SafeStateMatcher* Copy(bool safe = false) const override {
    return new SafeStateMatcher(*this, safe);
  }

  MatchType Type(bool test) const override { return matcher_.Type(test); }

  void SetState(StateId s) final {
    state_ = s;
    if (s != kNoStateId) matcher_.SetState(s);
  }

  bool Find(Label label) final {
    if (state_ == kNoStateId) return false;
    return matcher_.Find(label);
  }

  bool Done() const final { return matcher_.Done(); }
  const Arc& Value() const final { return matcher_.Value(); }
  void Next() final { matcher_.Next(); }
  Weight Final(StateId s) const final { return matcher_.Final(s); }
  ssize_t Priority(StateId s) final { return matcher_.Priority(s); }
  const FST& GetFst() const override { return matcher_.GetFst(); }
  uint64_t Properties(uint64_t props) const override {
    return matcher_.Properties(props);
  }
  uint32_t Flags() const override { return matcher_.Flags(); }

 private:
  SortedMatcher<FST> matcher_;
  StateId state_ = kNoStateId;
};

class SafeStateReachable : public LabelReachable<Arc> {
 public:
  using Base = LabelReachable<Arc>;
  using Base::Base;
  using Base::Reach;

  SafeStateReachable(const SafeStateReachable& reachable, bool safe = false)
      : Base(reachable, safe) {}

  void SetState(StateId s, StateId aiter_s = kNoStateId) {
    state_ = s;
    if (s != kNoStateId) Base::SetState(s, aiter_s);
  }

  bool Reach(Label label) const {
    if (state_ == kNoStateId) return false;
    return Base::Reach(label);
  }

 private:
  StateId state_ = kNoStateId;
};

TEST(LabelLookAheadMatcherTest, InitializesSetStateFlags) {
  VectorFst<Arc> fst;
  const StateId s0 = fst.AddState();
  const StateId s1 = fst.AddState();
  fst.SetStart(s0);
  fst.SetFinal(s1, Weight::One());
  fst.AddArc(s0, Arc(1, 1, Weight::One(), s1));

  using Matcher =
      LabelLookAheadMatcher<SafeStateMatcher, olabel_lookahead_flags,
                            DefaultAccumulator<Arc>, SafeStateReachable>;
  Matcher matcher(fst, MATCH_OUTPUT);
  // Calling Find and LookAheadLabel before SetState(s0) (or after
  // SetState(kNoStateId), which returns early) reads match_set_state_ and
  // reach_set_state_ while state_ is still kNoStateId.
  matcher.SetState(kNoStateId);
  EXPECT_FALSE(matcher.Find(1));
  EXPECT_FALSE(matcher.LookAheadLabel(1));

  matcher.SetState(s0);
  EXPECT_TRUE(matcher.Find(1));
  EXPECT_TRUE(matcher.LookAheadLabel(1));

  // Copy constructor resets state_ to kNoStateId and must also initialize
  // match_set_state_ and reach_set_state_ to false.
  Matcher copied(matcher);
  EXPECT_FALSE(copied.Find(1));
  EXPECT_FALSE(copied.LookAheadLabel(1));
  copied.SetState(s0);
  EXPECT_TRUE(copied.Find(1));
  EXPECT_TRUE(copied.LookAheadLabel(1));
}

}  // namespace
}  // namespace fst
