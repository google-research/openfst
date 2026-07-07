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
// Unit tests for FST property bit manipulation and verification.

#include "openfst/lib/properties.h"

#include <cstdint>

#include "gtest/gtest.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/test-properties.h"
#include "openfst/lib/vector-fst.h"

namespace fst {
namespace {

using Arc = StdArc;
using Weight = Arc::Weight;

TEST(PropertiesTest, KnownProperties) {
  // Binary properties are always returned set by KnownProperties.
  EXPECT_TRUE(internal::KnownProperties(0) & kExpanded);
  EXPECT_TRUE(internal::KnownProperties(0) & kMutable);
  EXPECT_TRUE(internal::KnownProperties(0) & kError);

  // Trinary properties return both positive and negative bits set if either
  // is known.
  const uint64_t props = kAcceptor;
  const uint64_t known = internal::KnownProperties(props);
  EXPECT_TRUE(known & kAcceptor);
  EXPECT_TRUE(known & kNotAcceptor);
  EXPECT_FALSE(known & kIDeterministic);
}

TEST(PropertiesTest, CompatProperties) {
  // Compatible sets.
  EXPECT_TRUE(internal::CompatProperties(kAcceptor, kAcceptor));
  EXPECT_TRUE(internal::CompatProperties(kAcceptor, kIDeterministic));

  // Incompatible trinary property values.
  EXPECT_FALSE(internal::CompatProperties(kAcceptor, kNotAcceptor));
  EXPECT_FALSE(internal::CompatProperties(kAcyclic, kCyclic));
}

TEST(PropertiesTest, AddArcProperties) {
  uint64_t props = kAcceptor | kILabelSorted | kTopSorted | kNoEpsilons |
                   kNoIEpsilons | kNoOEpsilons | kUnweighted;
  Arc arc(1, 1, Weight::One(), 1);
  props = AddArcProperties<Arc>(props, 0, arc, nullptr);
  // Equal ilabel and olabel preserve acceptor.
  EXPECT_TRUE(props & kAcceptor);
  EXPECT_FALSE(props & kNotAcceptor);

  // Add a non-acceptor arc out of order.
  Arc arc2(2, 3, Weight(2.0), 0);
  props = AddArcProperties<Arc>(props, 0, arc2, &arc);
  EXPECT_TRUE(props & kNotAcceptor);
  EXPECT_FALSE(props & kAcceptor);
  EXPECT_TRUE(props & kWeighted);
  EXPECT_FALSE(props & kUnweighted);
  // nextstate 0 <= state 0 so top-sorted is false.
  EXPECT_TRUE(props & kNotTopSorted);
}

TEST(PropertiesTest, SetFinalProperties) {
  uint64_t props = kAcceptor | kUnweighted;
  props = SetFinalProperties(props, Weight::Zero(), Weight(1.5));
  EXPECT_TRUE(props & kWeighted);
  EXPECT_FALSE(props & kUnweighted);
}

TEST(PropertiesTest, DeleteArcsProperties) {
  const uint64_t props = kAcceptor | kILabelSorted | kTopSorted;
  const uint64_t deleted_props = DeleteArcsProperties(props);
  // Deleting arcs preserves trinary properties like acceptor and sortedness.
  EXPECT_TRUE(deleted_props & kAcceptor);
  EXPECT_TRUE(deleted_props & kILabelSorted);
}

TEST(PropertiesTest, ComputePropertiesAcyclic) {
  VectorFst<Arc> fst;
  const auto s0 = fst.AddState();
  const auto s1 = fst.AddState();
  fst.SetStart(s0);
  fst.SetFinal(s1, Weight::One());
  fst.AddArc(s0, Arc(1, 1, Weight::One(), s1));

  uint64_t known = 0;
  const uint64_t props =
      internal::ComputeProperties(fst, kFstProperties, &known);
  EXPECT_TRUE(props & kAcceptor);
  EXPECT_TRUE(props & kAcyclic);
  EXPECT_TRUE(props & kTopSorted);
}

TEST(PropertiesTest, ComputePropertiesCyclic) {
  VectorFst<Arc> fst;
  const auto s0 = fst.AddState();
  fst.SetStart(s0);
  fst.SetFinal(s0, Weight::One());
  // Self-loop makes it cyclic.
  fst.AddArc(s0, Arc(1, 1, Weight::One(), s0));

  uint64_t known = 0;
  const uint64_t props =
      internal::ComputeProperties(fst, kFstProperties, &known);
  EXPECT_TRUE(props & kCyclic);
  EXPECT_FALSE(props & kAcyclic);
}

}  // namespace
}  // namespace fst
