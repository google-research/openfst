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
// Classes and functions for registering and invoking FAR main
// functions that support multiple and extensible arc types.

#ifndef OPENFST_EXTENSIONS_FAR_GETTERS_H_
#define OPENFST_EXTENSIONS_FAR_GETTERS_H_

#include <string>

#include "absl/strings/string_view.h"
#include "openfst/extensions/far/far-type.h"

namespace fst {
namespace script {

bool GetFarType(absl::string_view str, FarType* far_type);

bool GetFarEntryType(absl::string_view str, FarEntryType* entry_type);

}  // namespace script

std::string GetFarTypeString(FarType far_type);

}  // namespace fst

#endif  // OPENFST_EXTENSIONS_FAR_GETTERS_H_
