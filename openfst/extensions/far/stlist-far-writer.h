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
// STListFarWriter class.

#ifndef OPENFST_EXTENSIONS_FAR_STLIST_FAR_WRITER_H_
#define OPENFST_EXTENSIONS_FAR_STLIST_FAR_WRITER_H_

#include <memory>

#include "absl/strings/string_view.h"
#include "openfst/extensions/far/far-type.h"
#include "openfst/extensions/far/far-writer.h"
#include "openfst/extensions/far/fst-writer.h"
#include "openfst/extensions/far/stlist.h"
#include "openfst/lib/fst.h"

namespace fst {

template <class A>
class STListFarWriter : public FarWriter<A> {
 public:
  using Arc = A;

  static STListFarWriter* Create(absl::string_view source) {
    auto* writer = STListWriter<Fst<Arc>, FstWriter<Arc>>::Create(source);
    return new STListFarWriter(writer);
  }

  void Add(absl::string_view key, const Fst<Arc>& fst) final {
    writer_->Add(key, fst);
  }

  FarType Type() const final { return FarType::STLIST; }

  bool Error() const final { return writer_->Error(); }

 private:
  explicit STListFarWriter(STListWriter<Fst<Arc>, FstWriter<Arc>>* writer)
      : writer_(writer) {}

  std::unique_ptr<STListWriter<Fst<Arc>, FstWriter<Arc>>> writer_;
};

}  // namespace fst

#endif  // OPENFST_EXTENSIONS_FAR_STLIST_FAR_WRITER_H_
