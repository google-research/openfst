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

#include <cstring>
#include <string>

#include "absl/flags/declare.h"
#include "absl/flags/flag.h"
#include "absl/log/flags.h"
#include "absl/log/log.h"
#include "third_party/gloop/base/commandlineflags.h"
#include "third_party/gloop/base/init_google.h"
#include "openfst/extensions/linear/loglinear-apply.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/vector-fst.h"

ABSL_DECLARE_FLAG(bool, normalize);

int fstloglinearapply_main(int argc, char** argv) {
  std::string usage =
      "Applies an FST to another FST, treating the second as a log-linear "
      "model.\n\n  "
      "Usage: ";
  usage += argv[0];
  usage += " in.fst linear.fst [out.fst]\n";

  fst::InitOpenFst(usage.c_str(), &argc, &argv, true);
  if (argc < 3 || argc > 4) {
    LOG(INFO) << absl::ProgramUsageMessage();
    return 1;
  }

  std::string in_name = strcmp(argv[1], "-") != 0 ? argv[1] : "";
  std::string linear_name =
      (argc > 2 && (strcmp(argv[2], "-") != 0)) ? argv[2] : "";
  std::string out_name =
      (argc > 3 && (strcmp(argv[3], "-") != 0)) ? argv[3] : "";

  if (in_name.empty() && linear_name.empty()) {
    LOG(ERROR) << argv[0] << ": Can't take both inputs from standard input.";
    return 1;
  }

  fst::StdFst* ifst1 = fst::StdFst::Read(in_name);
  if (!ifst1) return 1;

  fst::StdFst* ifst2 = fst::StdFst::Read(linear_name);
  if (!ifst2) return 1;

  fst::StdVectorFst ofst;

  fst::LogLinearApply(*ifst1, *ifst2, &ofst, absl::GetFlag(FLAGS_normalize));

  return !ofst.Write(out_name);
}
