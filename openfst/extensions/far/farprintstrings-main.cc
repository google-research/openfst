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
// Outputs as strings the string FSTs in a finite-state archive.

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "openfst/compat/init.h"
#include "absl/flags/declare.h"
#include "absl/flags/flag.h"
#include "absl/log/flags.h"
#include "absl/log/log.h"
#include "openfst/extensions/far/far-class.h"
#include "openfst/extensions/far/far-type.h"
#include "openfst/extensions/far/farscript.h"
#include "openfst/extensions/far/getters.h"
#include "openfst/lib/string.h"
#include "openfst/lib/util.h"
#include "openfst/script/getters.h"

ABSL_DECLARE_FLAG(std::string, filename_prefix);
ABSL_DECLARE_FLAG(std::string, filename_suffix);
ABSL_DECLARE_FLAG(int32_t, generate_filenames);
ABSL_DECLARE_FLAG(std::string, begin_key);
ABSL_DECLARE_FLAG(std::string, end_key);
ABSL_DECLARE_FLAG(bool, print_key);
ABSL_DECLARE_FLAG(bool, print_weight);
ABSL_DECLARE_FLAG(std::string, entry_type);
ABSL_DECLARE_FLAG(std::string, token_type);
ABSL_DECLARE_FLAG(std::string, symbols);
ABSL_DECLARE_FLAG(bool, initial_symbols);

int farprintstrings_main(int argc, char** argv) {
  namespace s = fst::script;
  using fst::script::FarReaderClass;

  std::string usage = "Prints strings in an FST archive.\n\n  Usage: ";
  usage += argv[0];
  usage += " [in1.far in2.far ...]\n";

  fst::InitOpenFst(usage.c_str(), &argc, &argv, true);

  std::vector<std::string> sources;
  for (int i = 1; i < argc; ++i) sources.push_back(argv[i]);
  if (sources.empty()) sources.push_back("");
  std::unique_ptr<FarReaderClass> reader(FarReaderClass::Open(sources));
  if (!reader) return 1;

  fst::FarEntryType entry_type;
  if (!s::GetFarEntryType(absl::GetFlag(FLAGS_entry_type), &entry_type)) {
    LOG(ERROR) << "Unknown or unsupported FAR entry type: "
               << absl::GetFlag(FLAGS_entry_type);
    return 1;
  }

  fst::TokenType token_type;
  if (!s::GetTokenType(absl::GetFlag(FLAGS_token_type), &token_type)) {
    LOG(ERROR) << "Unknown or unsupported FAR token type: "
               << absl::GetFlag(FLAGS_token_type);
    return 1;
  }

  s::PrintStrings(
      *reader, entry_type, token_type, absl::GetFlag(FLAGS_begin_key),
      absl::GetFlag(FLAGS_end_key), absl::GetFlag(FLAGS_print_key),
      absl::GetFlag(FLAGS_print_weight), absl::GetFlag(FLAGS_symbols),
      absl::GetFlag(FLAGS_initial_symbols),
      absl::GetFlag(FLAGS_generate_filenames),
      absl::GetFlag(FLAGS_filename_prefix),
      absl::GetFlag(FLAGS_filename_suffix));

  if (reader->Error()) {
    FSTERROR() << "Error reading FAR(s)";
    return 1;
  }

  return 0;
}
