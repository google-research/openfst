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
// Two DFAs are equivalent iff their exit status is zero.

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
#include "openfst/script/equivalent.h"
#include "openfst/script/fst-class.h"
#include "openfst/script/getters.h"
#include "openfst/script/randequivalent.h"
#include "openfst/script/script-impl.h"

ABSL_DECLARE_FLAG(double, delta);
ABSL_DECLARE_FLAG(bool, random);
ABSL_DECLARE_FLAG(int32_t, max_length);
ABSL_DECLARE_FLAG(int32_t, npath);
ABSL_DECLARE_FLAG(std::optional<uint64_t>, seed);
ABSL_DECLARE_FLAG(std::string, select);

int fstequivalent_main(int argc, char** argv) {
  namespace s = fst::script;
  using fst::RandGenOptions;
  using fst::script::FstClass;

  std::string usage =
      "Two DFAs are equivalent iff the exit status is zero.\n\n"
      "  Usage: ";
  usage += argv[0];
  usage += " in1.fst in2.fst\n";

  fst::InitOpenFst(usage.c_str(), &argc, &argv, true);
  if (argc != 3) {
    LOG(INFO) << absl::ProgramUsageMessage();
    return 1;
  }

  const std::string in1_name = strcmp(argv[1], "-") == 0 ? "" : argv[1];
  const std::string in2_name = strcmp(argv[2], "-") == 0 ? "" : argv[2];

  if (in1_name.empty() && in2_name.empty()) {
    LOG(ERROR) << argv[0] << ": Can't take both inputs from standard input";
    return 1;
  }

  std::unique_ptr<FstClass> ifst1(FstClass::Read(in1_name));
  if (!ifst1) return 1;

  std::unique_ptr<FstClass> ifst2(FstClass::Read(in2_name));
  if (!ifst2) return 1;

  bool result;
  if (absl::GetFlag(FLAGS_random)) {
    s::RandArcSelection ras;
    if (!s::GetRandArcSelection(absl::GetFlag(FLAGS_select), &ras)) {
      LOG(ERROR) << argv[0] << ": Unknown or unsupported select type "
                 << absl::GetFlag(FLAGS_select);
      return 1;
    }
    const RandGenOptions<s::RandArcSelection> opts(
        ras, absl::GetFlag(FLAGS_max_length));
    result = s::RandEquivalent(*ifst1, *ifst2, absl::GetFlag(FLAGS_npath), opts,
                               absl::GetFlag(FLAGS_delta),
                               absl::GetFlag(FLAGS_seed));
  } else {
    result = s::Equivalent(*ifst1, *ifst2, absl::GetFlag(FLAGS_delta));
  }

  if (!result) VLOG(1) << "FSTs are not equivalent";

  return result ? 0 : 2;
}
