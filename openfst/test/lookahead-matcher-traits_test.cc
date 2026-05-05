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

#include "openfst/lib/lookahead-matcher-traits.h"

#include "gtest/gtest.h"
#include "openfst/lib/accumulator.h"
#include "openfst/lib/const-fst.h"
#include "openfst/lib/add-on.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/label-reachable.h"
#include "openfst/lib/lookahead-matcher.h"
#include "openfst/lib/matcher-fst.h"
#include "openfst/lib/matcher.h"

namespace fst {
namespace {

using ::testing::StaticAssertTypeEq;

TEST(LabelLookAheadMatcherTypesTest, LookAheadMatcher) {
  using Matcher = SortedMatcher<ConstFst<StdArc>>;
  using Accumulator = FastLogAccumulator<StdArc>;
  using LookAheadMatcher =
      LabelLookAheadMatcher<Matcher, ilabel_lookahead_flags, Accumulator,
                            LabelReachable<StdArc, Accumulator>>;
  using Types = LabelLookAheadMatcherTypes<LookAheadMatcher>;

  StaticAssertTypeEq<Matcher, Types::Matcher>();
  StaticAssertTypeEq<Accumulator, Types::Accumulator>();
  StaticAssertTypeEq<StdArc, Types::Arc>();
  StaticAssertTypeEq<LabelReachable<StdArc, Accumulator>, Types::Reachable>();
  EXPECT_EQ(ilabel_lookahead_flags, Types::kFlags);

  StaticAssertTypeEq<
      LabelReachable<StdArc, DefaultAccumulator<StdArc>>,
      Types::ChangeReachableAccumulator<DefaultAccumulator<StdArc>>>();

  StaticAssertTypeEq<
      LabelLookAheadMatcher<Matcher, ilabel_lookahead_flags,
                            DefaultAccumulator<StdArc>,
                            LabelReachable<StdArc, DefaultAccumulator<StdArc>>>,
      Types::ChangeAccumulator<DefaultAccumulator<StdArc>>>();
}

TEST(MatcherFstTypesTest, MatcherFst) {
  using Matcher =
      LabelLookAheadMatcher<SortedMatcher<ConstFst<StdArc>>,
                            ilabel_lookahead_flags, FastLogAccumulator<StdArc>>;
  using MatcherFstType =
      MatcherFst<ConstFst<StdArc>, Matcher, ilabel_lookahead_fst_type,
                 LabelLookAheadRelabeler<StdArc>>;
  using Types = MatcherFstTypes<MatcherFstType>;

  StaticAssertTypeEq<ConstFst<StdArc>, Types::Fst>();
  StaticAssertTypeEq<StdArc, Types::Arc>();
  StaticAssertTypeEq<Matcher, Types::Matcher>();
  StaticAssertTypeEq<Matcher::MatcherData, Types::MatcherData>();
  StaticAssertTypeEq<LabelLookAheadRelabeler<StdArc>, Types::Initializer>();
  StaticAssertTypeEq<AddOnPair<Matcher::MatcherData, Matcher::MatcherData>,
                     Types::AddOn>();

  EXPECT_EQ(ilabel_lookahead_fst_type, Types::name());
}

TEST(MatcherFstTypesTest, ChangeMatcher) {
  using Matcher =
      LabelLookAheadMatcher<SortedMatcher<ConstFst<StdArc>>,
                            ilabel_lookahead_flags, FastLogAccumulator<StdArc>>;
  using MatcherFstType =
      MatcherFst<ConstFst<StdArc>, Matcher, ilabel_lookahead_fst_type,
                 LabelLookAheadRelabeler<StdArc>>;
  using Types = MatcherFstTypes<MatcherFstType>;

  using NewMatcher =
      LabelLookAheadMatcher<SortedMatcher<ConstFst<StdArc>>,
                            ilabel_lookahead_flags, LogAccumulator<StdArc>>;
  using Expected =
      MatcherFst<ConstFst<StdArc>, NewMatcher, ilabel_lookahead_fst_type,
                 LabelLookAheadRelabeler<StdArc>>;
  StaticAssertTypeEq<Expected, Types::ChangeMatcher<NewMatcher>>();
}

TEST(IsLabelLookAheadMatcherTest, LookAheadMatcher) {
  using Matcher = SortedMatcher<ConstFst<StdArc>>;
  using LookAheadMatcher =
      LabelLookAheadMatcher<Matcher, ilabel_lookahead_flags>;

  EXPECT_TRUE(IsLabelLookAheadMatcher<LookAheadMatcher>::value);
  EXPECT_FALSE(IsLabelLookAheadMatcher<Matcher>::value);
}

TEST(IsLabelLookAheadMatcherFstTest, LookAheadMatcherFst) {
  EXPECT_TRUE(IsLabelLookAheadMatcherFst<StdILabelLookAheadFst>::value);
  EXPECT_TRUE(IsLabelLookAheadMatcherFst<StdOLabelLookAheadFst>::value);
  EXPECT_FALSE(IsLabelLookAheadMatcherFst<StdArcLookAheadFst>::value);
}

}  // namespace
}  // namespace fst
