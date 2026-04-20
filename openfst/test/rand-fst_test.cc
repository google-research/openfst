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

#include "openfst/test/rand-fst.h"

#include "gtest/gtest.h"
#include "absl/status/status_matchers.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/float-weight.h"
#include "openfst/lib/properties.h"
#include "openfst/lib/vector-fst.h"

namespace fst {
namespace {

using Generate = WeightGenerate<TropicalWeight>;

TEST(RandFstTest, AcyclicProb1) {
  for (int i = 0; i < 100; ++i) {
    VectorFst<StdArc> fst;
    Generate generate(/*seed=*/i, /*generate_tropical=*/false);
    ABSL_EXPECT_OK(RandFst(/*num_random_states=*/10, /*num_random_arcs=*/20,
                           /*num_random_labels=*/5, /*acyclic_prob=*/1.0,
                           generate, /*seed=*/i, &fst));
    EXPECT_TRUE(fst.Properties(kAcyclic, true) & kAcyclic);
  }
}

TEST(RandFstTest, AcyclicProb0) {
  int num_cyclic = 0;
  for (int i = 0; i < 100; ++i) {
    VectorFst<StdArc> fst;
    Generate generate(/*seed=*/i, /*generate_tropical=*/false);
    ABSL_EXPECT_OK(RandFst(/*num_random_states=*/10, /*num_random_arcs=*/20,
                           /*num_random_labels=*/5, /*acyclic_prob=*/0.0,
                           generate, /*seed=*/i, &fst));
    if (!(fst.Properties(kAcyclic, true) & kAcyclic)) {
      num_cyclic++;
    }
  }
  // With acyclic_prob 0.0, we expect mostly cyclic FSTs (about 75% of them).
  // Sometimes, we happen to generate an acyclic FST anyway.
  EXPECT_GT(num_cyclic, 0);
}

}  // namespace
}  // namespace fst
