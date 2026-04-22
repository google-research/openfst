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
// Generates random paths through an FST.

#include <cstdint>
#include <cstring>
#include <memory>
#include <optional>
#include <string>

#include "absl/flags/usage.h"
#include "openfst/compat/init.h"
#include "absl/flags/declare.h"
#include "absl/flags/flag.h"
#include "absl/log/flags.h"
#include "absl/log/log.h"
#include "openfst/lib/randgen.h"
#include "openfst/script/fst-class.h"
#include "openfst/script/getters.h"
#include "openfst/script/randgen.h"
#include "openfst/script/script-impl.h"

ABSL_DECLARE_FLAG(int32_t, max_length);
ABSL_DECLARE_FLAG(int32_t, npath);
ABSL_DECLARE_FLAG(std::optional<uint64_t>, seed);
ABSL_DECLARE_FLAG(std::string, select);
ABSL_DECLARE_FLAG(bool, weighted);
ABSL_DECLARE_FLAG(bool, remove_total_weight);

int fstrandgen_main(int argc, char** argv) {
  namespace s = fst::script;
  using fst::RandGenOptions;
  using fst::script::FstClass;
  using fst::script::VectorFstClass;

  std::string usage = "Generates random paths through an FST.\n\n  Usage: ";
  usage += argv[0];
  usage += " [in.fst [out.fst]]\n";

  fst::InitOpenFst(usage.c_str(), &argc, &argv, true);
  if (argc > 3) {
    LOG(INFO) << absl::ProgramUsageMessage();
    return 1;
  }

  const std::string in_name =
      (argc > 1 && strcmp(argv[1], "-") != 0) ? argv[1] : "";
  const std::string out_name =
      (argc > 2 && strcmp(argv[2], "-") != 0) ? argv[2] : "";

  std::unique_ptr<FstClass> ifst(FstClass::Read(in_name));
  if (!ifst) return 1;

  VectorFstClass ofst(ifst->ArcType());

  s::RandArcSelection ras;
  if (!s::GetRandArcSelection(absl::GetFlag(FLAGS_select), &ras)) {
    LOG(ERROR) << argv[0] << ": Unknown or unsupported select type "
               << absl::GetFlag(FLAGS_select);
    return 1;
  }
  s::RandGen(*ifst, &ofst,
             RandGenOptions<s::RandArcSelection>(
                 ras, absl::GetFlag(FLAGS_max_length),
                 absl::GetFlag(FLAGS_npath), absl::GetFlag(FLAGS_weighted),
                 absl::GetFlag(FLAGS_remove_total_weight)),
             absl::GetFlag(FLAGS_seed));

  return !ofst.Write(out_name);
}
