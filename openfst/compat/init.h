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

// Initialization of binaries.

#ifndef OPENFST_COMPAT_INIT_H_
#define OPENFST_COMPAT_INIT_H_

#include <cstdint>

#include "absl/base/nullability.h"
#include "absl/strings/string_view.h"

namespace fst {

void InitOpenFst(absl::string_view usage, int* absl_nonnull argc,
                 char* absl_nullable* absl_nonnull* absl_nonnull argv,
                 bool remove_flags);

// Looks for flags in argv and parses them.  Rearranges argv to put
// flags first, or removes them entirely if remove_flags is true.
// If a flag is defined more than once in the command line or flag
// file, the last definition is used.  Returns the index (into argv)
// of the first non-flag argument.
uint32_t ParseCommandLineFlags(int* argc, char*** argv, bool remove_flags);

uint32_t ParseCommandLineNonHelpFlags(int* argc, char*** argv,
                                      bool remove_flags);

}  // namespace fst

#endif  // OPENFST_COMPAT_INIT_H_
