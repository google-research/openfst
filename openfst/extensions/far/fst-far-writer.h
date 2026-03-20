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
// FstFarWriter class.

#ifndef OPENFST_EXTENSIONS_FAR_FST_FAR_WRITER_H_
#define OPENFST_EXTENSIONS_FAR_FST_FAR_WRITER_H_

#include <string>

#include "absl/log/log.h"
#include "absl/strings/string_view.h"
#include "openfst/extensions/far/far-type.h"
#include "openfst/extensions/far/far-writer.h"
#include "openfst/lib/fst.h"

namespace fst {

template <class A>
class FstFarWriter final : public FarWriter<A> {
 public:
  using Arc = A;

  explicit FstFarWriter(absl::string_view source)
      : source_(source), error_(false), written_(false) {}

  static FstFarWriter* Create(absl::string_view source) {
    return new FstFarWriter(source);
  }

  void Add(absl::string_view key, const Fst<A>& fst) final {
    if (written_) {
      LOG(WARNING) << "FstFarWriter::Add: only one FST supported,"
                   << " subsequent entries discarded.";
    } else {
      error_ = !fst.Write(source_);
      written_ = true;
    }
  }

  FarType Type() const final { return FarType::FST; }

  bool Error() const final { return error_; }

  ~FstFarWriter() final = default;

 private:
  std::string source_;
  bool error_;
  bool written_;
};

}  // namespace fst

#endif  // OPENFST_EXTENSIONS_FAR_FST_FAR_WRITER_H_
