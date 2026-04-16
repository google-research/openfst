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

#include <string>

#include "absl/flags/usage.h"
#include "openfst/compat/init.h"
#include "absl/flags/declare.h"
#include "absl/flags/flag.h"
#include "absl/log/flags.h"
#include "openfst/extensions/linear/linearscript.h"

ABSL_DECLARE_FLAG(std::string, arc_type);
ABSL_DECLARE_FLAG(std::string, epsilon_symbol);
ABSL_DECLARE_FLAG(std::string, unknown_symbol);
ABSL_DECLARE_FLAG(std::string, vocab);
ABSL_DECLARE_FLAG(std::string, out);
ABSL_DECLARE_FLAG(std::string, save_isymbols);
ABSL_DECLARE_FLAG(std::string, save_fsymbols);
ABSL_DECLARE_FLAG(std::string, save_osymbols);

int fstlinear_main(int argc, char** argv) {
  // TODO: more detailed usage
  fst::InitOpenFst(argv[0], &argc, &argv, true);
  fst::script::ValidateDelimiter();
  fst::script::ValidateEmptySymbol();

  if (argc == 1) {
    LOG(INFO) << absl::ProgramUsageMessage();
    return 1;
  }

  fst::script::LinearCompile(
      absl::GetFlag(FLAGS_arc_type), absl::GetFlag(FLAGS_epsilon_symbol),
      absl::GetFlag(FLAGS_unknown_symbol), absl::GetFlag(FLAGS_vocab), argv + 1,
      argc - 1, absl::GetFlag(FLAGS_out), absl::GetFlag(FLAGS_save_isymbols),
      absl::GetFlag(FLAGS_save_fsymbols), absl::GetFlag(FLAGS_save_osymbols));

  return 0;
}
