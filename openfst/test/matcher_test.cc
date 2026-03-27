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
// Unit test for composition matchers.

#include "openfst/lib/matcher.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/arcsort.h"
#include "openfst/lib/cache.h"
#include "openfst/lib/compose-filter.h"
#include "openfst/lib/compose.h"
#include "openfst/lib/equal.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/relabel.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/lib/verify.h"

namespace fst {
namespace {

using Arc = StdArc;
using StateId = Arc::StateId;
using Weight = Arc::Weight;
using Label = Arc::Label;

using TestRhoMatcher = RhoMatcher<Matcher<Fst<Arc>>>;
using TestSigmaMatcher = SigmaMatcher<Matcher<Fst<Arc>>>;
using TestPhiMatcher = PhiMatcher<Matcher<Fst<Arc>>>;
using TestMultiEpsMatcher = MultiEpsMatcher<Matcher<Fst<Arc>>>;

using TestMultiEpsFilter =
    MultiEpsFilter<SequenceComposeFilter<TestMultiEpsMatcher>>;

// Special label used alternatively to represent Phi, Rho or Sigma transitions.
constexpr int kSpecialLabel = -2;

class MatcherTest : public testing::Test {
 protected:
  void SetUp() override {
    const std::string path =
        JoinPath(std::string("."),
                       "openfst/test/testdata/matcher");
    const std::string matcher1_name = JoinPath(path, "m1.fst");
    const std::string matcher2_name = JoinPath(path, "m2.fst");
    const std::string matcher3_name = JoinPath(path, "m3.fst");
    const std::string matcher4_name = JoinPath(path, "m4.fst");
    const std::string matcher5_name = JoinPath(path, "m5.fst");
    const std::string matcher6_name = JoinPath(path, "m6.fst");
    const std::string matcher7_name = JoinPath(path, "m7.fst");
    const std::string matcher8_name = JoinPath(path, "m8.fst");
    const std::string matcher9_name = JoinPath(path, "m9.fst");
    const std::string matcher10_name = JoinPath(path, "m10.fst");
    const std::string matcher11_name = JoinPath(path, "m11.fst");
    const std::string matcher12_name = JoinPath(path, "m12.fst");
    const std::string matcher13_name = JoinPath(path, "m13.fst");
    const std::string matcher14_name = JoinPath(path, "m14.fst");
    const std::string lm_name = JoinPath(path, "lm.fst");
    const std::string a_name = JoinPath(path, "a.fst");
    const std::string bb_name = JoinPath(path, "bb.fst");
    const std::string a_bb_name = JoinPath(path, "a_bb.fst");
    const std::string lm_a_name = JoinPath(path, "lm.a.fst");
    const std::string lm_bb_name = JoinPath(path, "lm.bb.fst");
    const std::string lm_a_bb_name = JoinPath(path, "lm.a_bb.fst");

    mfst1_.reset(VectorFst<Arc>::Read(matcher1_name));
    mfst2_.reset(VectorFst<Arc>::Read(matcher2_name));
    mfst3_.reset(VectorFst<Arc>::Read(matcher3_name));
    mfst4_.reset(VectorFst<Arc>::Read(matcher4_name));
    mfst5_.reset(VectorFst<Arc>::Read(matcher5_name));
    mfst6_.reset(VectorFst<Arc>::Read(matcher6_name));
    mfst7_.reset(VectorFst<Arc>::Read(matcher7_name));
    mfst8_.reset(VectorFst<Arc>::Read(matcher8_name));
    mfst9_.reset(VectorFst<Arc>::Read(matcher9_name));
    mfst10_.reset(VectorFst<Arc>::Read(matcher10_name));
    mfst11_.reset(VectorFst<Arc>::Read(matcher11_name));
    mfst12_.reset(VectorFst<Arc>::Read(matcher12_name));
    mfst13_.reset(VectorFst<Arc>::Read(matcher13_name));
    mfst14_.reset(VectorFst<Arc>::Read(matcher14_name));
    lm_fst_.reset(VectorFst<Arc>::Read(lm_name));
    a_fst_.reset(VectorFst<Arc>::Read(a_name));
    bb_fst_.reset(VectorFst<Arc>::Read(bb_name));
    a_bb_fst_.reset(VectorFst<Arc>::Read(a_bb_name));
    lm_a_fst_.reset(VectorFst<Arc>::Read(lm_a_name));
    lm_bb_fst_.reset(VectorFst<Arc>::Read(lm_bb_name));
    lm_a_bb_fst_.reset(VectorFst<Arc>::Read(lm_a_bb_name));

    // Renames label '1000' to kSpecialLabel (trick needed to bypass the
    // requirement that all labels be non-negative in order for
    // fstcompile to succeed).
    std::vector<std::pair<Label, Label>> relabel;
    relabel.push_back(std::make_pair(1000, kSpecialLabel));
    Relabel(mfst2_.get(), relabel, relabel);
    ILabelCompare<Arc> ic;
    ArcSort(mfst2_.get(), ic);
  }

