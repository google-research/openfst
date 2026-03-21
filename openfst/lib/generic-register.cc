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

#ifndef FST_NO_DYNAMIC_LINKING
#if !defined(_WIN32)
#include <dlfcn.h>
#else
#include <windows.h>
#endif  // _WIN32
#endif  // FST_NO_DYNAMIC_LINKING


#include "absl/base/nullability.h"
#include "absl/log/log.h"

namespace fst {
namespace internal {

bool LoadSharedObject(const char* absl_nonnull so_filename) {
#ifdef FST_NO_DYNAMIC_LINKING
  return false;
#else
#if defined(_WIN32)
  static const HMODULE handle = LoadLibrary(TEXT(so_filename));
  if (handle == nullptr) {
    wchar_t buf[512];
    FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                   nullptr, GetLastError(),
                   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buf,
                   sizeof(buf) / sizeof(wchar_t), nullptr);
    LOG(ERROR) << "GenericRegister::GetEntry for '" << so_filename
               << "': " << buf;
    return false;
  }
#else
  static const void* handle = dlopen(so_filename, RTLD_LAZY);
  if (handle == nullptr) {
    LOG(ERROR) << "GenericRegister::GetEntry for '" << so_filename
               << "': " << dlerror();
    return false;
  }
#endif  // WIN32
#ifdef RUN_MODULE_INITIALIZERS
  RUN_MODULE_INITIALIZERS();
#endif
  return true;
#endif  // FST_NO_DYNAMIC_LINKING
}

}  // namespace internal
}  // namespace fst
