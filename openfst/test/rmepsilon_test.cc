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
// Unit test for RmEpsilon.

#include "openfst/lib/rmepsilon.h"

#include <cstdint>
#include <memory>
#include <string>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "absl/flags/flag.h"
#include "absl/flags/reflection.h"
#include "absl/strings/string_view.h"
#include "openfst/lib/arc-map.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/connect.h"
#include "openfst/lib/equal.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/properties.h"
#include "openfst/lib/queue.h"
#include "openfst/lib/string-weight.h"
#include "openfst/lib/test-properties.h"
#include "openfst/lib/util.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/lib/verify.h"
#include "openfst/script/compile-impl.h"
#include "openfst/script/equal.h"
#include "openfst/script/fst-class.h"
#include "openfst/script/rmepsilon.h"
#include "openfst/script/verify.h"
#include "openfst/script/weight-class.h"

namespace fst {
namespace {

using Arc = StdArc;
using StateId = Arc::StateId;
using Weight = Arc::Weight;
using Label = Arc::Label;

class RmEpsilonTest : public testing::Test {
 protected:
  void SetUp() override {
    const std::string path =
        JoinPath(std::string("."),
                       "openfst/test/testdata/rmepsilon");
    const std::string rmeps1_name = JoinPath(path, "r1.fst");
    const std::string rmeps2_name = JoinPath(path, "r2.fst");
    const std::string rmeps3_name = JoinPath(path, "r3.fst");
    const std::string rmeps4_name = JoinPath(path, "r4.fst");

    rfst1_.reset(VectorFst<Arc>::Read(rmeps1_name));
    rfst2_.reset(VectorFst<Arc>::Read(rmeps2_name));
    rfst3_.reset(VectorFst<Arc>::Read(rmeps3_name));
    rfst4_.reset(VectorFst<Arc>::Read(rmeps4_name));
  }

