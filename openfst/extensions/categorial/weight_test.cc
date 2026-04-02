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

#include <cstdlib>
#include <iostream>
#include <ostream>

#include "gtest/gtest.h"
#include "absl/flags/flag.h"
#include "absl/log/flags.h"
#include "absl/log/log.h"
#include "openfst/extensions/categorial/categorial-weight.h"
#include "openfst/test/weight-tester.h"

ABSL_FLAG(int, seed, -1, "Random seed.");
ABSL_FLAG(int, repeat, 10000, "Number of test repetitions.");

namespace fst {
namespace {

TEST(CategorialWeight, LeftTest) {
  WeightGenerate<CategorialWeight<int>> left_category_generate;
  WeightTester<CategorialWeight<int>> left_category_tester(
      left_category_generate);
  left_category_tester.Test(absl::GetFlag(FLAGS_repeat));
  std::cout << "PASS left categorial test" << std::endl;
}

TEST(CategorialWeight, RightTest) {
  WeightGenerate<CategorialWeight<int, CategoryType::RIGHT>>
      right_category_generate;
  WeightTester<CategorialWeight<int, CategoryType::RIGHT>>
      right_category_tester(right_category_generate);
  right_category_tester.Test(absl::GetFlag(FLAGS_repeat));
  std::cout << "PASS right categorial test" << std::endl;
}

}  // namespace
}  // namespace fst

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  int seed = absl::GetFlag(FLAGS_seed) >= 0 ? absl::GetFlag(FLAGS_seed)
                                            : time(nullptr);
  std::srand(seed);
  LOG(INFO) << "Seed = " << absl::GetFlag(FLAGS_seed);
  return RUN_ALL_TESTS();
}
