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

#include "openfst/lib/generic-register.h"

#include "absl/base/nullability.h"

#ifndef FST_NO_DYNAMIC_LINKING
#include <dlfcn.h>
#endif  // FST_NO_DYNAMIC_LINKING


#include "absl/log/log.h"

namespace fst {
namespace internal {

bool LoadSharedObject(const char* absl_nonnull so_filename) {
#ifdef FST_NO_DYNAMIC_LINKING
  return false;
#else
  void* handle = dlopen(so_filename, RTLD_LAZY);
  if (handle == nullptr) {
    LOG(ERROR) << "GenericRegister::GetEntry: " << dlerror();
    return false;
  }
#ifdef RUN_MODULE_INITIALIZERS
  RUN_MODULE_INITIALIZERS();
#endif
  return true;
#endif  // FST_NO_DYNAMIC_LINKING
}

}  // namespace internal
}  // namespace fst
