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

#ifndef OPENFST_TEST_MY_REGISTER_H_
#define OPENFST_TEST_MY_REGISTER_H_

#include <string>

#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "openfst/lib/generic-register.h"

namespace fst {

class MyRegister
    : public GenericRegister<std::string, std::string, MyRegister> {
 protected:
  std::string ConvertKeyToSoFilename(absl::string_view key) const override {
    return absl::StrCat(key, ".so");
  }
};

using MyRegisterer = GenericRegisterer<MyRegister>;

}  // namespace fst

#endif  // OPENFST_TEST_MY_REGISTER_H_
