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
// Creates and returns an FST from a textual representation,
//
// e.g.:
// std::unique_ptr<StdFst> f(InlineFst<StdArc>(
//   "0 1 1 2 4.0\n"
//   "1 2 3 4 2.0\n"
//   "2\n"));
//
// This function is restricted to unit tests to encourage the use of
// the C++ MutableFst mutator methods in non-test code.

#ifndef OPENFST_TEST_INLINE_FST_H_
#define OPENFST_TEST_INLINE_FST_H_

#include <memory>
#include <sstream>
#include <string>

#include "absl/log/log.h"
#include "absl/memory/memory.h"
#include "openfst/compat/compat_memory.h"
#include "openfst/lib/fst.h"
#include "openfst/script/compile-impl.h"

namespace fst {
template <class A>
std::unique_ptr<Fst<A>> InlineFst(const std::string& text,
                                  bool accept = false) {
  std::istringstream istr(text);
  auto rv =
      absl::WrapUnique(FstCompiler<A>(istr, "inline", nullptr, nullptr, nullptr,
                                      accept, false, false, false)
                           .Fst()
                           .Copy());
  if (!istr.fail()) {
    LOG(FATAL) << "Failed to read inline Fst";  // Crash OK
    return nullptr;
  }
  if (rv == nullptr) {
    LOG(FATAL) << "Failed to compile inline Fst";  // Crash OK
  }
  return rv;
}
}  // namespace fst

#endif  // OPENFST_TEST_INLINE_FST_H_
