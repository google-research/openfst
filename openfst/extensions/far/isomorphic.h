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

#ifndef OPENFST_EXTENSIONS_FAR_ISOMORPHIC_H_
#define OPENFST_EXTENSIONS_FAR_ISOMORPHIC_H_

#include "absl/log/log.h"
#include "absl/strings/string_view.h"
#include "openfst/extensions/far/far-reader.h"
#include "openfst/extensions/far/map-reduce.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/isomorphic.h"
#include "openfst/lib/weight.h"

namespace fst {

template <class Arc>
bool Isomorphic(FarReader<Arc>& reader1, FarReader<Arc>& reader2,
                float delta = kDelta, absl::string_view begin_key = "",
                absl::string_view end_key = "") {
  return internal::MapAllReduce(
      reader1, reader2,
      [delta](absl::string_view key, const Fst<Arc>* fst1,
              const Fst<Arc>* fst2) {
        if (!Isomorphic(*fst1, *fst2, delta)) {
          LOG(ERROR) << "Isomorphic: FSTs for key " << key << "are not equal";
          return false;
        }
        return true;
      },
      begin_key, end_key);
}

}  // namespace fst

#endif  // OPENFST_EXTENSIONS_FAR_ISOMORPHIC_H_
