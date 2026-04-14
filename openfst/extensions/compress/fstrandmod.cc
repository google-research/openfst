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
// Generates a random FST according to a class-specific transition model.

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <memory>
#include <string>

#include "absl/flags/usage.h"
#include "openfst/compat/init.h"
#include "absl/flags/flag.h"
#include "absl/log/flags.h"
#include "absl/random/bit_gen_ref.h"
#include "absl/random/random.h"
#include "openfst/extensions/compress/randmod.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/float-weight.h"
#include "openfst/lib/fst-decl.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/lib/weight.h"

ABSL_FLAG(int32_t, states, 10, "# of states");
ABSL_FLAG(int32_t, labels, 2, "# of labels");
ABSL_FLAG(int32_t, classes, 1, "# of probability distributions");
ABSL_FLAG(bool, transducer, false, "Output a transducer");
ABSL_FLAG(bool, weights, false, "Output a weighted FST");

int main(int argc, char** argv) {
  using fst::StdArc;
  using fst::StdVectorFst;
  using fst::TropicalWeight;
  using fst::WeightGenerate;

  std::string usage = "Generates a random FST.\n\n  Usage: ";
  usage += argv[0];
  usage += "[out.fst]\n";

  fst::InitOpenFst(usage.c_str(), &argc, &argv, true);
  if (argc > 2) {
    LOG(INFO) << absl::ProgramUsageMessage();
    return 1;
  }

  std::string out_name =
      (argc > 1 && (strcmp(argv[1], "-") != 0)) ? argv[1] : "";
  absl::BitGen bit_gen;
  int num_states = absl::Uniform(absl::IntervalClosedClosed, bit_gen, 1,
                                 absl::GetFlag(FLAGS_states));
  int num_classes = absl::Uniform(absl::IntervalClosedClosed, bit_gen, 1,
                                  absl::GetFlag(FLAGS_classes));
  int num_labels = absl::Uniform(absl::IntervalClosedClosed, bit_gen, 1,
                                 absl::GetFlag(FLAGS_labels));

  StdVectorFst fst;
  using TropicalWeightGenerate = WeightGenerate<TropicalWeight>;
  std::unique_ptr<TropicalWeightGenerate> generate(
      absl::GetFlag(FLAGS_weights) ? new TropicalWeightGenerate(false)
                                   : nullptr);
  fst::RandMod<StdArc, TropicalWeightGenerate> rand_mod(
      num_states, num_classes, num_labels, absl::GetFlag(FLAGS_transducer),
      generate.get(), bit_gen);
  rand_mod.Generate(&fst, bit_gen);

  return !fst.Write(out_name);
}
