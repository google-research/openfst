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

#ifndef OPENFST_EXTENSIONS_FAR_CONVERT_H_
#define OPENFST_EXTENSIONS_FAR_CONVERT_H_

#include "absl/memory/memory.h"
#include "openfst/compat/compat_memory.h"
#include "absl/strings/string_view.h"
#include "openfst/extensions/far/far-reader.h"
#include "openfst/extensions/far/far-writer.h"
#include "openfst/extensions/far/map-reduce.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/register.h"
#include "openfst/lib/util.h"

namespace fst {

template <class Arc>
void Convert(FarReader<Arc>& reader, FarWriter<Arc>& writer,
             absl::string_view fst_type) {
  internal::Map(reader, writer,
                [&fst_type](absl::string_view key, const Fst<Arc>* ifst) {
                  if (fst_type.empty() || ifst->Type() == fst_type) {
                    return absl::WrapUnique(ifst->Copy());
                  }
                  auto ofst = absl::WrapUnique(Convert(*ifst, fst_type));
                  if (!ofst) {
                    FSTERROR() << "FarConvert: Cannot convert FST with key "
                               << key << " to " << fst_type;
                  }
                  return ofst;
                });
}

}  // namespace fst

#endif  // OPENFST_EXTENSIONS_FAR_CONVERT_H_
