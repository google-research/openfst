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
// Creates a finite-state archive from input FSTs.

#include <cstdint>
#include <cstring>
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
#include "openfst/extensions/far/script-impl.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/file-util.h"
#include "openfst/lib/util.h"

ABSL_DECLARE_FLAG(std::string, key_prefix);
ABSL_DECLARE_FLAG(std::string, key_suffix);
ABSL_DECLARE_FLAG(int32_t, generate_keys);
ABSL_DECLARE_FLAG(std::string, far_type);
ABSL_DECLARE_FLAG(bool, file_list_input);

int farcreate_main(int argc, char** argv) {
  namespace s = fst::script;
  using fst::script::FarWriterClass;

  std::string usage = "Creates an archive from FSTs.\n\n  Usage: ";
  usage += argv[0];
  usage += " [in1.fst [[in2.fst ...] out.far]]\n";

  fst::InitOpenFst(usage.c_str(), &argc, &argv, true);

  std::vector<std::string> sources;
  if (absl::GetFlag(FLAGS_file_list_input)) {
    for (int i = 1; i < argc - 1; ++i) {
      file::FileInStream istrm(argv[i]);
      std::string str;
      while (std::getline(istrm, str)) sources.push_back(str);
    }
  } else {
    for (int i = 1; i < argc - 1; ++i)
      sources.push_back(strcmp(argv[i], "-") != 0 ? argv[i] : "");
    if (sources.empty()) {
      // argc == 1 || argc == 2. This cleverly handles both the no-file case
      // and the one (input) file case together.
      sources.push_back(argc == 2 && strcmp(argv[1], "-") != 0 ? argv[1] : "");
    }
  }

  // argc <= 2 means the file (if any) is an input file, so write to stdout.
  const std::string out_far =
      argc > 2 && strcmp(argv[argc - 1], "-") != 0 ? argv[argc - 1] : "";

  fst::FarType far_type;
  if (!s::GetFarType(absl::GetFlag(FLAGS_far_type), &far_type)) {
    LOG(ERROR) << "Unknown or unsupported FAR type: "
               << absl::GetFlag(FLAGS_far_type);
    return 1;
  }

  std::string arc_type = fst::ErrorArc::Type();
  if (!sources.empty()) {
    arc_type = s::LoadArcTypeFromFst(sources[0]);
    if (arc_type.empty()) return 1;
  }

  std::unique_ptr<FarWriterClass> writer(
      FarWriterClass::Create(out_far, arc_type, far_type));
  if (!writer) return 1;

  s::Create(sources, *writer, absl::GetFlag(FLAGS_generate_keys),
            absl::GetFlag(FLAGS_key_prefix), absl::GetFlag(FLAGS_key_suffix));

  if (writer->Error()) {
    FSTERROR() << "Error writing FAR: " << out_far;
    return 1;
  }

  return 0;
}