  template <class Matcher>
  void IterateAndMatch(const typename Matcher::FST& fst,
                       Matcher* matcher) const {
    for (StateIterator<Fst<Arc>> siter(fst); !siter.Done(); siter.Next()) {
      const auto s = siter.Value();
      matcher->SetState(s);
      EXPECT_TRUE(matcher->Find(0));
      EXPECT_FALSE(matcher->Done());
      matcher->Next();
      typename Arc::Label label = kNoLabel;
      typename Arc::Label max_label = 0;
      for (ArcIterator<Fst<Arc>> aiter(fst, s); !aiter.Done(); aiter.Next()) {
        const Arc& arc = aiter.Value();
        if (arc.ilabel > max_label) max_label = arc.ilabel;
        if (aiter.Position() == 0) {
          EXPECT_TRUE((arc.ilabel == 0) == !matcher->Done());
          label = arc.ilabel;
          EXPECT_TRUE(matcher->Find(label == 0 ? kNoLabel : label));
        } else if (label != arc.ilabel) {
          EXPECT_TRUE(matcher->Done());
          label = arc.ilabel;
          EXPECT_TRUE(matcher->Find(label == 0 ? kNoLabel : label));
        }
        EXPECT_TRUE(!matcher->Done());
        EXPECT_EQ(matcher->Value().ilabel, arc.ilabel);
        matcher->Next();
      }
      EXPECT_FALSE(matcher->Find(max_label + 1));
      EXPECT_EQ(matcher->Final(s), fst.Final(s));
    }
  }

  template <class Matcher>
  void IterativeTest(const typename Matcher::FST& fst, int count) const {
    Matcher matcher(fst, MATCH_INPUT);
    EXPECT_EQ(matcher.Type(true), MATCH_INPUT);
    for (int i = 0; i < count; ++i) {
      IterateAndMatch(fst, &matcher);
      std::unique_ptr<Matcher> matcherp(new Matcher(matcher, /*safe=*/false));
      IterateAndMatch(fst, matcherp.get());
      matcherp.reset(new Matcher(matcher, /*safe=*/true));
      IterateAndMatch(fst, matcherp.get());
    }
  }

  std::unique_ptr<const VectorFst<Arc>> mfst1_;
  std::unique_ptr<VectorFst<Arc>> mfst2_;
  std::unique_ptr<const VectorFst<Arc>> mfst3_;
  std::unique_ptr<const VectorFst<Arc>> mfst4_;
  std::unique_ptr<const VectorFst<Arc>> mfst5_;
  std::unique_ptr<const VectorFst<Arc>> mfst6_;
  std::unique_ptr<const VectorFst<Arc>> mfst7_;
  std::unique_ptr<const VectorFst<Arc>> mfst8_;
  std::unique_ptr<const VectorFst<Arc>> mfst9_;
  std::unique_ptr<const VectorFst<Arc>> mfst10_;
  std::unique_ptr<const VectorFst<Arc>> mfst11_;
  std::unique_ptr<const VectorFst<Arc>> mfst12_;
  std::unique_ptr<const VectorFst<Arc>> mfst13_;
  std::unique_ptr<const VectorFst<Arc>> mfst14_;

