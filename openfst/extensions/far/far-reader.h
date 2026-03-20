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
// Finite-State Transducer (FST) archive reader interface.

#ifndef OPENFST_EXTENSIONS_FAR_FAR_READER_H_
#define OPENFST_EXTENSIONS_FAR_FAR_READER_H_

#include <string>

#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include "openfst/extensions/far/far-type.h"
#include "openfst/lib/fst.h"

namespace fst {

// This class iterates through an existing archive of FSTs.
template <class A>
class FarReader {
 public:
  using Arc = A;

  // Opens an existing FST archive in a single file; returns null on error.
  // Sets current position to the beginning of the archive.
  // The implementation is defined in far.h to avoid pulling in all subclasses
  // as dependencies of this header.
  static FarReader* Open(absl::string_view source);

  // Opens an existing FST archive in multiple files; returns null on error.
  // Sets current position to the beginning of the archive.
  // The implementation is defined in far.h to avoid pulling in all subclasses
  // as dependencies of this header.
  static FarReader* Open(absl::Span<const std::string> sources);

  // Resets current position to beginning of archive.
  virtual void Reset() = 0;

  // Sets current position to first entry >= key. Returns true if a match.
  virtual bool Find(absl::string_view key) = 0;

  // Current position at end of archive?
  virtual bool Done() const = 0;

  // Move current position to next FST.
  virtual void Next() = 0;

  // Returns key at the current position. This reference is invalidated if
  // the current position in the archive is changed.
  virtual const std::string& GetKey() const = 0;

  // Returns pointer to FST at the current position. This is invalidated if
  // the current position in the archive is changed.
  virtual const Fst<Arc>* GetFst() const = 0;

  virtual FarType Type() const = 0;

  virtual bool Error() const = 0;

  virtual ~FarReader() = default;

 protected:
  FarReader() = default;
};

}  // namespace fst

#endif  // OPENFST_EXTENSIONS_FAR_FAR_READER_H_
