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
// FstReader class.

#ifndef OPENFST_EXTENSIONS_FAR_FST_READER_H_
#define OPENFST_EXTENSIONS_FAR_FST_READER_H_

#include <istream>

#include "openfst/lib/fst.h"

namespace fst {

template <class Arc>
class FstReader {
 public:
  Fst<Arc>* operator()(std::istream& strm,
                       const FstReadOptions& options = FstReadOptions()) const {
    return Fst<Arc>::Read(strm, options);
  }
};

}  // namespace fst

#endif  // OPENFST_EXTENSIONS_FAR_FST_READER_H_
