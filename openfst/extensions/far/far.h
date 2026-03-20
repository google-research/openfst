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
// Finite-State Transducer (FST) archive classes.

#ifndef OPENFST_EXTENSIONS_FAR_FAR_H_
#define OPENFST_EXTENSIONS_FAR_FAR_H_

#include <string>
#include <vector>

#include "absl/log/log.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include "openfst/extensions/far/far-reader.h"
#include "openfst/extensions/far/far-type.h"
#include "openfst/extensions/far/far-writer.h"
#include "openfst/extensions/far/fst-far-reader.h"
#include "openfst/extensions/far/fst-far-writer.h"
#include "openfst/extensions/far/fst-reader.h"
#include "openfst/extensions/far/fst-writer.h"
#include "openfst/extensions/far/stlist-far-reader.h"
#include "openfst/extensions/far/stlist-far-writer.h"
#include "openfst/extensions/far/stlist.h"
#include "openfst/extensions/far/sttable-far-reader.h"
#include "openfst/extensions/far/sttable-far-writer.h"
#include "openfst/extensions/far/sttable.h"


namespace fst {

// Checks for FST magic number in an input stream (to be opened given the source
// name), to indicate to the caller function that the stream content is an FST
// header.
bool IsFst(absl::string_view source);

// FST archive header class
class FarHeader {
 public:
  const std::string& ArcType() const { return arctype_; }

  enum FarType FarType() const { return fartype_; }

  bool Read(const std::string& source);

 private:
  enum FarType fartype_;
  std::string arctype_;
};

// Note: The factory method implementations (FarWriter::Create and
// FarReader::Open) are defined here in far.h, rather than in far-writer.h or
// far-reader.h, to avoid having the base interface headers depend on all of the
// specific FarReader and FarWriter subclasses (e.g., STList, STTable, etc.).
// This keeps the base headers lean for clients that only need the interface.

template <class Arc>
FarWriter<Arc>* FarWriter<Arc>::Create(absl::string_view source, FarType type) {
  switch (type) {
    case FarType::DEFAULT:
      if (source.empty()) return STListFarWriter<Arc>::Create(source);
      [[fallthrough]];
    case FarType::STTABLE:
      return STTableFarWriter<Arc>::Create(source);
    case FarType::STLIST:
      return STListFarWriter<Arc>::Create(source);
    case FarType::FST:
      return FstFarWriter<Arc>::Create(source);
    default:
      LOG(ERROR) << "FarWriter::Create: Unknown FAR type";
      return nullptr;
  }
}

template <class Arc>
FarReader<Arc>* FarReader<Arc>::Open(absl::string_view source) {
  if (source.empty())
    return STListFarReader<Arc>::Open(source);
  else if (IsSTTable(source))
    return STTableFarReader<Arc>::Open(source);
  else if (IsSTList(source))
    return STListFarReader<Arc>::Open(source);
  else if (IsFst(source))
    return FstFarReader<Arc>::Open(source);
  return nullptr;
}

template <class Arc>
FarReader<Arc>* FarReader<Arc>::Open(absl::Span<const std::string> sources) {
  if (!sources.empty() && sources[0].empty())
    return STListFarReader<Arc>::Open(
        std::vector<std::string>(sources.begin(), sources.end()));
  else if (!sources.empty() && IsSTTable(sources[0]))
    return STTableFarReader<Arc>::Open(
        std::vector<std::string>(sources.begin(), sources.end()));
  else if (!sources.empty() && IsSTList(sources[0]))
    return STListFarReader<Arc>::Open(
        std::vector<std::string>(sources.begin(), sources.end()));
  else if (!sources.empty() && IsFst(sources[0]))
    return FstFarReader<Arc>::Open(
        std::vector<std::string>(sources.begin(), sources.end()));
  return nullptr;
}

}  // namespace fst

#endif  // OPENFST_EXTENSIONS_FAR_FAR_H_