  std::unique_ptr<const VectorFst<Arc>> rfst1_;
  std::unique_ptr<const VectorFst<Arc>> rfst2_;
  std::unique_ptr<const VectorFst<Arc>> rfst3_;
  std::unique_ptr<const VectorFst<Arc>> rfst4_;
};

TEST_F(RmEpsilonTest, RmEpsilon) {
  VectorFst<Arc> nfst1;
  VectorFst<Arc> nfst2;

  VectorFst<Arc> vfst(*rfst1_);

  RmEpsilon(&vfst, false);
  ASSERT_TRUE(Verify(vfst));
  ASSERT_TRUE(vfst.Properties(kNoEpsilons, true));
  ASSERT_TRUE(Equal(*rfst2_, vfst));

  RmEpsilon(&nfst1, false);
  ASSERT_TRUE(Verify(nfst1));
  ASSERT_TRUE(Equal(nfst1, nfst2));
}

TEST_F(RmEpsilonTest, RmEpsilonFstClass) {
  namespace s = fst::script;

  s::VectorFstClass rfst1(*rfst1_);
  s::VectorFstClass rfst2(*rfst2_);

  s::VectorFstClass vfst(rfst1);

  const s::WeightClass threshold = s::WeightClass::Zero(vfst.WeightType());
  const s::RmEpsilonOptions opts(AUTO_QUEUE, false, threshold);

  RmEpsilon(&vfst, opts);
  ASSERT_TRUE(Verify(vfst));
  ASSERT_TRUE(vfst.Properties(kNoEpsilons, true));
  ASSERT_TRUE(Equal(rfst2, vfst));
}

TEST_F(RmEpsilonTest, RmEpsilonFst) {
  VectorFst<Arc> nfst;

  RmEpsilonFst<Arc> dfst(*rfst1_);
  ASSERT_TRUE(Verify(dfst));
  ASSERT_TRUE(dfst.Properties(kNoEpsilons, true));
  ASSERT_TRUE(Equal(*rfst3_, dfst));

  RmEpsilonFst<Arc> ndfst(nfst);
  ASSERT_TRUE(Verify(ndfst));
  ASSERT_TRUE(Equal(nfst, ndfst));

  for (const bool safe : {false, true}) {
    RmEpsilonFst<Arc> cfst(dfst, safe);
    ASSERT_TRUE(Verify(cfst));
    ASSERT_TRUE(cfst.Properties(kNoEpsilons, true));
    ASSERT_TRUE(Equal(*rfst3_, cfst));
  }
}

TEST_F(RmEpsilonTest, PrunedRmEpsilon) {
  VectorFst<Arc> nfst1;
  VectorFst<Arc> nfst2;

  VectorFst<Arc> vfst(*rfst1_);

  RmEpsilon(&vfst, false, Weight(1.0), 10);
  ASSERT_TRUE(Verify(vfst));
  ASSERT_TRUE(vfst.Properties(kNoEpsilons, true));
  ASSERT_TRUE(Equal(*rfst4_, vfst));

  RmEpsilon(&nfst1, false, Weight(1.0), 10);
  ASSERT_TRUE(Verify(nfst1));
  ASSERT_TRUE(Equal(nfst1, nfst2));
}

// Exercises RmEpsilon with heap-owning weights, so that arcs moved into the
// output FST are checked for correctness (and under sanitizers).
TEST(RmEpsilonStringTest, RmEpsilonStringWeight) {
  using SArc = StringArc<STRING_RIGHT>;
  using SWeight = SArc::Weight;

  const SWeight w1 = Times(SWeight(1), SWeight(2));
  const SWeight w2 = Times(SWeight(3), Times(SWeight(4), SWeight(5)));

  // 0 --eps:eps/w1--> 1 --6:6/w2--> 2 (final).
  VectorFst<SArc> fst;
  fst.AddStates(3);
  fst.SetStart(0);
  fst.AddArc(0, SArc(0, 0, w1, 1));
  fst.AddArc(1, SArc(6, 6, w2, 2));
  fst.SetFinal(2, SWeight::One());

  // 0 --6:6/(w1 w2)--> 2 (final).
  VectorFst<SArc> expected;
  expected.AddStates(2);
  expected.SetStart(0);
  expected.AddArc(0, SArc(6, 6, Times(w1, w2), 1));
  expected.SetFinal(1, SWeight::One());

  RmEpsilon(&fst);
  ASSERT_TRUE(Verify(fst));
  ASSERT_TRUE(fst.Properties(kNoEpsilons, true));
  EXPECT_TRUE(Equal(expected, fst));
}

// Verifies that all stored FST properties on `fst` are compatible with the
// properties computed directly from its states and arcs.
template <class Arc>
void ExpectStoredPropertiesCompatible(const Fst<Arc>& fst) {
  uint64_t known = 0;
  const uint64_t stored = fst.Properties(kFstProperties, false);
  const uint64_t computed =
      internal::ComputeProperties(fst, kFstProperties, &known);
  EXPECT_TRUE(internal::CompatProperties(stored, computed));
}

TEST(RmEpsilonPropertiesIntegrationTest,
     IdempotentUnweightedPreservesProperties) {
  // Parallel One()-weight epsilon paths in a cyclic StdArc acceptor:
  // 0 --eps--> 1, 0 --eps--> 2 --eps--> 1, 1 --1:1--> 0 (final: 0).
  VectorFst<Arc> input;
  input.AddStates(3);
  input.SetStart(0);
  input.SetFinal(0, Weight::One());
  input.AddArc(0, Arc(0, 0, Weight::One(), 1));
  input.AddArc(0, Arc(0, 0, Weight::One(), 2));
  input.AddArc(2, Arc(0, 0, Weight::One(), 1));
  input.AddArc(1, Arc(1, 1, Weight::One(), 0));
  // AddArc preserves kUnweighted while clearing kUnweightedCycles.
  ASSERT_EQ(input.Properties(kUnweighted | kUnweightedCycles, false),
            kUnweighted);

  RmEpsilonFst<Arc> dfst(input);
  ASSERT_TRUE(Verify(dfst));
  EXPECT_EQ(dfst.Properties(kUnweighted | kUnweightedCycles, false),
            kUnweighted | kUnweightedCycles);
  ExpectStoredPropertiesCompatible(dfst);

  for (const bool connect : {true, false}) {
    VectorFst<Arc> vfst(input);
    RmEpsilon(&vfst, connect);
    ASSERT_TRUE(Verify(vfst));
    EXPECT_EQ(vfst.Properties(kUnweighted | kUnweightedCycles, false),
              kUnweighted | kUnweightedCycles);
    ExpectStoredPropertiesCompatible(vfst);
  }
}

TEST(RmEpsilonPropertiesIntegrationTest,
     IdempotentUnweightedCyclesPreservesCycleProperty) {
  // Weighted initial epsilon transition into an unweighted cycle:
  // 0 --eps/2.0--> 1 --eps/One()--> 2 --1:1/One()--> 1 (final: 1).
  VectorFst<Arc> input;
  input.AddStates(3);
  input.SetStart(0);
  input.SetFinal(1, Weight::One());
  input.AddArc(0, Arc(0, 0, Weight(2.0), 1));
  input.AddArc(1, Arc(0, 0, Weight::One(), 2));
  input.AddArc(2, Arc(1, 1, Weight::One(), 1));
  const uint64_t computed_in = input.Properties(kFstProperties, true);
  input.SetProperties(computed_in, kFstProperties);
  ASSERT_EQ(
      input.Properties(kWeighted | kUnweighted | kUnweightedCycles, false),
      kWeighted | kUnweightedCycles);

  RmEpsilonFst<Arc> dfst(input);
  ASSERT_TRUE(Verify(dfst));
  EXPECT_EQ(dfst.Properties(kUnweighted | kUnweightedCycles, false),
            kUnweightedCycles);
  ExpectStoredPropertiesCompatible(dfst);

  VectorFst<Arc> vfst(input);
  RmEpsilon(&vfst);
  ASSERT_TRUE(Verify(vfst));
  EXPECT_EQ(vfst.Properties(kUnweighted | kUnweightedCycles, false),
            kUnweightedCycles);
  ExpectStoredPropertiesCompatible(vfst);
}

TEST(RmEpsilonPropertiesIntegrationTest,
     NonIdempotentUnweightedDoesNotSetUnweighted) {
  // Over LogArc (non-idempotent), two parallel One()-weight epsilon paths sum
  // to Plus(One(), One()) = -log(2) != One(), yielding a weighted cycle.
  using LArc = LogArc;
  using LWeight = LArc::Weight;
  VectorFst<LArc> input;
  input.AddStates(3);
  input.SetStart(0);
  input.SetFinal(0, LWeight::One());
  input.AddArc(0, LArc(0, 0, LWeight::One(), 1));
  input.AddArc(0, LArc(0, 0, LWeight::One(), 2));
  input.AddArc(2, LArc(0, 0, LWeight::One(), 1));
  input.AddArc(1, LArc(1, 1, LWeight::One(), 0));
  ASSERT_EQ(input.Properties(kUnweighted, false), kUnweighted);

  RmEpsilonFst<LArc> dfst(input);
  ASSERT_TRUE(Verify(dfst));
  EXPECT_EQ(dfst.Properties(kUnweighted | kUnweightedCycles, false), 0);
  ExpectStoredPropertiesCompatible(dfst);
  EXPECT_EQ(dfst.Properties(kWeighted | kWeightedCycles, true),
            kWeighted | kWeightedCycles);

  VectorFst<LArc> lfst(input);
  RmEpsilon(&lfst);
  ASSERT_TRUE(Verify(lfst));
  EXPECT_EQ(lfst.Properties(kUnweighted | kUnweightedCycles, false), 0);
  ExpectStoredPropertiesCompatible(lfst);
  EXPECT_EQ(lfst.Properties(kWeighted | kWeightedCycles, true),
            kWeighted | kWeightedCycles);
}

// Mapper that preserves stored unweighted properties at construction and
// injects a non-member weight dynamically when an arc with `bad_ilabel` and
// `bad_nextstate` is expanded. Matching the destination state as well allows
// targeting a single epsilon arc, since final weights are mapped as epsilon
// arcs to kNoStateId.
struct InjectNonMemberMapper {
  using FromArc = Arc;
  using ToArc = Arc;