  std::unique_ptr<VectorFst<Arc>> lm_fst_;
  std::unique_ptr<const VectorFst<Arc>> a_fst_;
  std::unique_ptr<const VectorFst<Arc>> bb_fst_;
  std::unique_ptr<const VectorFst<Arc>> a_bb_fst_;
  std::unique_ptr<const VectorFst<Arc>> lm_a_fst_;
  std::unique_ptr<const VectorFst<Arc>> lm_bb_fst_;
  std::unique_ptr<const VectorFst<Arc>> lm_a_bb_fst_;
};

TEST_F(MatcherTest, ComposeWithRhoMatcher) {
  ComposeFst<Arc> cfst(
      *mfst1_, *mfst2_,
      ComposeFstOptions<Arc, TestRhoMatcher>(
          CacheOptions(), new TestRhoMatcher(*mfst1_, MATCH_NONE, kNoLabel),
          new TestRhoMatcher(*mfst2_, MATCH_INPUT, kSpecialLabel)));
  ASSERT_TRUE(Verify(cfst));
  ASSERT_TRUE(Equal(*mfst3_, cfst));

  // To use the MatcherBase pointers during composition, instantiate
  // ComposeFstOptions with the Matcher<Fst<Arc>> template parameter
  MatcherBase<StdArc>* base_ptr1 =
      new TestRhoMatcher(*mfst1_, MATCH_NONE, kNoLabel);
  MatcherBase<StdArc>* base_ptr2 =
      new TestRhoMatcher(*mfst2_, MATCH_INPUT, kSpecialLabel);
  ComposeFst<Arc> cfst2(*mfst1_, *mfst2_,
                        ComposeFstOptions<Arc, Matcher<Fst<Arc>>>(
                            CacheOptions(), new Matcher<Fst<Arc>>(base_ptr1),
                            new Matcher<Fst<Arc>>(base_ptr2)));
  ASSERT_TRUE(Verify(cfst2));
  ASSERT_TRUE(Equal(*mfst3_, cfst2));

  ComposeFst<Arc> cfst3(
      *mfst10_, *mfst11_,
      ComposeFstOptions<Arc, TestRhoMatcher>(
          CacheOptions(),
          new TestRhoMatcher(*mfst10_, MATCH_OUTPUT, kSpecialLabel),
          new TestRhoMatcher(*mfst11_, MATCH_INPUT, kSpecialLabel)));
  ASSERT_TRUE(Verify(cfst3));
  ASSERT_TRUE(Equal(*mfst12_, cfst3));
}

TEST_F(MatcherTest, ComposeWithPhiMatcher) {
  ComposeFst<Arc> cfst(
      *mfst1_, *mfst2_,
      ComposeFstOptions<Arc, TestPhiMatcher>(
          CacheOptions(), new TestPhiMatcher(*mfst1_, MATCH_NONE, kNoLabel),
          new TestPhiMatcher(*mfst2_, MATCH_INPUT, kSpecialLabel)));
  ASSERT_TRUE(Verify(cfst));
  ASSERT_TRUE(Equal(*mfst4_, cfst));

  ComposeFst<Arc> cfst2(
      *mfst10_, *mfst11_,
      ComposeFstOptions<Arc, TestPhiMatcher>(
          CacheOptions(),
          new TestPhiMatcher(*mfst10_, MATCH_OUTPUT, kSpecialLabel),
          new TestPhiMatcher(*mfst11_, MATCH_INPUT, kSpecialLabel)));
  ASSERT_TRUE(Verify(cfst2));
  ASSERT_TRUE(Equal(*mfst13_, cfst2));
}

TEST_F(MatcherTest, ComposeWithSigmaMatcher) {
  ComposeFst<Arc> cfst(
      *mfst1_, *mfst2_,
      ComposeFstOptions<Arc, TestSigmaMatcher>(
          CacheOptions(), new TestSigmaMatcher(*mfst1_, MATCH_NONE, kNoLabel),
          new TestSigmaMatcher(*mfst2_, MATCH_INPUT, kSpecialLabel)));
  ASSERT_TRUE(Verify(cfst));
  ASSERT_TRUE(Equal(*mfst5_, cfst));

  ComposeFst<Arc> cfst2(
      *mfst10_, *mfst11_,
      ComposeFstOptions<Arc, TestSigmaMatcher>(
          CacheOptions(),
          new TestSigmaMatcher(*mfst10_, MATCH_OUTPUT, kSpecialLabel),
          new TestSigmaMatcher(*mfst11_, MATCH_INPUT, kSpecialLabel)));
  ASSERT_TRUE(Verify(cfst2));
  ASSERT_TRUE(Equal(*mfst14_, cfst2));
}

TEST_F(MatcherTest, ComposeWithLanguageModel) {
  for (auto i = 0; i < 2; ++i) {
    Label phi_label = i == 0 ? 0 : kSpecialLabel;
    if (i == 1) {
      // Relabeled back-off transitions with kSpecialLabel.
      std::vector<std::pair<Label, Label>> relabel;
      relabel.push_back(std::make_pair(0, kSpecialLabel));
      Relabel(lm_fst_.get(), relabel, relabel);
    }
    VectorFst<Arc> fst;

    // Composes LM with string "a".
    fst = ComposeFst<Arc>(
        *lm_fst_, *a_fst_,
        ComposeFstOptions<Arc, TestPhiMatcher>(
            CacheOptions(),
            new TestPhiMatcher(*lm_fst_, MATCH_OUTPUT, phi_label),
            new TestPhiMatcher(*a_fst_, MATCH_NONE, kNoLabel)));
    ASSERT_TRUE(Verify(fst));
    EXPECT_TRUE(Equal(fst, *lm_a_fst_));

    fst = ComposeFst<Arc>(
        *a_fst_, *lm_fst_,
        ComposeFstOptions<Arc, TestPhiMatcher>(
            CacheOptions(), new TestPhiMatcher(*a_fst_, MATCH_NONE, kNoLabel),
            new TestPhiMatcher(*lm_fst_, MATCH_INPUT, phi_label)));
    ASSERT_TRUE(Verify(fst));
    EXPECT_TRUE(Equal(fst, *lm_a_fst_));

    // Composes LM with string "bb".
    fst = ComposeFst<Arc>(
        *lm_fst_, *bb_fst_,
        ComposeFstOptions<Arc, TestPhiMatcher>(
            CacheOptions(),
            new TestPhiMatcher(*lm_fst_, MATCH_OUTPUT, phi_label),
            new TestPhiMatcher(*bb_fst_, MATCH_NONE, kNoLabel)));
    ASSERT_TRUE(Verify(fst));
    EXPECT_TRUE(Equal(fst, *lm_bb_fst_));

    fst = ComposeFst<Arc>(
        *bb_fst_, *lm_fst_,
        ComposeFstOptions<Arc, TestPhiMatcher>(
            CacheOptions(), new TestPhiMatcher(*bb_fst_, MATCH_NONE, kNoLabel),
            new TestPhiMatcher(*lm_fst_, MATCH_INPUT, phi_label)));
    ASSERT_TRUE(Verify(fst));
    EXPECT_TRUE(Equal(fst, *lm_bb_fst_));

    // Checks handling of epsilon transitions:
    // composes LM with string "a" <epsilon> "bb".
    fst = ComposeFst<Arc>(
        *lm_fst_, *a_bb_fst_,
        ComposeFstOptions<Arc, TestPhiMatcher>(
            CacheOptions(),
            new TestPhiMatcher(*lm_fst_, MATCH_OUTPUT, phi_label),
            new TestPhiMatcher(*a_bb_fst_, MATCH_NONE, kNoLabel)));
    fst.Write("/tmp/r.fst");
    ASSERT_TRUE(Verify(fst));
    EXPECT_TRUE(Equal(fst, *lm_a_bb_fst_));

    fst = ComposeFst<Arc>(
        *a_bb_fst_, *lm_fst_,
        ComposeFstOptions<Arc, TestPhiMatcher>(
            CacheOptions(),
            new TestPhiMatcher(*a_bb_fst_, MATCH_NONE, kNoLabel),
            new TestPhiMatcher(*lm_fst_, MATCH_INPUT, phi_label)));
    ASSERT_TRUE(Verify(fst));
    EXPECT_TRUE(Equal(fst, *lm_a_bb_fst_));
  }
}

TEST_F(MatcherTest, ComposeWithMultiEpsMatcher) {
  TestMultiEpsMatcher* m1 = new TestMultiEpsMatcher(*mfst6_, MATCH_NONE);
  TestMultiEpsMatcher* m2 = new TestMultiEpsMatcher(*mfst7_, MATCH_INPUT);
  m2->AddMultiEpsLabel(3);
  m2->AddMultiEpsLabel(4);

  ComposeFst<Arc> cfst(
      *mfst6_, *mfst7_,
      ComposeFstOptions<Arc, TestMultiEpsMatcher, TestMultiEpsFilter>(
          CacheOptions(), m1, m2));
  ASSERT_TRUE(Verify(cfst));
  ASSERT_TRUE(Equal(*mfst8_, cfst));
}

TEST_F(MatcherTest, ComposeWithKeepMultiEpsMatcher) {
  TestMultiEpsMatcher* m1 = new TestMultiEpsMatcher(*mfst6_, MATCH_NONE);
  TestMultiEpsMatcher* m2 = new TestMultiEpsMatcher(*mfst7_, MATCH_INPUT);
  TestMultiEpsFilter* filt =
      new TestMultiEpsFilter(*mfst6_, *mfst7_, m1, m2, true);
  m2->AddMultiEpsLabel(3);
  m2->AddMultiEpsLabel(4);

  ComposeFst<Arc> cfst(
      *mfst6_, *mfst7_,
      ComposeFstOptions<Arc, TestMultiEpsMatcher, TestMultiEpsFilter>(
          CacheOptions(), m1, m2, filt));
  ASSERT_TRUE(Verify(cfst));
  ASSERT_TRUE(Equal(*mfst9_, cfst));
}

TEST_F(MatcherTest, PhiMatcherFinal) {
  TestPhiMatcher matcher1(*lm_fst_, MATCH_INPUT, 0);
  for (Arc::StateId s = 0; s < lm_fst_->NumStates(); ++s) {
    EXPECT_EQ(matcher1.Final(s), lm_fst_->Final(s));
  }

  StdVectorFst fst(*lm_fst_);
  fst.SetFinal(1, Weight(1.0));
  TestPhiMatcher matcher2(fst, MATCH_INPUT, 0);
  EXPECT_EQ(matcher2.Final(0), Weight(1.231511846));
  EXPECT_EQ(matcher2.Final(1), Weight(1.0));
  EXPECT_EQ(matcher2.Final(2), Weight(5.85650349));
  EXPECT_EQ(matcher2.Final(3), Weight(1.356674969));
  EXPECT_EQ(matcher2.Final(4), Weight::One());

  fst.SetFinal(1, Arc::Weight::Zero());
  fst.AddArc(1, Arc(0, 0, Weight(0.5), 1));
  ArcSort(&fst, StdILabelCompare());
  TestPhiMatcher matcher3(fst, MATCH_INPUT, 0);
  for (auto s = 0; s < lm_fst_->NumStates(); ++s) {
    EXPECT_EQ(matcher3.Final(s), lm_fst_->Final(s));
  }
}

TEST_F(MatcherTest, SortedMatcher) {
  SortedMatcher<VectorFst<Arc>> sorted_matcher(*mfst2_, MATCH_INPUT);
  sorted_matcher.SetState(0);

  sorted_matcher.LowerBound(0);
  ASSERT_TRUE(!sorted_matcher.Done());
  ASSERT_EQ(sorted_matcher.Value().ilabel, 1);
  ASSERT_EQ(sorted_matcher.Position(), 0);

  sorted_matcher.LowerBound(1);
  ASSERT_TRUE(!sorted_matcher.Done());
  ASSERT_EQ(sorted_matcher.Value().ilabel, 1);
  ASSERT_EQ(sorted_matcher.Position(), 0);

  sorted_matcher.LowerBound(2);
  ASSERT_TRUE(!sorted_matcher.Done());
  ASSERT_EQ(sorted_matcher.Value().ilabel, 2);
  ASSERT_EQ(sorted_matcher.Position(), 1);

  sorted_matcher.LowerBound(3);
  ASSERT_TRUE(sorted_matcher.Done());
  ASSERT_EQ(sorted_matcher.Position(), 2);

  for (auto s = 0; s < mfst2_->NumStates(); ++s) {
    ASSERT_EQ(sorted_matcher.Final(s), mfst2_->Final(s));
  }
}

TEST_F(MatcherTest, IterativeSortedMatcherTest) {
  IterativeTest<SortedMatcher<VectorFst<Arc>>>(*lm_fst_, /*count=*/2);
}

TEST_F(MatcherTest, HashMatcher) {
  HashMatcher<VectorFst<Arc>> hash_matcher(*mfst5_, MATCH_INPUT);

  hash_matcher.SetState(0);

  // "Searches" for epsilon, which just returns the implicit self-loop.
  ASSERT_TRUE(hash_matcher.Find(0));
  ASSERT_TRUE(!hash_matcher.Done());
  ASSERT_EQ(hash_matcher.Value().ilabel, kNoLabel);
  ASSERT_EQ(hash_matcher.Value().olabel, 0);
  hash_matcher.Next();
  ASSERT_TRUE(hash_matcher.Done());

  // Searches for 1, which is (explicitly) present.
  ASSERT_TRUE(hash_matcher.Find(1));
  ASSERT_TRUE(!hash_matcher.Done());
  ASSERT_EQ(hash_matcher.Value().ilabel, 1);
  hash_matcher.Next();
  ASSERT_TRUE(hash_matcher.Done());

  hash_matcher.SetState(4);

  // Searches for 2, of which there are two.
  ASSERT_TRUE(hash_matcher.Find(2));
  ASSERT_TRUE(!hash_matcher.Done());
  ASSERT_EQ(hash_matcher.Value().ilabel, 2);
  hash_matcher.Next();
  ASSERT_TRUE(!hash_matcher.Done());
  ASSERT_EQ(hash_matcher.Value().ilabel, 2);
  hash_matcher.Next();
  ASSERT_TRUE(hash_matcher.Done());

  for (auto s = 0; s < mfst5_->NumStates(); ++s) {
    ASSERT_EQ(hash_matcher.Final(s), mfst5_->Final(s));
  }
}

TEST_F(MatcherTest, IterativeHashMatcherTest) {
  IterativeTest<HashMatcher<VectorFst<Arc>>>(*lm_fst_, /*count=*/2);
}

TEST_F(MatcherTest, ExplicitMatcher) {
  ExplicitMatcher<SortedMatcher<VectorFst<Arc>>> explicit_matcher(*mfst8_,
                                                                  MATCH_INPUT);
  explicit_matcher.SetState(0);

  ASSERT_FALSE(explicit_matcher.Find(0));
  ASSERT_TRUE(explicit_matcher.Done());

  ASSERT_TRUE(explicit_matcher.Find(1));
  ASSERT_FALSE(explicit_matcher.Done());
  ASSERT_EQ(explicit_matcher.Value().ilabel, 1);
  explicit_matcher.Next();
  ASSERT_TRUE(explicit_matcher.Done());

  explicit_matcher.SetState(1);

  ASSERT_TRUE(explicit_matcher.Find(0));
  ASSERT_FALSE(explicit_matcher.Done());
  ASSERT_EQ(explicit_matcher.Value().ilabel, 0);
  explicit_matcher.Next();
  ASSERT_TRUE(explicit_matcher.Done());

  ASSERT_FALSE(explicit_matcher.Find(1));
  ASSERT_TRUE(explicit_matcher.Done());

  for (auto s = 0; s < mfst8_->NumStates(); ++s) {
    ASSERT_EQ(explicit_matcher.Final(s), mfst8_->Final(s));
  }
}

}  // namespace
}  // namespace fst
