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
// Converts an RTN represented by FSTs and non-terminal labels into a PDT.

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "absl/flags/declare.h"
#include "absl/flags/flag.h"
#include "absl/log/flags.h"
#include "absl/log/log.h"
#include "absl/strings/numbers.h"
#include "third_party/gloop/base/commandlineflags.h"
#include "third_party/gloop/base/init_google.h"
#include "openfst/extensions/pdt/getters.h"
#include "openfst/extensions/pdt/pdtscript.h"
#include "openfst/extensions/pdt/replace.h"
#include "openfst/lib/util.h"
#include "openfst/script/fst-class.h"
#include "openfst/script/script-impl.h"

ABSL_DECLARE_FLAG(std::string, pdt_parentheses);
ABSL_DECLARE_FLAG(std::string, pdt_parser_type);
ABSL_DECLARE_FLAG(int64_t, start_paren_labels);
ABSL_DECLARE_FLAG(std::string, left_paren_prefix);
ABSL_DECLARE_FLAG(std::string, right_paren_prefix);

int pdtreplace_main(int argc, char** argv) {
  namespace s = fst::script;
  using fst::PdtParserType;
  using fst::WriteLabelPairs;
  using fst::script::FstClass;
  using fst::script::VectorFstClass;

  std::string usage = "Converts an RTN represented by FSTs";
  usage += " and non-terminal labels into PDT.\n\n  Usage: ";
  usage += argv[0];
  usage += " root.fst rootlabel [rule1.fst label1 ...] [out.fst]\n";

  fst::InitOpenFst(usage.c_str(), &argc, &argv, true);
  if (argc < 4) {
    LOG(INFO) << absl::ProgramUsageMessage();
    return 1;
  }

  const std::string out_name = argc % 2 == 0 ? argv[argc - 1] : "";

  PdtParserType parser_type;
  if (!s::GetPdtParserType(absl::GetFlag(FLAGS_pdt_parser_type),
                           &parser_type)) {
    LOG(ERROR) << argv[0] << ": Unknown PDT parser type: "
               << absl::GetFlag(FLAGS_pdt_parser_type);
    return 1;
  }

  std::vector<std::pair<int64_t, std::unique_ptr<const FstClass>>> pairs;
  for (auto i = 1; i < argc - 1; i += 2) {
    std::unique_ptr<const FstClass> ifst(FstClass::Read(argv[i]));
    if (!ifst) return 1;
    // Note that if the root label is beyond the range of the underlying FST's
    // labels, truncation will occur.
    int64_t label;
    if (!absl::SimpleAtoi(argv[i + 1], &label)) {
      LOG(ERROR) << argv[0] << ": Failed to convert \"" << argv[i + 1]
                 << " to integer";
      return 1;
    }
    pairs.emplace_back(label, std::move(ifst));
  }

  if (pairs.empty()) {
    LOG(ERROR) << argv[0] << ": At least one replace pair must be provided.";
    return 1;
  }
  const auto root = pairs.front().first;
  VectorFstClass ofst(pairs.back().second->ArcType());
  std::vector<std::pair<int64_t, int64_t>> parens;
  s::Replace(s::BorrowPairs(pairs), &ofst, &parens, root, parser_type,
             absl::GetFlag(FLAGS_start_paren_labels),
             absl::GetFlag(FLAGS_left_paren_prefix),
             absl::GetFlag(FLAGS_right_paren_prefix));

  if (!absl::GetFlag(FLAGS_pdt_parentheses).empty() &&
      !WriteLabelPairs(absl::GetFlag(FLAGS_pdt_parentheses), parens)) {
    return 1;
  }

  return !ofst.Write(out_name);
}
