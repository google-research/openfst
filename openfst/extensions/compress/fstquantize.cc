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

// Quantizes the weights of an FST according to the specified algorithm.

#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include "absl/flags/flag.h"
#include "absl/log/flags.h"
#include "absl/log/log.h"
#include "third_party/gloop/base/commandlineflags.h"
#include "third_party/gloop/base/init_google.h"
#include "openfst/extensions/compress/quantize_weights.h"
#include "openfst/lib/mutable-fst.h"

ABSL_FLAG(std::string, algorithm, "uniform",
          "Algorithm to quantize the weights of the FST (algorithm, "
          "uniform, kmedian");
// If num_levels = 0, weights are only clipped i.e., they are set to
// max(min_level, min(weight, max_level)).
ABSL_FLAG(int32_t, num_levels, 128, "Number of quantization levels.");
ABSL_FLAG(int32_t, kmedian_num_steps, 100,
          "Number of steps for the kmedian algorithm");
ABSL_FLAG(double, min_level, -10.0, "Minimum value of quantization.");
ABSL_FLAG(double, max_level, 10.0, "Maximum value of quantization.");
ABSL_FLAG(bool, min_level_from_fst, false,
          "If true, use the minimum finite weight value in the FST "
          "for the quantization range lower bound.");
ABSL_FLAG(bool, max_level_from_fst, false,
          "If true, use the maximum finite weight value in the FST "
          "for the quantization range upper bound.");
ABSL_FLAG(bool, quantize_zero_weight, false,
          "If Weight::Zero() should be quantized or kept unchanged");

int main(int argc, char** argv) {
  namespace f = fst;
  std::string usage = "Quantizes the weights.\n\n";
  usage += " Usage: ";
  usage += argv[0];
  usage += " in.fst [out.fst]\n";

  fst::InitOpenFst(usage.c_str(), &argc, &argv, true);
  if (argc > 3 || argc < 2) {
    LOG(INFO) << absl::ProgramUsageMessage();
    return 1;
  }

  const std::string in_name = strcmp(argv[1], "-") != 0 ? argv[1] : "";
  const std::string out_name =
      argc > 2 && strcmp(argv[2], "-") != 0 ? argv[2] : "";

  std::unique_ptr<f::StdMutableFst> fst(f::StdMutableFst::Read(in_name, true));
  if (!fst) return 1;

  std::vector<double> boundaries;
  bool clip_weights_only = false;
  if (absl::GetFlag(FLAGS_num_levels) == 0) {
    boundaries.push_back(absl::GetFlag(FLAGS_min_level));
    boundaries.push_back(absl::GetFlag(FLAGS_max_level));
    clip_weights_only = true;
  } else if (absl::GetFlag(FLAGS_algorithm) == "uniform") {
    double min_level = absl::GetFlag(FLAGS_min_level);
    double max_level = absl::GetFlag(FLAGS_max_level);
    if (absl::GetFlag(FLAGS_min_level_from_fst) ||
        absl::GetFlag(FLAGS_max_level_from_fst)) {
      auto [fst_min_level, fst_max_level] =
          fst::GetMinMaxFiniteWeightValues(*fst);
      if (absl::GetFlag(FLAGS_min_level_from_fst)) min_level = fst_min_level;
      if (absl::GetFlag(FLAGS_max_level_from_fst)) max_level = fst_max_level;
    }
    // FLAGS_min_level_from_fst && FLAGS_min_level_from_fst is the only
    // time min_level > max_level is ok. In this case, it means an empty
    // FST.
    if (!absl::GetFlag(FLAGS_min_level_from_fst) ||
        !absl::GetFlag(FLAGS_max_level_from_fst)) {
      if (min_level > max_level) {
        LOG(ERROR) << "Got min_level > max_level: " << min_level << " > "
                   << max_level;
        return 1;
      }
    }
    if (min_level <= max_level) {
      fst::ComputeUniformQuantization(absl::GetFlag(FLAGS_num_levels),
                                      min_level, max_level, &boundaries);
    }
  } else if (absl::GetFlag(FLAGS_algorithm) == "kmedian") {
    fst::ComputeWeightedkMedianQuantization(
        *fst, absl::GetFlag(FLAGS_num_levels), &boundaries,
        absl::GetFlag(FLAGS_kmedian_num_steps));
  } else {
    LOG(ERROR) << "Algorithm: " << absl::GetFlag(FLAGS_algorithm)
               << " not implemented.";
    return 1;
  }
  if (!boundaries.empty()) {
    fst::QuantizeWeights(boundaries, fst.get(),
                         absl::GetFlag(FLAGS_quantize_zero_weight),
                         clip_weights_only);
  }

  return !fst->Write(out_name);
}
