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

#include "openfst/script/randgen.h"

#include <cstdint>
#include <optional>

#include "openfst/lib/properties.h"
#include "openfst/lib/randgen.h"
#include "openfst/script/fst-class.h"
#include "openfst/script/script-impl.h"

namespace fst {
namespace script {

void RandGen(const FstClass& ifst, MutableFstClass* ofst,
             const RandGenOptions<RandArcSelection>& opts,
             std::optional<uint64_t> seed) {
  if (!internal::ArcTypesMatch(ifst, *ofst, "RandGen")) {
    ofst->SetProperties(kError, kError);
    return;
  }
  FstRandGenArgs args{ifst, ofst, opts, seed};
  Apply<Operation<FstRandGenArgs>>("RandGen", ifst.ArcType(), &args);
}

void RandGen(const FstClass& ifst, MutableFstClass* ofst,
             const RandGenOptions<RandArcSelection>& opts) {
  RandGen(ifst, ofst, opts, std::nullopt);
}

REGISTER_FST_OPERATION_3ARCS(RandGen, FstRandGenArgs);

}  // namespace script
}  // namespace fst
