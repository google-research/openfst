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
// FstFarReader class.

#ifndef OPENFST_EXTENSIONS_FAR_FST_FAR_READER_H_
#define OPENFST_EXTENSIONS_FAR_FST_FAR_READER_H_

#include <algorithm>
#include <cstddef>
#include <ios>
#include <iostream>
#include <istream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "absl/log/log.h"
#include "absl/strings/string_view.h"
#include "openfst/extensions/far/far-reader.h"
#include "openfst/extensions/far/far-type.h"
#include "openfst/lib/file-util.h"
#include "openfst/lib/fst.h"

namespace fst {

template <class A>
class FstFarReader final : public FarReader<A> {
 public:
  using Arc = A;

  static FstFarReader* Open(absl::string_view source) {
    std::vector<std::string> sources;
    sources.push_back(std::string(source));
    return new FstFarReader<Arc>(sources);
  }

  static FstFarReader* Open(std::vector<std::string> sources) {
    return new FstFarReader<Arc>(std::move(sources));
  }

  explicit FstFarReader(std::vector<std::string> sources)
      : keys_(std::move(sources)), has_stdin_(false), pos_(0), error_(false) {
    std::sort(keys_.begin(), keys_.end());
    streams_.resize(keys_.size(), nullptr);
    for (size_t i = 0; i < keys_.size(); ++i) {
      if (keys_[i].empty()) {
        if (!has_stdin_) {
          streams_[i] = &std::cin;
          has_stdin_ = true;
          SetBinaryMode(stdin);
        } else {
          FSTERROR() << "FstFarReader::FstFarReader: standard input should "
                        "only appear once in the input file list";
          error_ = true;
          return;
        }
      } else {
        streams_[i] = new file::FileInStream(
            keys_[i], std::ios_base::in | std::ios_base::binary);
        if (streams_[i]->fail()) {
          FSTERROR() << "FstFarReader::FstFarReader: Error reading file: "
                     << keys_[i];
          error_ = true;
          return;
        }
      }
    }
    if (pos_ >= keys_.size()) return;
    ReadFst();
  }

  void Reset() final {
    if (has_stdin_) {
      FSTERROR()
          << "FstFarReader::Reset: Operation not supported on standard input";
      error_ = true;
      return;
    }
    pos_ = 0;
    ReadFst();
  }

  bool Find(absl::string_view key) final {
    if (has_stdin_) {
      FSTERROR()
          << "FstFarReader::Find: Operation not supported on standard input";
      error_ = true;
      return false;
    }
    pos_ = 0;  // TODO
    ReadFst();
    return true;
  }

  bool Done() const final { return error_ || pos_ >= keys_.size(); }

  void Next() final {
    ++pos_;
    ReadFst();
  }

  const std::string& GetKey() const final { return keys_[pos_]; }

  const Fst<Arc>* GetFst() const final { return fst_.get(); }

  FarType Type() const final { return FarType::FST; }

  bool Error() const final { return error_; }

  ~FstFarReader() final {
    for (size_t i = 0; i < keys_.size(); ++i) {
      if (streams_[i] != &std::cin) {
        delete streams_[i];
      }
    }
  }

 private:
  void ReadFst() {
    fst_.reset();
    if (pos_ >= keys_.size()) return;
    streams_[pos_]->seekg(0);
    fst_.reset(Fst<Arc>::Read(*streams_[pos_], FstReadOptions()));
    if (!fst_) {
      FSTERROR() << "FstFarReader: Error reading Fst from: " << keys_[pos_];
      error_ = true;
    }
  }

  std::vector<std::string> keys_;
  std::vector<std::istream*> streams_;
  bool has_stdin_;
  size_t pos_;
  mutable std::unique_ptr<Fst<Arc>> fst_;
  mutable bool error_;
};

}  // namespace fst

#endif  // OPENFST_EXTENSIONS_FAR_FST_FAR_READER_H_
