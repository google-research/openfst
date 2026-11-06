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
// Unit test for Push.

#include "openfst/lib/push.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/equal.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/properties.h"
#include "openfst/lib/reweight.h"
#include "openfst/lib/shortest-distance.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/lib/verify.h"
#include "openfst/script/equal.h"
#include "openfst/script/fst-class.h"
#include "openfst/script/push.h"

namespace fst {
namespace {

using Arc = StdArc;

class PushTest : public testing::Test {
 protected:
  void SetUp() override {
    const std::string path = JoinPath(
        std::string("."), "openfst/test/testdata/push");
    const std::string push1_name = JoinPath(path, "p1.fst");
    const std::string push2_name = JoinPath(path, "p2.fst");
    const std::string push3_name = JoinPath(path, "p3.fst");
    const std::string push4_name = JoinPath(path, "p4.fst");
    const std::string push5_name = JoinPath(path, "p5.fst");

    pfst1_.reset(VectorFst<Arc>::Read(push1_name));
    pfst2_.reset(VectorFst<Arc>::Read(push2_name));
    pfst3_.reset(VectorFst<Arc>::Read(push3_name));
    pfst4_.reset(VectorFst<Arc>::Read(push4_name));
    pfst5_.reset(VectorFst<Arc>::Read(push5_name));
  }

