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

#include <cmath>

#include "gtest/gtest.h"
#include "openfst/extensions/linear/loglinear-apply.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/vector-fst.h"

namespace fst {
namespace {

using Arc = StdArc;
using Weight = Arc::Weight;

constexpr float kDelta = 1e-4f;

TEST(LogLinearApplyTest, UnnormalizedApply) {
  VectorFst<Arc> ifst;
  const auto s0 = ifst.AddState();
  const auto s1 = ifst.AddState();
  ifst.SetStart(s0);
  ifst.SetFinal(s1, Weight::One());
  ifst.AddArc(s0, Arc(1, 2, Weight(1.0f), s1));

  VectorFst<Arc> lfst;
  const auto l0 = lfst.AddState();
  const auto l1 = lfst.AddState();
  lfst.SetStart(l0);
  lfst.SetFinal(l1, Weight::One());
  lfst.AddArc(l0, Arc(2, 3, Weight(2.0f), l1));

  VectorFst<Arc> ofst;
  LogLinearApply(ifst, lfst, &ofst, /*normalize=*/false);

  EXPECT_EQ(ofst.Start(), 0);
  // Total weight should be 1.0 + 2.0 = 3.0.
  bool found = false;
  for (ArcIterator<VectorFst<Arc>> aiter(ofst, ofst.Start()); !aiter.Done();
       aiter.Next()) {
    const auto& arc = aiter.Value();
    if (arc.ilabel == 1 && arc.olabel == 3) {
      EXPECT_NEAR(arc.weight.Value(), 3.0f, kDelta);
      found = true;
    }
  }
  EXPECT_TRUE(found);
}

TEST(LogLinearApplyTest, NormalizedApply) {
  VectorFst<Arc> ifst;
  const auto s0 = ifst.AddState();
  const auto s1 = ifst.AddState();
  ifst.SetStart(s0);
  ifst.SetFinal(s1, Weight::One());
  ifst.AddArc(s0, Arc(1, 1, Weight(0.0f), s1));

  VectorFst<Arc> lfst;
  const auto l0 = lfst.AddState();
  const auto l1 = lfst.AddState();
  lfst.SetStart(l0);
  lfst.SetFinal(l1, Weight::One());
  lfst.AddArc(l0, Arc(1, 3, Weight(0.0f), l1));
  lfst.AddArc(l0, Arc(1, 4, Weight(0.0f), l1));

  VectorFst<Arc> ofst;
  LogLinearApply(ifst, lfst, &ofst, /*normalize=*/true);

  ASSERT_NE(ofst.Start(), kNoStateId);

  // Each normalized path should have weight log(2) ~= 0.693147.
  const float expected_weight = std::log(2.0f);
  int count = 0;
  for (ArcIterator<VectorFst<Arc>> aiter(ofst, ofst.Start()); !aiter.Done();
       aiter.Next()) {
    const auto& arc = aiter.Value();
    EXPECT_NEAR(arc.weight.Value(), expected_weight, kDelta);
    ++count;
  }
  EXPECT_EQ(count, 2);
}

TEST(LogLinearApplyTest, EmptyLattice) {
  VectorFst<Arc> ifst;
  ifst.AddState();
  ifst.SetStart(0);
  // No final state / arcs.

  VectorFst<Arc> lfst;
  lfst.AddState();
  lfst.SetStart(0);
  lfst.SetFinal(0, Weight::One());

  VectorFst<Arc> ofst;
  LogLinearApply(ifst, lfst, &ofst, /*normalize=*/true);
  EXPECT_EQ(ofst.Start(), kNoStateId);
}

}  // namespace
}  // namespace fst
