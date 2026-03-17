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

#include "openfst/lib/file-util.h"

#include <cstdio>
#if defined(_WIN32)
#include <fcntl.h>
#include <io.h>

#include "absl/log/log.h"
#endif  // _WIN32

namespace fst {

void SetBinaryMode(FILE* fp) {
#if defined(_WIN32)
  if (_setmode(_fileno(fp), _O_BINARY) < 0) {
    // Quietly log the error message and continue.
    LOG(ERROR) << "Failed to mark the file as binary.";
  }
#endif  // _WIN32
}

}  // namespace fst