  std::unique_ptr<const VectorFst<Arc>> pfst1_;
  std::unique_ptr<const VectorFst<Arc>> pfst2_;
  std::unique_ptr<const VectorFst<Arc>> pfst3_;
  std::unique_ptr<const VectorFst<Arc>> pfst4_;
  std::unique_ptr<const VectorFst<Arc>> pfst5_;
};

TEST_F(PushTest, PushWeight) {
  using fst::script::Push;
  using fst::script::VectorFstClass;

  VectorFstClass pfst2(*pfst2_);
  VectorFstClass pfst3(*pfst3_);

  VectorFstClass vfst1(*pfst1_);
  VectorFstClass vfst2(*pfst1_);

  Push(&vfst1, REWEIGHT_TO_INITIAL);
  ASSERT_TRUE(Equal(pfst2, vfst1));

  Push(&vfst2, REWEIGHT_TO_FINAL);
  ASSERT_TRUE(Equal(pfst3, vfst2));
}

// FstClass test - testing script-level operations
TEST_F(PushTest, PushWeightFstClass) {
  // Most of the Push features in script-land are tested in the bin-test.
  // This just makes sure that the other signature for push is working
  // correctly.

  VectorFst<Arc> nfst1;
  VectorFst<Arc> nfst2;

  VectorFst<Arc> vfst1(*pfst1_);
  VectorFst<Arc> vfst2(*pfst1_);

  Push(&vfst1, REWEIGHT_TO_INITIAL);
  ASSERT_TRUE(Verify(vfst1));
  ASSERT_TRUE(Equal(*pfst2_, vfst1));

  Push(&vfst2, REWEIGHT_TO_FINAL);
  ASSERT_TRUE(Verify(vfst2));
  ASSERT_TRUE(Equal(*pfst3_, vfst2));

  Push(&nfst1, REWEIGHT_TO_INITIAL);
  ASSERT_TRUE(Verify(nfst1));

  Push(&nfst2, REWEIGHT_TO_FINAL);
  ASSERT_TRUE(Verify(nfst2));
}

TEST_F(PushTest, PushLabel) {
  VectorFst<Arc> nfst;

  VectorFst<Arc> nfst1;
  VectorFst<Arc> nfst2;
  VectorFst<Arc> vfst1;
  VectorFst<Arc> vfst2;

  Push<Arc, REWEIGHT_TO_INITIAL>(*pfst1_, &vfst1, kPushLabels);
  ASSERT_TRUE(Verify(vfst1));
  ASSERT_TRUE(Equal(*pfst4_, vfst1));

  Push<Arc, REWEIGHT_TO_FINAL>(*pfst1_, &vfst2, kPushLabels);
  ASSERT_TRUE(Verify(vfst2));
  ASSERT_TRUE(Equal(*pfst5_, vfst2));

  Push<Arc, REWEIGHT_TO_INITIAL>(nfst, &nfst1, kPushLabels);
  Push(&nfst1, REWEIGHT_TO_INITIAL);
  ASSERT_TRUE(Verify(nfst1));

  Push<Arc, REWEIGHT_TO_FINAL>(nfst, &nfst2, kPushLabels);
  ASSERT_TRUE(Verify(nfst2));
}

constexpr uint8_t kLabelRemovalFlagMasks[] = {
    kPushLabels | kPushRemoveCommonAffix,
    kPushLabels | kPushRemoveTotalWeight,
    kPushLabels | kPushRemoveCommonAffix | kPushRemoveTotalWeight,
    kPushWeights | kPushLabels | kPushRemoveCommonAffix,
    kPushWeights | kPushLabels | kPushRemoveTotalWeight,
    kPushWeights | kPushLabels | kPushRemoveCommonAffix |
        kPushRemoveTotalWeight,
};

TEST_F(PushTest, PushLabelEmptyFstWithRemovalFlags) {
  VectorFst<Arc> empty_fst;
  for (uint8_t ptype : kLabelRemovalFlagMasks) {
    SCOPED_TRACE(static_cast<int>(ptype));
    VectorFst<Arc> ofst_initial;
    Push<Arc, REWEIGHT_TO_INITIAL>(empty_fst, &ofst_initial, ptype);
    EXPECT_TRUE(Verify(ofst_initial));
    EXPECT_EQ(ofst_initial.Start(), kNoStateId);
    EXPECT_EQ(ofst_initial.NumStates(), 0);

    VectorFst<Arc> ofst_final;
    Push<Arc, REWEIGHT_TO_FINAL>(empty_fst, &ofst_final, ptype);
    EXPECT_TRUE(Verify(ofst_final));
    EXPECT_EQ(ofst_final.Start(), kNoStateId);
    EXPECT_EQ(ofst_final.NumStates(), 0);
  }
}

TEST_F(PushTest, PushLabelNoAcceptingPathWithRemovalFlags) {
  // Dead-end FST with no final states.
  VectorFst<Arc> no_final_fst;
  const auto s0 = no_final_fst.AddState();
  const auto s1 = no_final_fst.AddState();
  no_final_fst.SetStart(s0);
  no_final_fst.AddArc(s0, Arc(1, 2, Arc::Weight(3.0), s1));

  // FST whose only final state is unreachable from Start().
  VectorFst<Arc> unreachable_final_fst(no_final_fst);
  const auto s2 = unreachable_final_fst.AddState();
  unreachable_final_fst.SetFinal(s2, Arc::Weight(5.0));

  for (const VectorFst<Arc>* ifst : {&no_final_fst, &unreachable_final_fst}) {
    for (uint8_t ptype : kLabelRemovalFlagMasks) {
      SCOPED_TRACE(static_cast<int>(ptype));
      const uint8_t base_ptype = ptype & (kPushWeights | kPushLabels);

      VectorFst<Arc> expected_initial;
      Push<Arc, REWEIGHT_TO_INITIAL>(*ifst, &expected_initial, base_ptype);
      VectorFst<Arc> ofst_initial;
      Push<Arc, REWEIGHT_TO_INITIAL>(*ifst, &ofst_initial, ptype);
      EXPECT_TRUE(Verify(ofst_initial));
      EXPECT_TRUE(Equal(expected_initial, ofst_initial));

      VectorFst<Arc> expected_final;
      Push<Arc, REWEIGHT_TO_FINAL>(*ifst, &expected_final, base_ptype);
      VectorFst<Arc> ofst_final;
      Push<Arc, REWEIGHT_TO_FINAL>(*ifst, &ofst_final, ptype);
      EXPECT_TRUE(Verify(ofst_final));
      EXPECT_TRUE(Equal(expected_final, ofst_final));
    }
  }
}

TEST_F(PushTest, PushLabelCancellingWeightsWithRemovalFlags) {
  using SLogWeight = SignedLogArc::Weight;
  // Two accepting paths with identical output label 5 whose SignedLogWeights
  // (+1, 0) and (-1, 0) cancel to Zero(), so ComputeTotalWeight has a non-Zero
  // common affix (5) paired with a Zero scalar weight sum.
  VectorFst<SignedLogArc> ifst;
  const auto s0 = ifst.AddState();
  const auto s1 = ifst.AddState();
  const auto s2 = ifst.AddState();
  ifst.SetStart(s0);
  const SLogWeight pos_one(SLogWeight::W1(1.0), SLogWeight::W2::One());
  const SLogWeight neg_one(SLogWeight::W1(-1.0), SLogWeight::W2::One());
  ifst.AddArc(s0, SignedLogArc(1, 5, pos_one, s1));
  ifst.AddArc(s0, SignedLogArc(2, 5, pos_one, s2));
  ifst.SetFinal(s1, pos_one);
  ifst.SetFinal(s2, neg_one);

  for (uint8_t base_ptype :
       {static_cast<uint8_t>(kPushLabels),
        static_cast<uint8_t>(kPushLabels | kPushRemoveCommonAffix),
        static_cast<uint8_t>(kPushWeights | kPushLabels),
        static_cast<uint8_t>(kPushWeights | kPushLabels |
                             kPushRemoveCommonAffix)}) {
    SCOPED_TRACE(static_cast<int>(base_ptype));
    VectorFst<SignedLogArc> expected;
    Push<SignedLogArc, REWEIGHT_TO_FINAL>(ifst, &expected, base_ptype);

    VectorFst<SignedLogArc> ofst;
    Push<SignedLogArc, REWEIGHT_TO_FINAL>(ifst, &ofst,
                                          base_ptype | kPushRemoveTotalWeight);
    EXPECT_TRUE(Verify(ofst));
    EXPECT_TRUE(Equal(expected, ofst));
  }
}

TEST_F(PushTest, RemoveWeightAndComputeTotalWeightWithoutStartState) {
  const std::vector<Arc::Weight> distance = {Arc::Weight(2.0)};
  VectorFst<Arc> empty_fst;
  EXPECT_EQ(ComputeTotalWeight(empty_fst, distance, /*reverse=*/false),
            Arc::Weight::Zero());
  EXPECT_EQ(ComputeTotalWeight(empty_fst, distance, /*reverse=*/true),
            Arc::Weight::Zero());
  RemoveWeight(&empty_fst, Arc::Weight(2.0), /*at_final=*/false);
  EXPECT_TRUE(Verify(empty_fst));
  EXPECT_EQ(empty_fst.NumStates(), 0);
  RemoveWeight(&empty_fst, Arc::Weight(2.0), /*at_final=*/true);
  EXPECT_TRUE(Verify(empty_fst));
  EXPECT_EQ(empty_fst.NumStates(), 0);

  // Pushing an empty FST with kError set under REWEIGHT_TO_FINAL and total
  // weight removal causes ShortestDistance to populate a 1-element NoWeight()
  // distance vector; ComputeTotalWeight must not index state 0 out of bounds.
  VectorFst<Arc> empty_error_fst;
  empty_error_fst.SetProperties(kError, kError);
  Push(&empty_error_fst, REWEIGHT_TO_FINAL, kShortestDelta,
       /*remove_total_weight=*/true);
  EXPECT_EQ(empty_error_fst.NumStates(), 0);

  // Non-empty FST with no start state (Start() == kNoStateId) has no accepting
  // paths, so ComputeTotalWeight should return Zero() and RemoveWeight should
  // leave states and weights untouched for both directions.
  VectorFst<Arc> startless_fst;
  const auto s0 = startless_fst.AddState();
  startless_fst.SetFinal(s0, Arc::Weight(3.0));
  const VectorFst<Arc> orig(startless_fst);

  EXPECT_EQ(ComputeTotalWeight(startless_fst, distance, /*reverse=*/false),
            Arc::Weight::Zero());
  EXPECT_EQ(ComputeTotalWeight(startless_fst, distance, /*reverse=*/true),
            Arc::Weight::Zero());
  RemoveWeight(&startless_fst, Arc::Weight(2.0), /*at_final=*/false);
  EXPECT_TRUE(Equal(orig, startless_fst));
  RemoveWeight(&startless_fst, Arc::Weight(2.0), /*at_final=*/true);
  EXPECT_TRUE(Equal(orig, startless_fst));

  // Negative Start() other than kNoStateId (-1) should also be treated as
  // having no valid start state rather than indexing out of bounds.
  startless_fst.SetStart(-2);
  EXPECT_EQ(ComputeTotalWeight(startless_fst, distance, /*reverse=*/false),
            Arc::Weight::Zero());
  EXPECT_EQ(ComputeTotalWeight(startless_fst, distance, /*reverse=*/true),
            Arc::Weight::Zero());
  RemoveWeight(&startless_fst, Arc::Weight(2.0), /*at_final=*/false);
  EXPECT_EQ(startless_fst.Final(s0), Arc::Weight(3.0));
  RemoveWeight(&startless_fst, Arc::Weight(2.0), /*at_final=*/true);
  EXPECT_EQ(startless_fst.Final(s0), Arc::Weight(3.0));
}

}  // namespace
}  // namespace fst
