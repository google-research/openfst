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
// STListFarReader class.

#ifndef OPENFST_EXTENSIONS_FAR_STLIST_FAR_READER_H_
#define OPENFST_EXTENSIONS_FAR_STLIST_FAR_READER_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "absl/memory/memory.h"
#include "openfst/compat/compat_memory.h"
#include "absl/strings/string_view.h"
#include "openfst/extensions/far/far-reader.h"
#include "openfst/extensions/far/far-type.h"
#include "openfst/extensions/far/fst-reader.h"
#include "openfst/extensions/far/stlist.h"
#include "openfst/lib/fst.h"

namespace fst {

template <class A>
class STListFarReader : public FarReader<A> {
 public:
  using Arc = A;

  static STListFarReader* Open(absl::string_view source) {
    auto reader =
        absl::WrapUnique(STListReader<Fst<Arc>, FstReader<Arc>>::Open(source));
    if (!reader || reader->Error()) return nullptr;
    return new STListFarReader(std::move(reader));
  }

  static STListFarReader* Open(std::vector<std::string> sources) {
    auto reader = absl::WrapUnique(
        STListReader<Fst<Arc>, FstReader<Arc>>::Open(std::move(sources)));
    if (!reader || reader->Error()) return nullptr;
    return new STListFarReader(std::move(reader));
  }

  void Reset() final { reader_->Reset(); }

  bool Find(absl::string_view key) final { return reader_->Find(key); }

  bool Done() const final { return reader_->Done(); }

  void Next() final { return reader_->Next(); }

  const std::string& GetKey() const final { return reader_->GetKey(); }

  const Fst<Arc>* GetFst() const final { return reader_->GetEntry(); }

  FarType Type() const final { return FarType::STLIST; }

  bool Error() const final { return reader_->Error(); }

 private:
  explicit STListFarReader(
      std::unique_ptr<STListReader<Fst<Arc>, FstReader<Arc>>> reader)
      : reader_(std::move(reader)) {}

  std::unique_ptr<STListReader<Fst<Arc>, FstReader<Arc>>> reader_;
};

}  // namespace fst

#endif  // OPENFST_EXTENSIONS_FAR_STLIST_FAR_READER_H_
