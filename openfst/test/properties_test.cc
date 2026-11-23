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

// Masks covering both bits of each trinary property they name; a trinary
// property is unknown iff neither of its bits is set.
constexpr uint64_t kAcceptorMask = kAcceptor | kNotAcceptor;
constexpr uint64_t kIDeterministicMask = kIDeterministic | kNonIDeterministic;
constexpr uint64_t kCyclicMask = kCyclic | kAcyclic;
constexpr uint64_t kStringMask = kString | kNotString;
constexpr uint64_t kTrimMask =
    kAccessible | kNotAccessible | kCoAccessible | kNotCoAccessible;

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

TEST(PropertiesTest, ComposeProperties) {
  // Unweighted acceptors and transducers propagate both kUnweighted and
  // kUnweightedCycles even when kUnweightedCycles is not set on the inputs
  // (e.g., after VectorFst::AddArc).
  const uint64_t acceptor_out =
      ComposeProperties(kAcceptor | kUnweighted, kAcceptor | kUnweighted);
  EXPECT_EQ(acceptor_out & (kUnweighted | kUnweightedCycles),
            kUnweighted | kUnweightedCycles);
  EXPECT_EQ(acceptor_out & (kWeighted | kWeightedCycles), 0);

  const uint64_t transducer_out =
      ComposeProperties(kNotAcceptor | kUnweighted, kNotAcceptor | kUnweighted);
  EXPECT_EQ(transducer_out & (kUnweighted | kUnweightedCycles),
            kUnweighted | kUnweightedCycles);
  EXPECT_EQ(transducer_out & (kWeighted | kWeightedCycles), 0);

  // Weighted inputs with unweighted cycles propagate kUnweightedCycles (and
  // not kUnweighted), including when paired with a kUnweighted input.
  const uint64_t both_unweighted_cycles = ComposeProperties(
      kWeighted | kUnweightedCycles, kWeighted | kUnweightedCycles);
  EXPECT_EQ(both_unweighted_cycles & (kUnweighted | kUnweightedCycles),
            kUnweightedCycles);
  const uint64_t mixed =
      ComposeProperties(kUnweighted, kWeighted | kUnweightedCycles);
  EXPECT_EQ(mixed & (kUnweighted | kUnweightedCycles), kUnweightedCycles);

  // If either input may have weighted cycles, neither bit is propagated.
  EXPECT_EQ(ComposeProperties(kUnweighted, kWeighted) &
                (kUnweighted | kUnweightedCycles),
            0);
  EXPECT_EQ(ComposeProperties(kWeighted | kWeightedCycles,
                              kUnweighted | kUnweightedCycles) &
                (kUnweighted | kUnweightedCycles),
            0);
}

