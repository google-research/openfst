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
// Extracts component FSTs from an finite-state archive.

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "openfst/compat/init.h"
#include "absl/flags/declare.h"
#include "absl/flags/flag.h"
#include "absl/log/flags.h"
#include "openfst/extensions/far/far-class.h"
#include "openfst/extensions/far/farscript.h"
#include "openfst/lib/util.h"

ABSL_DECLARE_FLAG(std::string, filename_prefix);
ABSL_DECLARE_FLAG(std::string, filename_suffix);
ABSL_DECLARE_FLAG(int32_t, generate_filenames);
ABSL_DECLARE_FLAG(std::string, keys);
ABSL_DECLARE_FLAG(std::string, key_separator);
ABSL_DECLARE_FLAG(std::string, range_delimiter);

int farextract_main(int argc, char** argv) {
  namespace s = fst::script;
  using fst::script::FarReaderClass;

  std::string usage = "Extracts FSTs from a finite-state archive.\n\n  Usage:";
  usage += argv[0];
  usage += " [in1.far in2.far...]\n";

  fst::InitOpenFst(usage.c_str(), &argc, &argv, true);

  std::vector<std::string> sources;
  for (int i = 1; i < argc; ++i) sources.push_back(argv[i]);
  if (sources.empty()) sources.push_back("");
  std::unique_ptr<FarReaderClass> reader(FarReaderClass::Open(sources));
  if (!reader) return 1;

  s::Extract(*reader, absl::GetFlag(FLAGS_generate_filenames),
             absl::GetFlag(FLAGS_keys), absl::GetFlag(FLAGS_key_separator),
             absl::GetFlag(FLAGS_range_delimiter),
             absl::GetFlag(FLAGS_filename_prefix),
             absl::GetFlag(FLAGS_filename_suffix));

  if (reader->Error()) {
    FSTERROR() << "Error reading FAR(s)";
    return 1;
  }

  return 0;
}
