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
// Tests whether two FAR files contains the same (key, fst) pairs.

#include <memory>
#include <string>

#include "absl/flags/usage.h"
#include "openfst/compat/init.h"
#include "absl/flags/declare.h"
#include "absl/flags/flag.h"
#include "absl/log/flags.h"
#include "absl/log/log.h"
#include "openfst/extensions/far/far-class.h"
#include "openfst/extensions/far/farscript.h"
#include "openfst/lib/util.h"

ABSL_DECLARE_FLAG(std::string, begin_key);
ABSL_DECLARE_FLAG(std::string, end_key);
ABSL_DECLARE_FLAG(double, delta);

int farequal_main(int argc, char** argv) {
  namespace s = fst::script;
  using fst::script::FarReaderClass;

  std::string usage = "Compares two FST archives for equality.\n\n  Usage: ";
  usage += argv[0];
  usage += " in1.far in2.far";

  fst::InitOpenFst(usage.c_str(), &argc, &argv, true);
  if (argc != 3) {
    LOG(INFO) << absl::ProgramUsageMessage();
    return 1;
  }

  std::unique_ptr<FarReaderClass> reader1(FarReaderClass::Open(argv[1]));
  if (!reader1) return 1;

  std::unique_ptr<FarReaderClass> reader2(FarReaderClass::Open(argv[2]));
  if (!reader2) return 1;

  const bool result =
      s::Equal(*reader1, *reader2, absl::GetFlag(FLAGS_delta),
               absl::GetFlag(FLAGS_begin_key), absl::GetFlag(FLAGS_end_key));

  if (reader1->Error()) {
    FSTERROR() << "Error reading FAR: " << argv[1];
    return 1;
  }
  if (reader2->Error()) {
    FSTERROR() << "Error reading FAR: " << argv[2];
    return 1;
  }

  if (!result) VLOG(1) << "FARs are not equal";

  return result ? 0 : 2;
}
