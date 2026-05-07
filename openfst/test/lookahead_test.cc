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
// Unit test for lookahead matchers and filters.

#include "openfst/test/lookahead_test.h"

#include <cstdint>

#include "gtest/gtest.h"
#include "absl/flags/flag.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/compact-fst.h"
#include "openfst/lib/const-fst.h"
#include "openfst/lib/fst-decl.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/test/compactors.h"

namespace fst {
namespace {

using Arc = StdArc;
using StateId = Arc::StateId;
using Weight = Arc::Weight;
using Label = Arc::Label;

template <typename Arc>
using TrivialCompactArcFst =
    CompactArcFst<Arc, TrivialArcCompactor<Arc>, uint32_t>;

template <typename Arc>
using TrivialCompactFst = CompactFst<Arc, TrivialCompactor<Arc>>;

using LookAheadTypes =
    ::testing::Types<ConstFst<Arc>, VectorFst<Arc>, TrivialCompactArcFst<Arc>,
                     TrivialCompactFst<Arc>>;

INSTANTIATE_TYPED_TEST_SUITE_P(LookAhead, LookAheadTest, LookAheadTypes);

}  // namespace
}  // namespace fst