  InjectNonMemberMapper(Label bad_ilabel, StateId bad_nextstate)
      : bad_ilabel(bad_ilabel), bad_nextstate(bad_nextstate) {}

  Arc operator()(const Arc& arc) const {
    if (arc.ilabel == bad_ilabel && arc.nextstate == bad_nextstate) {
      return Arc(arc.ilabel, arc.olabel, Weight::NoWeight(), arc.nextstate);
    }
    return arc;
  }

  MapFinalAction FinalAction() const { return MAP_NO_SUPERFINAL; }
  MapSymbolsAction InputSymbolsAction() const { return MAP_COPY_SYMBOLS; }
  MapSymbolsAction OutputSymbolsAction() const { return MAP_COPY_SYMBOLS; }
  uint64_t Properties(uint64_t props) const { return props; }

  Label bad_ilabel;
  StateId bad_nextstate;
};

template <class A>
VectorFst<A> CompileTextFst(absl::string_view text) {
  SpanInStream istrm(text);
  const FstCompiler<A> compiler(istrm, /*source=*/"inline", /*isyms=*/nullptr,
                                /*osyms=*/nullptr, /*ssyms=*/nullptr,
                                /*accep=*/false, /*ikeep=*/false,
                                /*okeep=*/false, /*nkeep=*/true);
  return compiler.Fst();
}

TEST(RmEpsilonUnweightedTest, CyclicAndAcyclicStdArcClosure) {
  // Cyclic unweighted transducer with diamond + cycle epsilon paths and
  // duplicate non-epsilon transitions (1 --10:20--> 3 and 2 --10:20--> 3).
  VectorFst<Arc> cyclic = CompileTextFst<Arc>(
      "0 1 0 0\n"    // 0 --eps--> 1
      "0 2 0 0\n"    // 0 --eps--> 2
      "2 1 0 0\n"    // 2 --eps--> 1 (diamond)
      "1 0 0 0\n"    // 1 --eps--> 0 (cycle)
      "1 3 10 20\n"  // 1 --10:20--> 3
      "2 3 10 20\n"  // 2 --10:20--> 3 (duplicate non-epsilon)
      "2 4 30 40\n"  // 2 --30:40--> 4
      "3\n"          // state 3 final
      "4\n");        // state 4 final
  ASSERT_EQ(cyclic.Properties(kUnweighted, false), kUnweighted);

  // Build an equivalent reference whose stored kUnweighted bit is cleared so it
  // exercises the general ShortestDistanceState path.
  VectorFst<Arc> cyclic_ref(cyclic);
  cyclic_ref.SetProperties(0, kUnweighted);
  ASSERT_EQ(cyclic_ref.Properties(kUnweighted, false), 0);

  RmEpsilonFst<Arc> dfst(cyclic);
  RmEpsilonFst<Arc> dfst_ref(cyclic_ref);
  ASSERT_TRUE(Verify(dfst));
  EXPECT_TRUE(Equal(dfst_ref, dfst));
  ExpectStoredPropertiesCompatible(dfst);

  for (const bool connect : {true, false}) {
    VectorFst<Arc> vfst(cyclic);
    VectorFst<Arc> vfst_ref(cyclic_ref);
    RmEpsilon(&vfst, connect);
    RmEpsilon(&vfst_ref, connect);
    ASSERT_TRUE(Verify(vfst));
    EXPECT_TRUE(Equal(vfst_ref, vfst));
    ExpectStoredPropertiesCompatible(vfst);
  }

  // Non-top-sorted acyclic unweighted FST (exercises kAcyclic | kUnweighted in
  // both C++ RmEpsilon and script::RmEpsilon with AUTO_QUEUE).
  VectorFst<Arc> acyclic = CompileTextFst<Arc>(
      "0 2 0 0\n"  // 0 --eps--> 2 (out of topological order)
      "2 1 0 0\n"  // 2 --eps--> 1
      "0 1 0 0\n"  // 0 --eps--> 1
      "1 3 5 5\n"  // 1 --5:5--> 3
      "3\n");      // state 3 final
  acyclic.SetProperties(acyclic.Properties(kFstProperties, true),
                        kFstProperties);
  ASSERT_EQ(acyclic.Properties(kAcyclic | kUnweighted | kTopSorted, false),
            kAcyclic | kUnweighted);

  VectorFst<Arc> acyclic_ref(acyclic);
  acyclic_ref.SetProperties(kAcyclic, kAcyclic | kUnweighted);

  VectorFst<Arc> acyclic_out(acyclic);
  VectorFst<Arc> acyclic_ref_out(acyclic_ref);
  RmEpsilon(&acyclic_out);
  RmEpsilon(&acyclic_ref_out);
  ASSERT_TRUE(Verify(acyclic_out));
  EXPECT_TRUE(Equal(acyclic_ref_out, acyclic_out));
  ExpectStoredPropertiesCompatible(acyclic_out);

  script::VectorFstClass s_acyclic(acyclic);
  const script::WeightClass s_threshold =
      script::WeightClass::Zero(s_acyclic.WeightType());
  const script::RmEpsilonOptions s_opts(AUTO_QUEUE, true, s_threshold);
  script::RmEpsilon(&s_acyclic, s_opts);
  ASSERT_TRUE(script::Verify(s_acyclic));
  EXPECT_TRUE(
      script::Equal(script::VectorFstClass(acyclic_ref_out), s_acyclic));
}

TEST(RmEpsilonUnweightedTest, RightStringWeightUnweightedClosure) {
  using SArc = StringArc<STRING_RIGHT>;

  // Two parallel One()-weight epsilon paths merging duplicate non-epsilon arcs.
  VectorFst<SArc> input = CompileTextFst<SArc>(
      "0 1 0 0\n"  // 0 --eps--> 1
      "0 2 0 0\n"  // 0 --eps--> 2
      "1 3 7 8\n"  // 1 --7:8--> 3
      "2 3 7 8\n"  // 2 --7:8--> 3 (duplicate non-epsilon)
      "3\n");      // state 3 final
  ASSERT_EQ(input.Properties(kUnweighted, false), kUnweighted);

  const VectorFst<SArc> expected = CompileTextFst<SArc>(
      "0 1 7 8\n"  // 0 --7:8--> 1
      "1\n");      // state 1 final

  RmEpsilonFst<SArc> dfst(input);
  ASSERT_TRUE(Verify(dfst));
  VectorFst<SArc> dfst_connected(dfst);
  Connect(&dfst_connected);
  EXPECT_TRUE(Equal(expected, dfst_connected));

  RmEpsilon(&input);
  ASSERT_TRUE(Verify(input));
  EXPECT_TRUE(Equal(expected, input));
}

TEST(RmEpsilonUnweightedTest, ZeroWeightEpsilonNotTraversed) {
  VectorFst<Arc> input = CompileTextFst<Arc>(
      "0 1 0 0 Infinity\n"  // 0 --eps/Zero()--> 1 (dead epsilon branch)
      "1 3 0 0\n"           // 1 --eps/One()--> 3
      "0 3 2 2 Infinity\n"  // 0 --2:2/Zero()--> 3
      "0 2 0 0\n"           // 0 --eps/One()--> 2 (live epsilon branch)
      "2 3 2 2\n"           // 2 --2:2/One()--> 3 (sums with Zero() to One())
      "3 4 1 1\n"           // 3 --1:1/One()--> 4 (unreachable from 0 on eps)
      "3\n"                 // state 3 final
      "4\n");               // state 4 final
  ASSERT_EQ(input.Properties(kUnweighted, false), kUnweighted);

  // Copy with kUnweighted cleared to exercise the weighted ShortestDistance
  // path (both delayed OOB when state 3 > max visited state 2, and destructive
  // stale (*distance_)[3] when Expand(3) runs before Expand(0)).
  VectorFst<Arc> input_ref(input);
  input_ref.SetProperties(0, kUnweighted);
  ASSERT_EQ(input_ref.Properties(kUnweighted, false), 0);

  const VectorFst<Arc> expected = CompileTextFst<Arc>(
      "0 1 2 2\n"  // 0 --2:2/One()--> 1 (orig state 3)
      "1 2 1 1\n"  // 1 --1:1/One()--> 2 (orig state 4)
      "1\n"        // state 1 final
      "2\n");      // state 2 final

  // Exercise a delayed wrapper (NumStatesIfKnown() == std::nullopt) as well as
  // destructive VectorFst RmEpsilon on both unweighted and weighted paths.
  ArcMapFst<Arc, Arc, IdentityArcMapper<Arc>> delayed_input(
      input, IdentityArcMapper<Arc>());
  ArcMapFst<Arc, Arc, IdentityArcMapper<Arc>> delayed_input_ref(
      input_ref, IdentityArcMapper<Arc>());
  RmEpsilonFst<Arc> dfst(delayed_input);
  RmEpsilonFst<Arc> dfst_ref(delayed_input_ref);
  EXPECT_TRUE(Equal(dfst_ref, dfst));

  VectorFst<Arc> dfst_connected(dfst);
  Connect(&dfst_connected);
  ASSERT_TRUE(Verify(dfst_connected));
  EXPECT_TRUE(Equal(expected, dfst_connected));

  VectorFst<Arc> dfst_ref_connected(dfst_ref);
  Connect(&dfst_ref_connected);
  ASSERT_TRUE(Verify(dfst_ref_connected));
  EXPECT_TRUE(Equal(expected, dfst_ref_connected));

  RmEpsilon(&input);
  RmEpsilon(&input_ref);
  ASSERT_TRUE(Verify(input));
  ASSERT_TRUE(Verify(input_ref));
  EXPECT_TRUE(Equal(expected, input));
  EXPECT_TRUE(Equal(expected, input_ref));
}

TEST(RmEpsilonUnweightedTest, LeftStringWeightUnweightedReportsError) {
  const absl::FlagSaver fs;
  absl::SetFlag(&FLAGS_fst_error_fatal, false);

  // StringArc<STRING_LEFT> is idempotent (kIdempotent) but NOT right
  // distributive (kLeftSemiring only), so RmEpsilon must still report kError.
  using LSArc = StringArc<STRING_LEFT>;
  const VectorFst<LSArc> input = CompileTextFst<LSArc>(
      "0 1 0 0\n"  // 0 --eps--> 1
      "1 2 1 1\n"  // 1 --1:1--> 2
      "2\n");      // state 2 final
  ASSERT_EQ(input.Properties(kUnweighted, false), kUnweighted);

  RmEpsilonFst<LSArc> dfst(input);
  dfst.NumArcs(dfst.Start());
  EXPECT_EQ(dfst.Properties(kError, false), kError);

  VectorFst<LSArc> vfst(input);
  RmEpsilon(&vfst);
  EXPECT_EQ(vfst.Properties(kError, false), kError);
}

TEST(RmEpsilonUnweightedTest, UnweightedPropagatesDynamicError) {
  const absl::FlagSaver fs;
  absl::SetFlag(&FLAGS_fst_error_fatal, false);

  // Delayed FST advertising kUnweighted that yields a non-Member() weight on
  // expansion.
  const VectorFst<Arc> base = CompileTextFst<Arc>(
      "0 1 0 0\n"    // 0 --eps--> 1
      "1 2 5 5\n"    // 1 --5:5--> 2 (valid arc before bad arc)
      "1 2 99 99\n"  // 1 --99:99--> 2 (mapped to NoWeight() on expansion)
      "2\n");        // state 2 final
  ArcMapFst<Arc, Arc, InjectNonMemberMapper> bad_delayed(
      base, InjectNonMemberMapper(/*bad_ilabel=*/99, /*bad_nextstate=*/2));
  ASSERT_EQ(bad_delayed.Properties(kUnweighted, false), kUnweighted);

  RmEpsilonFst<Arc> dfst_bad(bad_delayed);
  EXPECT_EQ(dfst_bad.Properties(kError, false), 0);
  dfst_bad.NumArcs(dfst_bad.Start());
  EXPECT_EQ(dfst_bad.Properties(kError, false), kError);
  // Once error_ is set, subsequent expansions short-circuit with 0 arcs.
  EXPECT_EQ(dfst_bad.NumArcs(1), 0);
}

TEST(RmEpsilonUnweightedTest, UnweightedPropagatesDynamicEpsilonArcError) {
  const absl::FlagSaver fs;
  absl::SetFlag(&FLAGS_fst_error_fatal, false);

  // Delayed FST advertising kUnweighted that yields a non-Member() weight on
  // an epsilon arc: 0 --eps/NoWeight()--> 1 (final). A floating-point
  // NoWeight() is NaN, which compares unequal to Zero(), so the arc must be
  // rejected rather than traversed.
  const VectorFst<Arc> base = CompileTextFst<Arc>(
      "0 1 0 0\n"  // 0 --eps--> 1 (mapped to NoWeight() on expansion)
      "1\n");      // state 1 final
  ArcMapFst<Arc, Arc, InjectNonMemberMapper> bad_delayed(
      base, InjectNonMemberMapper(/*bad_ilabel=*/0, /*bad_nextstate=*/1));
  ASSERT_EQ(bad_delayed.Properties(kUnweighted, false), kUnweighted);

  RmEpsilonFst<Arc> dfst_bad(bad_delayed);
  dfst_bad.NumArcs(dfst_bad.Start());
  EXPECT_EQ(dfst_bad.Properties(kError, false), kError);
}

}  // namespace
}  // namespace fst
