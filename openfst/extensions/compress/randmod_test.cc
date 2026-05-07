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

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "absl/flags/flag.h"
#include "absl/log/flags.h"
#include "absl/log/log.h"
#include "openfst/compat/seed_sequences.h"
#include "absl/random/random.h"
#include "openfst/compat/seed_sequences.h"
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

// Testing if the output after decode and encode is isomorphic to
// the original unweighted fst
TEST(RandModTest, UnweightedRandMod) {
  absl::BitGen bit_gen(fst::MakeTaggedSeedSeq(
      "UNWEIGHTED_RAND_MOD"));

  int num_states =
      absl::Uniform<int>(absl::IntervalClosedClosed, bit_gen, 1, 100);
  int num_classes =
      absl::Uniform<int>(absl::IntervalClosedClosed, bit_gen, 1, 5);
  int num_labels =
      absl::Uniform<int>(absl::IntervalClosedClosed, bit_gen, 1, 10);
  using TropicalWeightGenerate = WeightGenerate<TropicalWeight>;
  RandMod<StdArc, TropicalWeightGenerate> rand_mod(
      num_states, num_classes, num_labels, false, nullptr, bit_gen);

  const std::string unweighted_output =
      JoinPath(::testing::TempDir(), "unweight_output.fstz");

  StdVectorFst input_fst;
  StdVectorFst output_fst;
  StdVectorFst input_dfa;
  StdVectorFst output_dfa;
  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    rand_mod.Generate(&input_fst, bit_gen);
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
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
