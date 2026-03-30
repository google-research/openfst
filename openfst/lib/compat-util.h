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

#ifndef OPENFST_LIB_COMPAT_UTIL_H_
#define OPENFST_LIB_COMPAT_UTIL_H_

// __native_client__ is defined for --config=nacl builds, targeting Chrome.

#if defined(__ANDROID__) || defined(__native_client__) ||               \
    defined(__APPLE__) || defined(_WIN32) ||                            \
    defined(GOOGLE_UNSUPPORTED_OS_LOONIX) || defined(__EMSCRIPTEN__) || \
    defined(__Fuchsia__)

#include "openfst/compat/checksummer.h"

#else  // __ANDROID__ || __native_client__ || __APPLE__ || _WIN32 ||
       // GOOGLE_UNSUPPORTED_OS_LOONIX || __EMSCRIPTEN__ || __Fuchsia__

#include "openfst/compat/checksummer.h"

namespace fst {
using CheckSummer = CheckSummer;
}  // namespace fst

#endif  // __ANDROID__ || __native_client__ || __APPLE__ || _WIN32 ||
        // GOOGLE_UNSUPPORTED_OS_LOONIX || __EMSCRIPTEN__ || __Fuchsia__

#endif  // OPENFST_LIB_COMPAT_UTIL_H_
