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
// Composes a PDT and an FST.

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
#include "openfst/extensions/pdt/compose.h"
#include "openfst/extensions/pdt/getters.h"
#include "openfst/extensions/pdt/pdtscript.h"
#include "openfst/lib/util.h"
#include "openfst/script/fst-class.h"

ABSL_DECLARE_FLAG(std::string, pdt_parentheses);
ABSL_DECLARE_FLAG(bool, left_pdt);
ABSL_DECLARE_FLAG(bool, connect);
ABSL_DECLARE_FLAG(std::string, compose_filter);

int pdtcompose_main(int argc, char** argv) {
  namespace s = fst::script;
  using fst::PdtComposeFilter;
  using fst::PdtComposeOptions;
  using fst::ReadLabelPairs;
  using fst::script::FstClass;
  using fst::script::VectorFstClass;

  std::string usage = "Compose a PDT and an FST.\n\n  Usage: ";
  usage += argv[0];
  usage += " in.pdt in.fst [out.pdt]\n";
  usage += " in.fst in.pdt [out.pdt]\n";

  fst::InitOpenFst(usage.c_str(), &argc, &argv, true);
  if (argc < 3 || argc > 4) {
    LOG(INFO) << absl::ProgramUsageMessage();
    return 1;
  }

  const std::string in1_name = strcmp(argv[1], "-") == 0 ? "" : argv[1];
  const std::string in2_name = strcmp(argv[2], "-") == 0 ? "" : argv[2];
  const std::string out_name =
      argc > 3 && strcmp(argv[3], "-") != 0 ? argv[3] : "";

  if (in1_name.empty() && in2_name.empty()) {
    LOG(ERROR) << argv[0] << ": Can't take both inputs from standard input.";
    return 1;
  }

  std::unique_ptr<FstClass> ifst1(FstClass::Read(in1_name));
  if (!ifst1) return 1;
  std::unique_ptr<FstClass> ifst2(FstClass::Read(in2_name));
  if (!ifst2) return 1;

  if (absl::GetFlag(FLAGS_pdt_parentheses).empty()) {
    LOG(ERROR) << argv[0] << ": No PDT parenthesis label pairs provided";
    return 1;
  }

  std::vector<std::pair<int64_t, int64_t>> parens;
  if (!ReadLabelPairs(absl::GetFlag(FLAGS_pdt_parentheses), &parens)) return 1;

  VectorFstClass ofst(ifst1->ArcType());

  PdtComposeFilter compose_filter;
  if (!s::GetPdtComposeFilter(absl::GetFlag(FLAGS_compose_filter),
                              &compose_filter)) {
    LOG(ERROR) << argv[0] << ": Unknown or unsupported compose filter type: "
               << absl::GetFlag(FLAGS_compose_filter);
    return 1;
  }

  const PdtComposeOptions copts(absl::GetFlag(FLAGS_connect), compose_filter);

  s::Compose(*ifst1, *ifst2, parens, &ofst, copts,
             absl::GetFlag(FLAGS_left_pdt));

  return !ofst.Write(out_name);
}