TEST(PropertiesTest, RmEpsilonProperties) {
  // Default (idempotent = false) does not propagate kUnweighted or
  // kUnweightedCycles, since non-idempotent semirings such as LogArc sum
  // parallel One() epsilon paths to a non-One() weight.
  EXPECT_EQ(RmEpsilonProperties(kAcceptor | kUnweighted | kUnweightedCycles) &
                (kUnweighted | kUnweightedCycles),
            0);
  EXPECT_EQ(RmEpsilonProperties(kAcceptor | kUnweighted | kUnweightedCycles,
                                /*delayed=*/true) &
                (kUnweighted | kUnweightedCycles),
            0);

  // Over an idempotent semiring, kUnweighted propagates both kUnweighted and
  // kUnweightedCycles even when kUnweightedCycles is unset on the input
  // (e.g., after VectorFst::AddArc).
  for (const bool delayed : {false, true}) {
    const uint64_t unweighted_out = RmEpsilonProperties(
        kAcceptor | kUnweighted, delayed, /*idempotent=*/true);
    EXPECT_EQ(unweighted_out & (kUnweighted | kUnweightedCycles),
              kUnweighted | kUnweightedCycles);
    EXPECT_EQ(unweighted_out & (kWeighted | kWeightedCycles), 0);

    // Weighted inputs with unweighted cycles preserve kUnweightedCycles
    // without setting kUnweighted.
    const uint64_t unweighted_cycles_out = RmEpsilonProperties(
        kWeighted | kUnweightedCycles, delayed, /*idempotent=*/true);
    EXPECT_EQ(unweighted_cycles_out & (kUnweighted | kUnweightedCycles),
              kUnweightedCycles);

    // Inputs that may have weighted cycles propagate neither bit.
    EXPECT_EQ(RmEpsilonProperties(kWeighted, delayed, /*idempotent=*/true) &
                  (kUnweighted | kUnweightedCycles),
              0);
    EXPECT_EQ(RmEpsilonProperties(kWeighted | kWeightedCycles, delayed,
                                  /*idempotent=*/true) &
                  (kUnweighted | kUnweightedCycles),
              0);
  }
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

TEST(PropertiesTest,
     ComputeOrUseStoredPropertiesSkipsArcScanWhenOnlyDfsUnknown) {
  // Cyclic FST with kIDeterministic already stored and kAcyclic | kCyclic (and
  // kString | kNotString) unknown: querying kAcyclic | kIDeterministic should
  // run only the DFS pass and skip the state/arc scan.
  VectorFst<Arc> cyclic;
  cyclic.AddState();
  cyclic.SetStart(0);
  cyclic.SetFinal(0, Weight::One());
  cyclic.AddArc(0, Arc(1, 1, Weight::One(), 0));
  cyclic.SetProperties(kIDeterministic, kIDeterministic);
  ASSERT_EQ(cyclic.Properties(kCyclicMask | kStringMask, false), 0);

  uint64_t known = 0;
  const uint64_t cyclic_props = internal::ComputeOrUseStoredProperties(
      cyclic, kAcyclic | kIDeterministic, &known);
  EXPECT_EQ(cyclic_props & (kCyclic | kIDeterministic),
            kCyclic | kIDeterministic);
  EXPECT_EQ(known & (kCyclicMask | kIDeterministicMask),
            kCyclicMask | kIDeterministicMask);
  EXPECT_EQ(known & kStringMask, 0);
}

TEST(PropertiesTest,
     ComputeOrUseStoredPropertiesSkipsDfsWhenOnlyArcScanUnknown) {
  // Acyclic top-sorted FST with kAcyclic already stored and kIDeterministic
  // (and kAccessible | kCoAccessible) unknown: querying
  // kAcyclic | kIDeterministic should run only the state/arc scan and skip the
  // DFS pass.
  VectorFst<Arc> acyclic;
  acyclic.AddStates(2);
  acyclic.SetStart(0);
  acyclic.SetFinal(1, Weight::One());
  acyclic.AddArc(0, Arc(1, 1, Weight::One(), 1));
  ASSERT_EQ(acyclic.Properties(kAcyclic, false), kAcyclic);
  ASSERT_EQ(acyclic.Properties(kIDeterministicMask | kTrimMask, false), 0);

  uint64_t known = 0;
  const uint64_t acyclic_props = internal::ComputeOrUseStoredProperties(
      acyclic, kAcyclic | kIDeterministic, &known);
  EXPECT_EQ(acyclic_props & (kAcyclic | kIDeterministic),
            kAcyclic | kIDeterministic);
  EXPECT_EQ(known & (kCyclicMask | kIDeterministicMask),
            kCyclicMask | kIDeterministicMask);
  EXPECT_EQ(known & kTrimMask, 0);
}

TEST(PropertiesTest,
     ComputeOrUseStoredPropertiesPrefersComputedOverStoredProperties) {
  // Transducer with a deliberately incorrect stored kAcceptor bit and
  // kIDeterministic unknown: the state/arc scan needed for kIDeterministic
  // also determines kNotAcceptor, which must be returned in place of the
  // stored bit so that callers can detect incorrectly stored properties.
  VectorFst<Arc> transducer;
  transducer.AddStates(2);
  transducer.SetStart(0);
  transducer.SetFinal(1, Weight::One());
  transducer.AddArc(0, Arc(1, 2, Weight::One(), 1));
  transducer.SetProperties(kAcceptor, kAcceptorMask);
  ASSERT_EQ(transducer.Properties(kIDeterministicMask, false), 0);

  const uint64_t props = internal::ComputeOrUseStoredProperties(
      transducer, kAcceptor | kIDeterministic, /*known=*/nullptr);
  EXPECT_EQ(props & (kAcceptorMask | kIDeterministic),
            kNotAcceptor | kIDeterministic);
}

}  // namespace
}  // namespace fst
