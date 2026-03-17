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

#include "openfst/extensions/far/sttable.h"

#include <cstdint>
#include <ios>
#include <string>

#include "absl/strings/string_view.h"
#include "openfst/lib/file-util.h"
#include "openfst/lib/util.h"

namespace fst {

bool IsSTTable(absl::string_view source) {
  file::FileInStream strm(std::string(source),
                          std::ios_base::in | std::ios_base::binary);
  if (!strm.good()) return false;

  int32_t magic_number = 0;
  ReadType(strm, &magic_number);
  return magic_number == kSTTableMagicNumber;
}

}  // namespace fst
