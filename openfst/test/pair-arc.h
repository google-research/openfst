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
// An FST arc type with weights over product of the tropical semiring with
// itself.

#ifndef OPENFST_TEST_PAIR_ARC_H_
#define OPENFST_TEST_PAIR_ARC_H_

#include <string>
#include <utility>

#include "absl/base/no_destructor.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/float-weight.h"
#include "openfst/lib/fst-decl.h"
#include "openfst/lib/product-weight.h"

namespace fst {

// Arc with integer labels and state IDs and weights over the
// product of the tropical semiring with itself.
struct PairArc : public ArcTpl<ProductWeight<TropicalWeight, TropicalWeight>> {
  using Base = ArcTpl<ProductWeight<TropicalWeight, TropicalWeight>>;

  using Base::Base;

  static const std::string& Type() {
    static const absl::NoDestructor<std::string> type("pair");
    return *type;
  }
};

}  // namespace fst

#endif  // OPENFST_TEST_PAIR_ARC_H_
