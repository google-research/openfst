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
// Finite-State Transducer (FST) archive writer interface.

#ifndef OPENFST_EXTENSIONS_FAR_FAR_WRITER_H_
#define OPENFST_EXTENSIONS_FAR_FAR_WRITER_H_

#include "absl/strings/string_view.h"
#include "openfst/extensions/far/far-type.h"
#include "openfst/lib/fst.h"

namespace fst {

// This class creates an archive of FSTs.
template <class A>
class FarWriter {
 public:
  using Arc = A;

  // Creates a new (empty) FST archive; returns null on error.
  // The implementation is defined in far.h to avoid pulling in all subclasses
  // as dependencies of this header.
  static FarWriter* Create(absl::string_view source,
                           FarType type = FarType::DEFAULT);

  // Adds an FST to the end of an archive. Keys must be non-empty and
  // in lexicographic order. FSTs must have a suitable write method.
  virtual void Add(absl::string_view key, const Fst<Arc>& fst) = 0;

  virtual FarType Type() const = 0;

  virtual bool Error() const = 0;

  virtual ~FarWriter() = default;

 protected:
  FarWriter() = default;
};

}  // namespace fst

#endif  // OPENFST_EXTENSIONS_FAR_FAR_WRITER_H_
