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
// Expands a (bounded-stack) PDT as an FST.

#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "absl/flags/declare.h"
#include "absl/flags/flag.h"
#include "absl/log/flags.h"
#include "absl/log/log.h"
#include "third_party/gloop/base/commandlineflags.h"
#include "third_party/gloop/base/init_google.h"
#include "openfst/extensions/pdt/pdtscript.h"
#include "openfst/lib/util.h"
#include "openfst/script/fst-class.h"
#include "openfst/script/weight-class.h"

ABSL_DECLARE_FLAG(std::string, pdt_parentheses);
ABSL_DECLARE_FLAG(bool, connect);
ABSL_DECLARE_FLAG(bool, keep_parentheses);
ABSL_DECLARE_FLAG(std::string, weight);

int pdtexpand_main(int argc, char** argv) {
  namespace s = fst::script;
  using fst::ReadLabelPairs;
  using fst::script::FstClass;
  using fst::script::VectorFstClass;
  using fst::script::WeightClass;

  std::string usage = "Expand a (bounded-stack) PDT as an FST.\n\n  Usage: ";
  usage += argv[0];
  usage += " in.pdt [out.fst]\n";

  fst::InitOpenFst(usage.c_str(), &argc, &argv, true);
  if (argc > 3) {
    LOG(INFO) << absl::ProgramUsageMessage();
    return 1;
  }

  const std::string in_name =
      (argc > 1 && (strcmp(argv[1], "-") != 0)) ? argv[1] : "";
  const std::string out_name =
      (argc > 2 && (strcmp(argv[2], "-") != 0)) ? argv[2] : "";

  std::unique_ptr<FstClass> ifst(FstClass::Read(in_name));
  if (!ifst) return 1;

  if (absl::GetFlag(FLAGS_pdt_parentheses).empty()) {
    LOG(ERROR) << argv[0] << ": No PDT parenthesis label pairs provided";
    return 1;
  }

  std::vector<std::pair<int64_t, int64_t>> parens;
  if (!ReadLabelPairs(absl::GetFlag(FLAGS_pdt_parentheses), &parens)) return 1;

  const auto weight_threshold =
      absl::GetFlag(FLAGS_weight).empty()
          ? WeightClass::Zero(ifst->WeightType())
          : WeightClass(ifst->WeightType(), absl::GetFlag(FLAGS_weight));

  VectorFstClass ofst(ifst->ArcType());
  s::Expand(*ifst, parens, &ofst,
            s::PdtExpandOptions(absl::GetFlag(FLAGS_connect),
                                absl::GetFlag(FLAGS_keep_parentheses),
                                weight_threshold));

  return !ofst.Write(out_name);
}
