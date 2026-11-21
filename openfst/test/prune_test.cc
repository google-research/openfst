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
// Unit test for Prune.

#include "openfst/lib/prune.h"

#include <memory>
#include <string>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/equal.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/shortest-distance.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/lib/verify.h"
#include "openfst/script/equal.h"
#include "openfst/script/fst-class.h"
#include "openfst/script/prune.h"
#include "openfst/script/weight-class.h"

namespace fst {
namespace {

using Arc = StdArc;
using Weight = Arc::Weight;

class PruneTest : public testing::Test {
 protected:
  void SetUp() override {
    const std::string path = JoinPath(
        std::string("."), "openfst/test/testdata/prune");
    const std::string prune1_name = JoinPath(path, "p1.fst");
    const std::string prune2_name = JoinPath(path, "p2.fst");

    pfst1_.reset(VectorFst<Arc>::Read(prune1_name));
    // pfst2_ = Prune(pfst1_)
    pfst2_.reset(VectorFst<Arc>::Read(prune2_name));
  }

  std::unique_ptr<const VectorFst<Arc>> pfst1_;
  std::unique_ptr<const VectorFst<Arc>> pfst2_;
};

TEST_F(PruneTest, DestructivePrune) {
  VectorFst<Arc> nfst;

  VectorFst<Arc> vfst(*pfst1_);

  Prune(&vfst, Weight(0.5));
  ASSERT_TRUE(Verify(vfst));
  ASSERT_TRUE(Equal(*pfst2_, vfst));

  Prune(&vfst, Weight(-1.0));
  ASSERT_TRUE(Verify(vfst));
  ASSERT_TRUE(Equal(nfst, vfst));

  Prune(&nfst, Weight(0.5));
  ASSERT_TRUE(Verify(vfst));

  Prune(&nfst, Weight(-1.0));
  ASSERT_TRUE(Verify(vfst));

  vfst = *pfst1_;
  Prune(&vfst, Weight::One());
  ASSERT_TRUE(Verify(vfst));
  ASSERT_TRUE(Equal(*pfst2_, vfst));

  vfst = *pfst1_;
  Prune(&vfst, Weight::Zero());
  ASSERT_TRUE(Verify(vfst));
  ASSERT_TRUE(Equal(*pfst1_, vfst));

  Prune(&vfst, Weight::Zero(), vfst.NumStates());
  ASSERT_TRUE(Verify(vfst));
  ASSERT_TRUE(Equal(*pfst1_, vfst));

  Prune(&vfst, Weight::Zero(), 3);
  ASSERT_TRUE(Verify(vfst));
  ASSERT_EQ(vfst.NumStates(), 3);

  vfst = *pfst1_;
  Prune(&vfst, Weight(0.5), 3);
  ASSERT_TRUE(Verify(vfst));
  ASSERT_TRUE(Equal(*pfst2_, vfst));
}

TEST_F(PruneTest, ConstructivePrune) {
  VectorFst<Arc> nfst;

  VectorFst<Arc> vfst;

  Prune(*pfst1_, &vfst, Weight::One());
  ASSERT_TRUE(Verify(vfst));
  ASSERT_TRUE(Equal(*pfst2_, vfst));

  Prune(*pfst1_, &vfst, Weight(0.5));
  ASSERT_TRUE(Verify(vfst));
  ASSERT_TRUE(Equal(*pfst2_, vfst));

  Prune(*pfst1_, &vfst, Weight(0.5), 3);
  ASSERT_TRUE(Verify(vfst));
  ASSERT_TRUE(Equal(*pfst2_, vfst));

  Prune(*pfst1_, &vfst, Weight(pfst1_->NumStates()));
  ASSERT_TRUE(Verify(vfst));
  ASSERT_EQ(pfst1_->NumStates(), vfst.NumStates());

  Prune(*pfst1_, &vfst, Weight(0.5), 0);
  ASSERT_TRUE(Verify(vfst));
  ASSERT_TRUE(Equal(nfst, vfst));

  Prune(*pfst1_, &vfst, Weight(-1.0));
  ASSERT_TRUE(Verify(vfst));
  ASSERT_TRUE(Equal(nfst, vfst));

  Prune(nfst, &vfst, Weight(0.5));
  ASSERT_TRUE(Verify(vfst));
  ASSERT_TRUE(Equal(nfst, vfst));

  Prune(nfst, &vfst, Weight(-1.0));
  ASSERT_TRUE(Verify(vfst));
  ASSERT_TRUE(Equal(nfst, vfst));
}

TEST_F(PruneTest, FstClassPrune) {
  namespace s = fst::script;

  s::VectorFstClass pfst1(*pfst1_);
  s::VectorFstClass pfst2(*pfst2_);
  s::VectorFstClass vfst(pfst1.ArcType());
  const s::WeightClass threshold(s::WeightClass::One(vfst.WeightType()));
  Prune(pfst1, &vfst, threshold);
  ASSERT_TRUE(Equal(pfst2, vfst));
}

// Builds a single-path FST where backward ShortestDistance computes
// -1.0f + (4e-8f + 4e-8f) = -0.99999994f, while forward accumulation in Prune
// computes (-1.0f + 4e-8f) + 4e-8f = -0.99999988f (1 ULP larger) on both arc
// 1 -> 2 and state 2's final weight.
VectorFst<Arc> MakeFloatNonAssociativePathFst() {
  VectorFst<Arc> fst;
  fst.AddStates(3);
  fst.SetStart(0);
  fst.AddArc(0, Arc(1, 1, Weight(-1.0f), 1));
  fst.AddArc(1, Arc(2, 2, Weight(4e-8f), 2));
  fst.SetFinal(2, Weight(4e-8f));
  return fst;
}

// Verifies that destructive and constructive Prune keep an optimal path whose
// forward weight rounds up within delta, while delta = 0.0f enforces exact
// comparison.
TEST(PruneToleranceTest, KeepsShortestPathWithinDelta) {
  const VectorFst<Arc> ifst = MakeFloatNonAssociativePathFst();

  VectorFst<Arc> destructive_fst(ifst);
  Prune(&destructive_fst, Weight::One());
  ASSERT_TRUE(Verify(destructive_fst));
  EXPECT_TRUE(Equal(ifst, destructive_fst));

  VectorFst<Arc> constructive_fst;
  Prune(ifst, &constructive_fst, Weight::One());
  ASSERT_TRUE(Verify(constructive_fst));
  EXPECT_TRUE(Equal(ifst, constructive_fst));

  VectorFst<Arc> expected_zero_delta;
  expected_zero_delta.AddStates(2);
  expected_zero_delta.SetStart(0);
  expected_zero_delta.AddArc(0, Arc(1, 1, Weight(-1.0f), 1));

  VectorFst<Arc> destructive_zero_delta(ifst);
  Prune(&destructive_zero_delta, Weight::One(), kNoStateId, /*delta=*/0.0f);
  ASSERT_TRUE(Verify(destructive_zero_delta));
  EXPECT_TRUE(Equal(expected_zero_delta, destructive_zero_delta));
  EXPECT_EQ(ShortestDistance(destructive_zero_delta), Weight::Zero());

  VectorFst<Arc> constructive_zero_delta;
  Prune(ifst, &constructive_zero_delta, Weight::One(), kNoStateId,
        /*delta=*/0.0f);
  ASSERT_TRUE(Verify(constructive_zero_delta));
  EXPECT_TRUE(Equal(expected_zero_delta, constructive_zero_delta));
  EXPECT_EQ(ShortestDistance(constructive_zero_delta), Weight::Zero());
}

// Verifies that a negative weight_threshold (even smaller in magnitude than
// kDelta) or an FST with no start state produces an empty FST.
TEST(PruneToleranceTest, NegativeThresholdOrUnsetStartClearsFst) {
  const VectorFst<Arc> ifst = MakeFloatNonAssociativePathFst();

  VectorFst<Arc> destructive_fst(ifst);
  Prune(&destructive_fst, Weight(-1e-5f));
  ASSERT_TRUE(Verify(destructive_fst));
  EXPECT_EQ(destructive_fst.NumStates(), 0);

  VectorFst<Arc> constructive_fst;
  Prune(ifst, &constructive_fst, Weight(-1e-5f));
  ASSERT_TRUE(Verify(constructive_fst));
  EXPECT_EQ(constructive_fst.NumStates(), 0);

  VectorFst<Arc> no_start_fst;
  no_start_fst.AddStates(2);
  Prune(&no_start_fst, Weight::One());
  ASSERT_TRUE(Verify(no_start_fst));
  EXPECT_EQ(no_start_fst.NumStates(), 0);
}

}  // namespace
}  // namespace fst
