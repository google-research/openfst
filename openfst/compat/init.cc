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

#include "openfst/compat/init.h"

#include <algorithm>
#include <cstdint>
#include <vector>

#include "absl/base/nullability.h"
#include "absl/container/flat_hash_set.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "absl/log/initialize.h"
#include "absl/strings/string_view.h"

namespace fst {
namespace {

// Unlike the original implementation in
// https://github.com/abseil/gloop/blob/main/gloop/base/commandlineflags.h
// this function does not save the original argument vector and does not
// call `InstallFlagsUsageConfig`.
uint32_t ParseCommandLineFlagsInternal(int* argc, char*** argv,
                                       bool remove_flags, bool do_report) {
  char** argv_val = *argv;

  // NOLINTBEGIN(abseil-no-internal-dependencies)
  const std::vector<char*> positional_args =
      absl::flags_internal::ParseCommandLineImpl(
          *argc, argv_val,
          do_report ? absl::flags_internal::UsageFlagsAction::kHandleUsage
                    : absl::flags_internal::UsageFlagsAction::kIgnoreUsage,
          absl::flags_internal::OnUndefinedFlag::kIgnoreUndefined);
  // NOLINTEND(abseil-no-internal-dependencies)

  if (remove_flags) {
    // If Abseil Flags were intended to be removed, the positional_args contain
    // our desired output. Copy the values into argv as is and return 1 as the
    // index of the first positional argument.
    // The C-standard requires argv[argc] to be nullptr, but some code passes
    // an "artificial" argv that isn't null-terminated. To accommodate both
    // cases we keep the end of the argument vector in place and move the
    // beginning (instead of keeping the beginning at 0, moving the end and
    // setting the argv[argc] to nullptr). This way we do not need to set
    // argv[argc] to nullptr. If it was there in the original argument vector it
    // will continue to exist in our output.
    *argv = argv_val = argv_val + *argc - positional_args.size();
    std::copy(positional_args.begin(), positional_args.end(), argv_val);
    *argc = positional_args.size();
    return 1;
  }

  absl::flat_hash_set<void*> position_args_set(positional_args.begin(),
                                               positional_args.end());

  // If we were asked to keep arguments corresponding to Abseil Flags, we should
  // iterate through the original arguments list and keep those that are not
  // present in positional arguments list.
  uint32_t out_pos = 1;
  for (int in_pos = 0; in_pos < *argc; ++in_pos) {
    if (position_args_set.count(argv_val[in_pos]) == 0) {
      argv_val[out_pos++] = argv_val[in_pos];
    }
  }

  // Now we can add all the positional arguments back into original list.
  // The first index we add positional argument to is our result value.
  // We are skipping first element in positional_args, since it is program name.
  // If positional_args has 0 or 1 elements, there are no additional positional
  // arguments to copy.
  if (positional_args.size() > 1) {
    std::copy(positional_args.begin() + 1, positional_args.end(),
              argv_val + out_pos);
  }
  *argc = out_pos + positional_args.size() - 1;

  return out_pos;
}

}  // namespace

void InitOpenFst(absl::string_view usage, int* absl_nonnull argc,
                 char* absl_nullable* absl_nonnull* absl_nonnull argv,
                 bool remove_flags) {
  absl::SetProgramUsageMessage(usage);
  char** argv_val = *argv;
  const std::vector<char*> pos_args = absl::ParseCommandLine(*argc, *argv);
  absl::InitializeLog();
  if (!remove_flags || pos_args.empty()) return;

  *argv = argv_val = argv_val + *argc - pos_args.size();
  std::copy(pos_args.begin(), pos_args.end(), argv_val);
  *argc = pos_args.size();
}

uint32_t ParseCommandLineFlags(int* argc, char*** argv, bool remove_flags) {
  return ParseCommandLineFlagsInternal(argc, argv, remove_flags, true);
}

uint32_t ParseCommandLineNonHelpFlags(int* argc, char*** argv,
                                      bool remove_flags) {
  return ParseCommandLineFlagsInternal(argc, argv, remove_flags, false);
}

}  // namespace fst
