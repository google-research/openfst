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
// Regression test for categorial weights.

#include "openfst/lib/weight.h"

#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <ostream>

#include "gtest/gtest.h"
#include "absl/flags/flag.h"
#include "absl/log/flags.h"
#include "absl/log/log.h"
#include "openfst/extensions/categorial/categorial-weight.h"
#include "openfst/test/weight-tester.h"

ABSL_FLAG(uint64_t, seed, 403, "Random seed.");
ABSL_FLAG(int32_t, repeat, 10000, "Number of test repetitions.");

namespace fst {

template <>
struct WeightTestTraits<CategorialWeight<int>> {
  static WeightGenerate<CategorialWeight<int>> Generator(uint64_t seed) {
    return WeightGenerate<CategorialWeight<int>>();
  }
  static bool IoRequiresParens() { return false; }
};

template <>
struct WeightTestTraits<CategorialWeight<int, CategoryType::RIGHT>> {
  static WeightGenerate<CategorialWeight<int, CategoryType::RIGHT>> Generator(
      uint64_t seed) {
    return WeightGenerate<CategorialWeight<int, CategoryType::RIGHT>>();
  }
  static bool IoRequiresParens() { return false; }
};

namespace {

using CategorialWeightTypes =
    ::testing::Types<CategorialWeight<int>,
                     CategorialWeight<int, CategoryType::RIGHT>>;

INSTANTIATE_TYPED_TEST_SUITE_P(Categorial, WeightTest, CategorialWeightTypes, );

}  // namespace
}  // namespace fst

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  int seed = absl::GetFlag(FLAGS_seed);
  std::srand(seed);
  LOG(INFO) << "Seed = " << absl::GetFlag(FLAGS_seed);
  return RUN_ALL_TESTS();
}
