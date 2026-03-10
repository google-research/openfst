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
// Unit test for compression of random FSTs.

#include "openfst/extensions/compress/randmod.h"

#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <memory>
#include <string>

#include "openfst/compat/init.h"
#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "absl/flags/flag.h"
#include "absl/log/flags.h"
#include "absl/log/log.h"
#include "openfst/extensions/compress/compress.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/determinize.h"
#include "openfst/lib/expanded-fst.h"
#include "openfst/lib/float-weight.h"
#include "openfst/lib/fst-decl.h"
#include "openfst/lib/isomorphic.h"
#include "openfst/lib/mutable-fst.h"
#include "openfst/lib/test-properties.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/lib/weight.h"

ABSL_FLAG(int32_t, seed, -1, "Random seed");
ABSL_FLAG(int32_t, repeat, 25, "number of test repetitions");

using ::fst::Isomorphic;
using ::fst::RandMod;
using ::fst::StdArc;
using ::fst::StdMutableFst;
using ::fst::StdVectorFst;
using ::fst::TropicalWeight;
using ::fst::WeightGenerate;

namespace fst {
namespace {

class RandModTest : public testing::Test {
 protected:
  using TropicalWeightGenerate = WeightGenerate<TropicalWeight>;

  void SetUp() override {
    int num_states = (rand() % 100) + 1;
    int num_classes = (rand() % 5) + 1;
    int num_labels = (rand() % 10) + 1;
    rand_mod_ = std::make_unique<RandMod<StdArc, TropicalWeightGenerate>>(
        num_states, num_classes, num_labels, false, nullptr);
  }

  std::unique_ptr<RandMod<StdArc, TropicalWeightGenerate>> rand_mod_;
};

// Testing if the output after decode and encode is isomorphic to
// the original unweighted fst
TEST_F(RandModTest, UnweightedRandMod) {
  const std::string unweighted_output =
      JoinPath(::testing::TempDir(), "unweight_output.fstz");
  StdVectorFst input_fst;
  StdVectorFst output_fst;
  StdVectorFst input_dfa;
  StdVectorFst output_dfa;
  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    rand_mod_->Generate(&input_fst);
    ASSERT_TRUE(Compress(input_fst, unweighted_output));
    ASSERT_TRUE(Decompress(unweighted_output, &output_fst));
    EXPECT_EQ(CountStates(input_fst), CountStates(output_fst));
    EXPECT_EQ(CountArcs(input_fst), CountArcs(output_fst));
    Determinize(input_fst, &input_dfa);
    Determinize(output_fst, &output_dfa);
    EXPECT_TRUE(Isomorphic(input_dfa, output_dfa));
  }
}

}  // namespace
}  // namespace fst

int main(int argc, char** argv) {
  absl::SetFlag(&FLAGS_fst_verify_properties, true);
  fst::InitOpenFst(argv[0], &argc, &argv, true);
  int seed = absl::GetFlag(FLAGS_seed) >= 0 ? absl::GetFlag(FLAGS_seed)
                                            : time(nullptr);
  srand(seed);
  LOG(INFO) << "Seed = " << seed;
  return RUN_ALL_TESTS();
}
