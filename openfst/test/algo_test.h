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
// Regression test for various FST algorithms.

#ifndef OPENFST_TEST_ALGO_TEST_H_
#define OPENFST_TEST_ALGO_TEST_H_

#include <cstdint>
#include <memory>
#include <random>
#include <utility>
#include <vector>

#include "gtest/gtest.h"
#include "absl/flags/declare.h"
#include "absl/flags/flag.h"
#include "absl/log/check.h"
#include "absl/log/log.h"
#include "absl/status/status.h"
#include "openfst/lib/arc-map.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/arcfilter.h"
#include "openfst/lib/arcsort.h"
#include "openfst/lib/closure.h"
#include "openfst/lib/compose-filter.h"
#include "openfst/lib/compose.h"
#include "openfst/lib/concat.h"
#include "openfst/lib/connect.h"
#include "openfst/lib/determinize.h"
#include "openfst/lib/dfs-visit.h"
#include "openfst/lib/difference.h"
#include "openfst/lib/disambiguate.h"
#include "openfst/lib/encode.h"
#include "openfst/lib/equivalent.h"
#include "openfst/lib/float-weight.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/intersect.h"
#include "openfst/lib/invert.h"
#include "openfst/lib/lookahead-matcher.h"
#include "openfst/lib/matcher-fst.h"
#include "openfst/lib/matcher.h"
#include "openfst/lib/minimize.h"
#include "openfst/lib/mutable-fst.h"
#include "openfst/lib/pair-weight.h"
#include "openfst/lib/project.h"
#include "openfst/lib/properties.h"
#include "openfst/lib/prune.h"
#include "openfst/lib/push.h"
#include "openfst/lib/randequivalent.h"
#include "openfst/lib/randgen.h"
#include "openfst/lib/rational.h"
#include "openfst/lib/relabel.h"
#include "openfst/lib/reverse.h"
#include "openfst/lib/reweight.h"
#include "openfst/lib/rmepsilon.h"
#include "openfst/lib/shortest-distance.h"
#include "openfst/lib/shortest-path.h"
#include "openfst/lib/string-weight.h"
#include "openfst/lib/synchronize.h"
#include "openfst/lib/topsort.h"
#include "openfst/lib/union-weight.h"
#include "openfst/lib/union.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/lib/verify.h"
#include "openfst/lib/weight.h"
#include "openfst/test/rand-fst.h"

ABSL_DECLARE_FLAG(int32_t, repeat);
ABSL_DECLARE_FLAG(uint64_t, seed);

namespace fst {

// Mapper to change input and output label of every transition into
// epsilons.
template <class Arc>
class EpsMapper {
 public:
  EpsMapper() = default;

  Arc operator()(const Arc& arc) const {
    return Arc(0, 0, arc.weight, arc.nextstate);
  }

  uint64_t Properties(uint64_t props) const {
    props &= ~kNotAcceptor;
    props |= kAcceptor;
    props &= ~kNoIEpsilons & ~kNoOEpsilons & ~kNoEpsilons;
    props |= kIEpsilons | kOEpsilons | kEpsilons;
    props &= ~kNotILabelSorted & ~kNotOLabelSorted;
    props |= kILabelSorted | kOLabelSorted;
    return props;
  }

  MapFinalAction FinalAction() const { return MAP_NO_SUPERFINAL; }

  MapSymbolsAction InputSymbolsAction() const { return MAP_COPY_SYMBOLS; }

  MapSymbolsAction OutputSymbolsAction() const { return MAP_COPY_SYMBOLS; }
};

// Generic - no lookahead.
template <class Arc>
void LookAheadCompose(const Fst<Arc>& ifst1, const Fst<Arc>& ifst2,
                      MutableFst<Arc>* ofst) {
  Compose(ifst1, ifst2, ofst);
}

// Specialized and epsilon olabel acyclic - lookahead.
inline void LookAheadCompose(const Fst<StdArc>& ifst1, const Fst<StdArc>& ifst2,
                             MutableFst<StdArc>* ofst) {
  std::vector<StdArc::StateId> order;
  bool acyclic;
  TopOrderVisitor<StdArc> visitor(&order, &acyclic);
  DfsVisit(ifst1, &visitor, OutputEpsilonArcFilter<StdArc>());
  if (acyclic) {  // no ifst1 output epsilon cycles?
    StdOLabelLookAheadFst lfst1(ifst1);
    StdVectorFst lfst2(ifst2);
    LabelLookAheadRelabeler<StdArc>::Relabel(&lfst2, lfst1, true);
    Compose(lfst1, lfst2, ofst);
  } else {
    Compose(ifst1, ifst2, ofst);
  }
}

template <class Arc>
class AlgoTestBase : public ::testing::Test {
 public:
  using Label = typename Arc::Label;
  using StateId = typename Arc::StateId;
  using Weight = typename Arc::Weight;
  using WeightGenerator = WeightGenerate<Weight>;

  AlgoTestBase()
      : seed_(absl::GetFlag(FLAGS_seed)),
        rand_(seed_),
        generate_(seed_, /*allow_zero=*/false) {
    one_fst_.AddState();
    one_fst_.SetStart(0);
    one_fst_.SetFinal(0);

    univ_fst_.AddState();
    univ_fst_.SetStart(0);
    univ_fst_.SetFinal(0);
    for (int i = 0; i < kNumRandomLabels; ++i) univ_fst_.EmplaceArc(0, i, i, 0);
  }

  absl::Status MakeRandFst(MutableFst<Arc>* fst) {
    return RandFst(kNumRandomStates, kNumRandomArcs, kNumRandomLabels,
                   kAcyclicProb, generate_, rand_(), fst);
  }

  // Tests if two FSTs are equivalent by checking if random
  // strings from one FST are transduced the same by both FSTs.
  template <class A>  // Do not shadow `Arc`.
  bool Equiv(const Fst<A>& fst1, const Fst<A>& fst2) {
    VLOG(1) << "Check FSTs for sanity (including property bits).";
    EXPECT_TRUE(Verify(fst1));
    EXPECT_TRUE(Verify(fst2));

    // Ensures seed used once per instantiation.
    const UniformArcSelector<A> uniform_selector(this->seed_);
    const RandGenOptions<UniformArcSelector<A>> opts(uniform_selector,
                                                     this->kRandomPathLength);
    return RandEquivalent(fst1, fst2, this->kNumRandomPaths, opts,
                          this->kTestDelta, this->seed_);
  }

  // Tests FSA is unambiguous.
  bool Unambiguous(const Fst<Arc>& fst) {
    VectorFst<StdArc> sfst, dfst;
    VectorFst<LogArc> lfst1, lfst2;
    ArcMap(fst, &sfst, RmWeightMapper<Arc, StdArc>());
    Determinize(sfst, &dfst);
    ArcMap(fst, &lfst1, RmWeightMapper<Arc, LogArc>());
    ArcMap(dfst, &lfst2, RmWeightMapper<StdArc, LogArc>());
    return Equiv(lfst1, lfst2);
  }

  // Ensures input-epsilon free transducers fst1 and fst2 have the
  // same domain and that for each string pair '(is, os)' in fst1,
  // '(is, os)' is the minimum weight match to 'is' in fst2.
  bool MinRelated(const Fst<Arc>& fst1, const Fst<Arc>& fst2) {
    // Same domain
    VectorFst<Arc> P1(fst1), P2(fst2);
    Project(&P1, ProjectType::INPUT);
    Project(&P2, ProjectType::INPUT);
    if (!Equiv(P1, P2)) {
      LOG(ERROR) << "Inputs not equivalent";
      return false;
    }

    // Ensures seed used once per instantiation.
    const UniformArcSelector<Arc> uniform_selector(this->seed_);
    const RandGenOptions<UniformArcSelector<Arc>> opts(uniform_selector,
                                                       this->kRandomPathLength);

    VectorFst<Arc> path, paths1, paths2;
    for (int n = 0; n < kNumRandomPaths; ++n) {
      RandGen(fst1, &path, opts);
      Invert(&path);
      ArcMap(&path, RmWeightMapper<Arc>());
      Compose(path, fst2, &paths1);
      Weight sum1 = ShortestDistance(paths1);
      Compose(paths1, path, &paths2);
      Weight sum2 = ShortestDistance(paths2);
      if (!ApproxEqual(Plus(sum1, sum2), sum2, kTestDelta)) {
        LOG(ERROR) << "Sums not equivalent: " << sum1 << " " << sum2;
        return false;
      }
    }
    return true;
  }

  // Tests ShortestDistance(A - P) >= ShortestDistance(A) times Threshold.
  bool PruneEquiv(const Fst<Arc>& fst, const Fst<Arc>& pfst, Weight threshold) {
    VLOG(1) << "Check FSTs for sanity (including property bits).";
    EXPECT_TRUE(Verify(fst));
    EXPECT_TRUE(Verify(pfst));

    DifferenceFst<Arc> D(fst, DeterminizeFst<Arc>(RmEpsilonFst<Arc>(
                                  ArcMapFst(pfst, RmWeightMapper<Arc>()))));
    const Weight sum1 = Times(ShortestDistance(fst), threshold);
    const Weight sum2 = ShortestDistance(D);
    return ApproxEqual(Plus(sum1, sum2), sum1, kTestDelta);
  }

  // Tests if two FSAs are equivalent.
  bool FsaEquiv(const Fst<Arc>& fsa1, const Fst<Arc>& fsa2) {
    VLOG(1) << "Check FSAs for sanity (including property bits).";
    EXPECT_TRUE(Verify(fsa1));
    EXPECT_TRUE(Verify(fsa2));

    VectorFst<Arc> vfsa1(fsa1);
    VectorFst<Arc> vfsa2(fsa2);
    RmEpsilon(&vfsa1);
    RmEpsilon(&vfsa2);
    DeterminizeFst<Arc> dfa1(vfsa1);
    DeterminizeFst<Arc> dfa2(vfsa2);

    // Test equivalence using union-find algorithm
    bool equiv1 = Equivalent(dfa1, dfa2);

    // Test equivalence by checking if (S1 - S2) U (S2 - S1) is empty
    ILabelCompare<Arc> comp;
    VectorFst<Arc> sdfa1(dfa1);
    ArcSort(&sdfa1, comp);
    VectorFst<Arc> sdfa2(dfa2);
    ArcSort(&sdfa2, comp);

    DifferenceFst<Arc> dfsa1(sdfa1, sdfa2);
    DifferenceFst<Arc> dfsa2(sdfa2, sdfa1);

    VectorFst<Arc> ufsa(dfsa1);
    Union(&ufsa, dfsa2);
    Connect(&ufsa);
    bool equiv2 = ufsa.NumStates() == 0;

    // Checks both equivalence tests match.
    EXPECT_EQ(equiv1, equiv2);

    return equiv1;
  }

  // Tests if FSA1 is a subset of FSA2 (disregarding weights).
  bool Subset(const Fst<Arc>& fsa1, const Fst<Arc>& fsa2) {
    VLOG(1) << "Check FSAs (incl. property bits) for sanity";
    EXPECT_TRUE(Verify(fsa1));
    EXPECT_TRUE(Verify(fsa2));

    VectorFst<StdArc> vfsa1;
    VectorFst<StdArc> vfsa2;
    ArcMap(fsa1, &vfsa1, RmWeightMapper<Arc, StdArc>());
    ArcMap(fsa2, &vfsa2, RmWeightMapper<Arc, StdArc>());
    RmEpsilon(&vfsa1);
    RmEpsilon(&vfsa2);
    ILabelCompare<StdArc> comp;
    ArcSort(&vfsa1, comp);
    ArcSort(&vfsa2, comp);
    IntersectFst<StdArc> ifsa(vfsa1, vfsa2);
    DeterminizeFst<StdArc> dfa1(vfsa1);
    DeterminizeFst<StdArc> dfa2(ifsa);
    return Equivalent(dfa1, dfa2);
  }

  // Returns complement FSA.
  void Complement(const Fst<Arc>& ifsa, MutableFst<Arc>* ofsa) {
    RmEpsilonFst<Arc> rfsa(ifsa);
    DeterminizeFst<Arc> dfa(rfsa);
    DifferenceFst<Arc> cfsa(univ_fst_, dfa);
    *ofsa = cfsa;
  }

 protected:
  // Random seed.
  uint64_t seed_;
  // Random state.
  std::mt19937_64 rand_;
  // Generates weights used in testing.
  WeightGenerator generate_;
  // FST with no states.
  VectorFst<Arc> zero_fst_;
  // FST with one state that accepts epsilon.
  VectorFst<Arc> one_fst_;
  // FST with one state that accepts all strings.
  VectorFst<Arc> univ_fst_;

  // Maximum number of states in random test Fst.
  static constexpr int kNumRandomStates = 10;
  // Maximum number of arcs in random test Fst.
  static constexpr int kNumRandomArcs = 25;
  // Number of alternative random labels.
  static constexpr int kNumRandomLabels = 5;
  // Probability to force an acyclic Fst.
  static constexpr float kAcyclicProb = .25;
  // Maximum random path length.
  static constexpr int kRandomPathLength = 25;
  // Number of random paths to explore.
  static constexpr int kNumRandomPaths = 100;
  // Maximum number of nshortest paths.
  static constexpr int kNumRandomShortestPaths = 100;
  // Maximum number of nshortest states.
  static constexpr int kNumShortestStates = 10000;
  // Delta for equivalence tests.
  static constexpr float kTestDelta = .05;
  // Delta for algorithm configuration.
  static constexpr float kDelta = 1.0e-4;
};

template <class Arc>
class RationalTest : public AlgoTestBase<Arc> {};
TYPED_TEST_SUITE_P(RationalTest);

TYPED_TEST_P(RationalTest, UnionEquivalent) {
  using Arc = TypeParam;
  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    VectorFst<Arc> T1, T2;
    CHECK_OK(this->MakeRandFst(&T1));
    CHECK_OK(this->MakeRandFst(&T2));
    VLOG(1) << "Check destructive and delayed union are equivalent.";
    VectorFst<Arc> U1(T1);
    Union(&U1, T2);
    UnionFst<Arc> U2(T1, T2);
    EXPECT_TRUE(this->Equiv(U1, U2));
  }
}

TYPED_TEST_P(RationalTest, ConcatEquivalent) {
  using Arc = TypeParam;
  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    VectorFst<Arc> T1, T2;
    CHECK_OK(this->MakeRandFst(&T1));
    CHECK_OK(this->MakeRandFst(&T2));
    VLOG(1) << "Check destructive and delayed concatenation are equivalent.";
    VectorFst<Arc> C1(T1);
    Concat(&C1, T2);
    ConcatFst<Arc> C2(T1, T2);
    EXPECT_TRUE(this->Equiv(C1, C2));
    VectorFst<Arc> C3(T2);
    Concat(T1, &C3);
    EXPECT_TRUE(this->Equiv(C3, C2));
  }
}

TYPED_TEST_P(RationalTest, ClosureStarEquivalent) {
  using Arc = TypeParam;
  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    VectorFst<Arc> T1;
    CHECK_OK(this->MakeRandFst(&T1));
    VLOG(1) << "Check destructive and delayed closure* are equivalent.";
    VectorFst<Arc> C1(T1);
    Closure(&C1, CLOSURE_STAR);
    ClosureFst<Arc> C2(T1, CLOSURE_STAR);
    EXPECT_TRUE(this->Equiv(C1, C2));
  }
}

TYPED_TEST_P(RationalTest, ClosurePlusEquivalent) {
  using Arc = TypeParam;
  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    VectorFst<Arc> T1;
    CHECK_OK(this->MakeRandFst(&T1));
    VLOG(1) << "Check destructive and delayed closure+ are equivalent.";
    VectorFst<Arc> C1(T1);
    Closure(&C1, CLOSURE_PLUS);
    ClosureFst<Arc> C2(T1, CLOSURE_PLUS);
    EXPECT_TRUE(this->Equiv(C1, C2));
  }
}

TYPED_TEST_P(RationalTest, UnionAssociativeDestructive) {
  using Arc = TypeParam;
  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    VectorFst<Arc> T1, T2, T3;
    CHECK_OK(this->MakeRandFst(&T1));
    CHECK_OK(this->MakeRandFst(&T2));
    CHECK_OK(this->MakeRandFst(&T3));
    VLOG(1) << "Check union is associative (destructive).";
    VectorFst<Arc> U1(T1);
    Union(&U1, T2);
    Union(&U1, T3);

    VectorFst<Arc> U3(T2);
    Union(&U3, T3);
    VectorFst<Arc> U4(T1);
    Union(&U4, U3);

    EXPECT_TRUE(this->Equiv(U1, U4));
  }
}

TYPED_TEST_P(RationalTest, UnionAssociativeDelayed) {
  using Arc = TypeParam;
  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    VectorFst<Arc> T1, T2, T3;
    CHECK_OK(this->MakeRandFst(&T1));
    CHECK_OK(this->MakeRandFst(&T2));
    CHECK_OK(this->MakeRandFst(&T3));
    VLOG(1) << "Check union is associative (delayed).";
    UnionFst<Arc> U1(T1, T2);
    UnionFst<Arc> U2(U1, T3);

    UnionFst<Arc> U3(T2, T3);
    UnionFst<Arc> U4(T1, U3);

    EXPECT_TRUE(this->Equiv(U2, U4));
  }
}

TYPED_TEST_P(RationalTest, UnionAssociativeDestructiveDelayed) {
  using Arc = TypeParam;
  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    VectorFst<Arc> T1, T2, T3;
    CHECK_OK(this->MakeRandFst(&T1));
    CHECK_OK(this->MakeRandFst(&T2));
    CHECK_OK(this->MakeRandFst(&T3));
    VLOG(1) << "Check union is associative (destructive delayed).";
    UnionFst<Arc> U1(T1, T2);
    Union(&U1, T3);

    UnionFst<Arc> U3(T2, T3);
    UnionFst<Arc> U4(T1, U3);

    EXPECT_TRUE(this->Equiv(U1, U4));
  }
}

TYPED_TEST_P(RationalTest, ConcatAssociativeDestructive) {
  using Arc = TypeParam;
  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    VectorFst<Arc> T1, T2, T3;
    CHECK_OK(this->MakeRandFst(&T1));
    CHECK_OK(this->MakeRandFst(&T2));
    CHECK_OK(this->MakeRandFst(&T3));
    VLOG(1) << "Check concatenation is associative (destructive).";
    VectorFst<Arc> C1(T1);
    Concat(&C1, T2);
    Concat(&C1, T3);

    VectorFst<Arc> C3(T2);
    Concat(&C3, T3);
    VectorFst<Arc> C4(T1);
    Concat(&C4, C3);

    EXPECT_TRUE(this->Equiv(C1, C4));
  }
}

TYPED_TEST_P(RationalTest, ConcatAssociativeDelayed) {
  using Arc = TypeParam;
  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    VectorFst<Arc> T1, T2, T3;
    CHECK_OK(this->MakeRandFst(&T1));
    CHECK_OK(this->MakeRandFst(&T2));
    CHECK_OK(this->MakeRandFst(&T3));
    VLOG(1) << "Check concatenation is associative (delayed).";
    ConcatFst<Arc> C1(T1, T2);
    ConcatFst<Arc> C2(C1, T3);

    ConcatFst<Arc> C3(T2, T3);
    ConcatFst<Arc> C4(T1, C3);

    EXPECT_TRUE(this->Equiv(C2, C4));
  }
}

TYPED_TEST_P(RationalTest, ConcatAssociativeDestructiveDelayed) {
  using Arc = TypeParam;
  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    VectorFst<Arc> T1, T2, T3;
    CHECK_OK(this->MakeRandFst(&T1));
    CHECK_OK(this->MakeRandFst(&T2));
    CHECK_OK(this->MakeRandFst(&T3));
    VLOG(1) << "Check concatenation is associative (destructive delayed).";
    ConcatFst<Arc> C1(T1, T2);
    Concat(&C1, T3);

    ConcatFst<Arc> C3(T2, T3);
    ConcatFst<Arc> C4(T1, C3);

    EXPECT_TRUE(this->Equiv(C1, C4));
  }
}

TYPED_TEST_P(RationalTest, ConcatLeftDistributesUnionDestructive) {
  using Arc = TypeParam;
  using Weight = typename Arc::Weight;
  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    VectorFst<Arc> T1, T2, T3;
    CHECK_OK(this->MakeRandFst(&T1));
    CHECK_OK(this->MakeRandFst(&T2));
    CHECK_OK(this->MakeRandFst(&T3));
    if (Weight::Properties() & kLeftSemiring) {
      VLOG(1)
          << "Check concatenation left distributes over union (destructive).";
      VectorFst<Arc> U1(T1);
      Union(&U1, T2);
      VectorFst<Arc> C1(T3);
      Concat(&C1, U1);

      VectorFst<Arc> C2(T3);
      Concat(&C2, T1);
      VectorFst<Arc> C3(T3);
      Concat(&C3, T2);
      VectorFst<Arc> U2(C2);
      Union(&U2, C3);

      EXPECT_TRUE(this->Equiv(C1, U2));
    }
  }
}

TYPED_TEST_P(RationalTest, ConcatRightDistributesUnionDestructive) {
  using Arc = TypeParam;
  using Weight = typename Arc::Weight;
  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    VectorFst<Arc> T1, T2, T3;
    CHECK_OK(this->MakeRandFst(&T1));
    CHECK_OK(this->MakeRandFst(&T2));
    CHECK_OK(this->MakeRandFst(&T3));
    if (Weight::Properties() & kRightSemiring) {
      VLOG(1)
          << "Check concatenation right distributes over union (destructive).";
      VectorFst<Arc> U1(T1);
      Union(&U1, T2);
      VectorFst<Arc> C1(U1);
      Concat(&C1, T3);

      VectorFst<Arc> C2(T1);
      Concat(&C2, T3);
      VectorFst<Arc> C3(T2);
      Concat(&C3, T3);
      VectorFst<Arc> U2(C2);
      Union(&U2, C3);

      EXPECT_TRUE(this->Equiv(C1, U2));
    }
  }
}

TYPED_TEST_P(RationalTest, ConcatLeftDistributesUnionDelayed) {
  using Arc = TypeParam;
  using Weight = typename Arc::Weight;
  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    VectorFst<Arc> T1, T2, T3;
    CHECK_OK(this->MakeRandFst(&T1));
    CHECK_OK(this->MakeRandFst(&T2));
    CHECK_OK(this->MakeRandFst(&T3));
    if (Weight::Properties() & kLeftSemiring) {
      VLOG(1) << "Check concatenation left distributes over union (delayed).";
      UnionFst<Arc> U1(T1, T2);
      ConcatFst<Arc> C1(T3, U1);

      ConcatFst<Arc> C2(T3, T1);
      ConcatFst<Arc> C3(T3, T2);
      UnionFst<Arc> U2(C2, C3);

      EXPECT_TRUE(this->Equiv(C1, U2));
    }
  }
}

TYPED_TEST_P(RationalTest, ConcatRightDistributesUnionDelayed) {
  using Arc = TypeParam;
  using Weight = typename Arc::Weight;
  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    VectorFst<Arc> T1, T2, T3;
    CHECK_OK(this->MakeRandFst(&T1));
    CHECK_OK(this->MakeRandFst(&T2));
    CHECK_OK(this->MakeRandFst(&T3));
    if (Weight::Properties() & kRightSemiring) {
      VLOG(1) << "Check concatenation right distributes over union (delayed).";
      UnionFst<Arc> U1(T1, T2);
      ConcatFst<Arc> C1(U1, T3);

      ConcatFst<Arc> C2(T1, T3);
      ConcatFst<Arc> C3(T2, T3);
      UnionFst<Arc> U2(C2, C3);

      EXPECT_TRUE(this->Equiv(C1, U2));
    }
  }
}

TYPED_TEST_P(RationalTest, TTStarDestructive) {
  using Arc = TypeParam;
  using Weight = typename Arc::Weight;
  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    VectorFst<Arc> T1;
    CHECK_OK(this->MakeRandFst(&T1));
    if (Weight::Properties() & kLeftSemiring) {
      VLOG(1) << "Check T T* == T+ (destructive).";
      VectorFst<Arc> S(T1);
      Closure(&S, CLOSURE_STAR);
      VectorFst<Arc> C(T1);
      Concat(&C, S);

      VectorFst<Arc> P(T1);
      Closure(&P, CLOSURE_PLUS);

      EXPECT_TRUE(this->Equiv(C, P));
    }
  }
}

TYPED_TEST_P(RationalTest, TStarTDestructive) {
  using Arc = TypeParam;
  using Weight = typename Arc::Weight;
  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    VectorFst<Arc> T1;
    CHECK_OK(this->MakeRandFst(&T1));
    if (Weight::Properties() & kRightSemiring) {
      VLOG(1) << "Check T* T == T+ (destructive).";
      VectorFst<Arc> S(T1);
      Closure(&S, CLOSURE_STAR);
      VectorFst<Arc> C(S);
      Concat(&C, T1);

      VectorFst<Arc> P(T1);
      Closure(&P, CLOSURE_PLUS);

      EXPECT_TRUE(this->Equiv(C, P));
    }
  }
}

TYPED_TEST_P(RationalTest, TTStarDelayed) {
  using Arc = TypeParam;
  using Weight = typename Arc::Weight;
  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    VectorFst<Arc> T1;
    CHECK_OK(this->MakeRandFst(&T1));
    if (Weight::Properties() & kLeftSemiring) {
      VLOG(1) << "Check T T* == T+ (delayed).";
      ClosureFst<Arc> S(T1, CLOSURE_STAR);
      ConcatFst<Arc> C(T1, S);

      ClosureFst<Arc> P(T1, CLOSURE_PLUS);

      EXPECT_TRUE(this->Equiv(C, P));
    }
  }
}

TYPED_TEST_P(RationalTest, TStarTDelayed) {
  using Arc = TypeParam;
  using Weight = typename Arc::Weight;
  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    VectorFst<Arc> T1;
    CHECK_OK(this->MakeRandFst(&T1));
    if (Weight::Properties() & kRightSemiring) {
      VLOG(1) << "Check T* T == T+ (delayed).";
      ClosureFst<Arc> S(T1, CLOSURE_STAR);
      ConcatFst<Arc> C(S, T1);

      ClosureFst<Arc> P(T1, CLOSURE_PLUS);

      EXPECT_TRUE(this->Equiv(C, P));
    }
  }
}

template <class Arc>
class MapTest : public AlgoTestBase<Arc> {};
TYPED_TEST_SUITE_P(MapTest);

TYPED_TEST_P(MapTest, ProjectEquivalent) {
  using Arc = TypeParam;
  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    VectorFst<Arc> T;
    CHECK_OK(this->MakeRandFst(&T));
    VLOG(1) << "Check destructive and delayed projection are equivalent.";
    VectorFst<Arc> P1(T);
    Project(&P1, ProjectType::INPUT);
    ProjectFst<Arc> P2(T, ProjectType::INPUT);
    EXPECT_TRUE(this->Equiv(P1, P2));
  }
}

TYPED_TEST_P(MapTest, InvertEquivalent) {
  using Arc = TypeParam;
  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    VectorFst<Arc> T;
    CHECK_OK(this->MakeRandFst(&T));
    VLOG(1) << "Check destructive and delayed inversion are equivalent.";
    VectorFst<Arc> I1(T);
    Invert(&I1);
    InvertFst<Arc> I2(T);
    EXPECT_TRUE(this->Equiv(I1, I2));
  }
}

TYPED_TEST_P(MapTest, IdentityDestructivePi1) {
  using Arc = TypeParam;
  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    VectorFst<Arc> T;
    CHECK_OK(this->MakeRandFst(&T));
    VLOG(1) << "Check Pi_1(T) = Pi_2(T^-1) (destructive).";
    VectorFst<Arc> P1(T);
    VectorFst<Arc> I1(T);
    Project(&P1, ProjectType::INPUT);
    Invert(&I1);
    Project(&I1, ProjectType::OUTPUT);
    EXPECT_TRUE(this->Equiv(P1, I1));
  }
}

TYPED_TEST_P(MapTest, IdentityDestructivePi2) {
  using Arc = TypeParam;
  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    VectorFst<Arc> T;
    CHECK_OK(this->MakeRandFst(&T));
    VLOG(1) << "Check Pi_2(T) = Pi_1(T^-1) (destructive).";
    VectorFst<Arc> P1(T);
    VectorFst<Arc> I1(T);
    Project(&P1, ProjectType::OUTPUT);
    Invert(&I1);
    Project(&I1, ProjectType::INPUT);
    EXPECT_TRUE(this->Equiv(P1, I1));
  }
}

TYPED_TEST_P(MapTest, IdentityDelayedPi1) {
  using Arc = TypeParam;
  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    VectorFst<Arc> T;
    CHECK_OK(this->MakeRandFst(&T));
    VLOG(1) << "Check Pi_1(T) = Pi_2(T^-1) (delayed).";
    ProjectFst<Arc> P1(T, ProjectType::INPUT);
    InvertFst<Arc> I1(T);
    ProjectFst<Arc> P2(I1, ProjectType::OUTPUT);
    EXPECT_TRUE(this->Equiv(P1, P2));
  }
}

TYPED_TEST_P(MapTest, IdentityDelayedPi2) {
  using Arc = TypeParam;
  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    VectorFst<Arc> T;
    CHECK_OK(this->MakeRandFst(&T));
    VLOG(1) << "Check Pi_2(T) = Pi_1(T^-1) (delayed).";
    ProjectFst<Arc> P1(T, ProjectType::OUTPUT);
    InvertFst<Arc> I1(T);
    ProjectFst<Arc> P2(I1, ProjectType::INPUT);
    EXPECT_TRUE(this->Equiv(P1, P2));
  }
}

TYPED_TEST_P(MapTest, RelabelDestructive) {
  using Arc = TypeParam;
  using Label = typename Arc::Label;
  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    VectorFst<Arc> T;
    CHECK_OK(this->MakeRandFst(&T));
    VLOG(1) << "Check destructive relabeling";
    static const int kNumLabels = 10;
    // set up relabeling pairs
    std::vector<Label> labelset(kNumLabels);
    for (int j = 0; j < kNumLabels; ++j) labelset[j] = j;
    for (int j = 0; j < kNumLabels; ++j) {
      using std::swap;
      const auto index =
          std::uniform_int_distribution<>(0, kNumLabels - 1)(this->rand_);
      swap(labelset[j], labelset[index]);
    }

    std::vector<std::pair<Label, Label>> ipairs1(kNumLabels);
    std::vector<std::pair<Label, Label>> opairs1(kNumLabels);
    for (int j = 0; j < kNumLabels; ++j) {
      ipairs1[j] = std::make_pair(j, labelset[j]);
      opairs1[j] = std::make_pair(labelset[j], j);
    }
    VectorFst<Arc> R(T);
    Relabel(&R, ipairs1, opairs1);

    std::vector<std::pair<Label, Label>> ipairs2(kNumLabels);
    std::vector<std::pair<Label, Label>> opairs2(kNumLabels);
    for (int j = 0; j < kNumLabels; ++j) {
      ipairs2[j] = std::make_pair(labelset[j], j);
      opairs2[j] = std::make_pair(j, labelset[j]);
    }
    Relabel(&R, ipairs2, opairs2);
    EXPECT_TRUE(this->Equiv(R, T));
  }
}

TYPED_TEST_P(MapTest, RelabelDelayed) {
  using Arc = TypeParam;
  using Label = typename Arc::Label;
  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    VectorFst<Arc> T;
    CHECK_OK(this->MakeRandFst(&T));
    static const int kNumLabels = 10;
    // set up relabeling pairs
    std::vector<Label> labelset(kNumLabels);
    for (int j = 0; j < kNumLabels; ++j) labelset[j] = j;
    for (int j = 0; j < kNumLabels; ++j) {
      using std::swap;
      const auto index =
          std::uniform_int_distribution<>(0, kNumLabels - 1)(this->rand_);
      swap(labelset[j], labelset[index]);
    }

    std::vector<std::pair<Label, Label>> ipairs1(kNumLabels);
    std::vector<std::pair<Label, Label>> opairs1(kNumLabels);
    for (int j = 0; j < kNumLabels; ++j) {
      ipairs1[j] = std::make_pair(j, labelset[j]);
      opairs1[j] = std::make_pair(labelset[j], j);
    }
    std::vector<std::pair<Label, Label>> ipairs2(kNumLabels);
    std::vector<std::pair<Label, Label>> opairs2(kNumLabels);
    for (int j = 0; j < kNumLabels; ++j) {
      ipairs2[j] = std::make_pair(labelset[j], j);
      opairs2[j] = std::make_pair(j, labelset[j]);
    }
    VLOG(1) << "Check on-the-fly relabeling";
    RelabelFst<Arc> Rdelay(T, ipairs1, opairs1);

    RelabelFst<Arc> RRdelay(Rdelay, ipairs2, opairs2);
    EXPECT_TRUE(this->Equiv(RRdelay, T));
  }
}

TYPED_TEST_P(MapTest, EncodeDestructive) {
  using Arc = TypeParam;
  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    VectorFst<Arc> T;
    CHECK_OK(this->MakeRandFst(&T));
    uint8_t encode_props = 0;
    if (std::bernoulli_distribution(.5)(this->rand_))
      encode_props |= kEncodeLabels;
    if (std::bernoulli_distribution(.5)(this->rand_))
      encode_props |= kEncodeWeights;
    VLOG(1) << "Check encoding/decoding (destructive).";
    VectorFst<Arc> D(T);
    EncodeMapper<Arc> encoder(encode_props, ENCODE);
    Encode(&D, &encoder);
    Decode(&D, encoder);
    EXPECT_TRUE(this->Equiv(D, T));
  }
}

TYPED_TEST_P(MapTest, EncodeDelayed) {
  using Arc = TypeParam;
  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    VectorFst<Arc> T;
    CHECK_OK(this->MakeRandFst(&T));
    uint8_t encode_props = 0;
    if (std::bernoulli_distribution(.5)(this->rand_))
      encode_props |= kEncodeLabels;
    if (std::bernoulli_distribution(.5)(this->rand_))
      encode_props |= kEncodeWeights;
    VLOG(1) << "Check encoding/decoding (delayed).";
    EncodeMapper<Arc> encoder(encode_props, ENCODE);
    EncodeFst<Arc> E(T, &encoder);
    VectorFst<Arc> Encoded(E);
    DecodeFst<Arc> D(Encoded, encoder);
    EXPECT_TRUE(this->Equiv(D, T));
  }
}

TYPED_TEST_P(MapTest, GallicConstructive) {
  using Arc = TypeParam;
  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    VectorFst<Arc> T;
    CHECK_OK(this->MakeRandFst(&T));
    VLOG(1) << "Check gallic mappers (constructive).";
    ToGallicMapper<Arc> to_mapper;
    FromGallicMapper<Arc> from_mapper;
    VectorFst<GallicArc<Arc>> G;
    VectorFst<Arc> F;
    ArcMap(T, &G, to_mapper);
    ArcMap(G, &F, from_mapper);
    EXPECT_TRUE(this->Equiv(T, F));
  }
}

TYPED_TEST_P(MapTest, GallicDelayed) {
  using Arc = TypeParam;
  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    VectorFst<Arc> T;
    CHECK_OK(this->MakeRandFst(&T));
    VLOG(1) << "Check gallic mappers (delayed).";
    ArcMapFst G(T, ToGallicMapper<Arc>());
    ArcMapFst F(G, FromGallicMapper<Arc>());
    EXPECT_TRUE(this->Equiv(T, F));
  }
}

template <class Arc>
class ComposeTest : public AlgoTestBase<Arc> {};
TYPED_TEST_SUITE_P(ComposeTest);

TYPED_TEST_P(ComposeTest, Associative) {
  using Arc = TypeParam;
  using Weight = typename Arc::Weight;
  if (!(Weight::Properties() & kCommutative)) return;
  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    VectorFst<Arc> T1, T2, T3;
    CHECK_OK(this->MakeRandFst(&T1));
    CHECK_OK(this->MakeRandFst(&T2));
    CHECK_OK(this->MakeRandFst(&T3));
    VectorFst<Arc> S1(T1), S2(T2), S3(T3);
    ILabelCompare<Arc> icomp;
    OLabelCompare<Arc> ocomp;

    ArcSort(&S1, ocomp);
    ArcSort(&S2, ocomp);
    ArcSort(&S3, icomp);
    VLOG(1) << "Check composition is associative.";
    ComposeFst<Arc> C1(S1, S2);
    ComposeFst<Arc> C2(C1, S3);
    ComposeFst<Arc> C3(S2, S3);
    ComposeFst<Arc> C4(S1, C3);

    EXPECT_TRUE(this->Equiv(C2, C4));
  }
}

TYPED_TEST_P(ComposeTest, LeftDistributesUnion) {
  using Arc = TypeParam;
  using Weight = typename Arc::Weight;
  if (!(Weight::Properties() & kCommutative)) return;
  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    VectorFst<Arc> T1, T2, T3;
    CHECK_OK(this->MakeRandFst(&T1));
    CHECK_OK(this->MakeRandFst(&T2));
    CHECK_OK(this->MakeRandFst(&T3));
    VectorFst<Arc> S1(T1), S2(T2), S3(T3);
    ILabelCompare<Arc> icomp;
    OLabelCompare<Arc> ocomp;

    ArcSort(&S1, ocomp);
    ArcSort(&S2, ocomp);
    ArcSort(&S3, icomp);
    VLOG(1) << "Check composition left distributes over union.";
    UnionFst<Arc> U1(S2, S3);
    ComposeFst<Arc> C1(S1, U1);

    ComposeFst<Arc> C2(S1, S2);
    ComposeFst<Arc> C3(S1, S3);
    UnionFst<Arc> U2(C2, C3);

    EXPECT_TRUE(this->Equiv(C1, U2));
  }
}

TYPED_TEST_P(ComposeTest, RightDistributesUnion) {
  using Arc = TypeParam;
  using Weight = typename Arc::Weight;
  if (!(Weight::Properties() & kCommutative)) return;
  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    VectorFst<Arc> T1, T2, T3;
    CHECK_OK(this->MakeRandFst(&T1));
    CHECK_OK(this->MakeRandFst(&T2));
    CHECK_OK(this->MakeRandFst(&T3));
    VectorFst<Arc> S1(T1), S2(T2), S3(T3);
    ILabelCompare<Arc> icomp;
    OLabelCompare<Arc> ocomp;

    ArcSort(&S1, ocomp);
    ArcSort(&S2, ocomp);
    ArcSort(&S3, icomp);
    VLOG(1) << "Check composition right distributes over union.";
    UnionFst<Arc> U1(S1, S2);
    ComposeFst<Arc> C1(U1, S3);

    ComposeFst<Arc> C2(S1, S3);
    ComposeFst<Arc> C3(S2, S3);
    UnionFst<Arc> U2(C2, C3);

    EXPECT_TRUE(this->Equiv(C1, U2));
  }
}

TYPED_TEST_P(ComposeTest, IntersectionCommutative) {
  using Arc = TypeParam;
  using Weight = typename Arc::Weight;
  if (!(Weight::Properties() & kCommutative)) return;
  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    VectorFst<Arc> T1, T2;
    CHECK_OK(this->MakeRandFst(&T1));
    CHECK_OK(this->MakeRandFst(&T2));
    VectorFst<Arc> A1(T1), A2(T2);
    Project(&A1, ProjectType::OUTPUT);
    Project(&A2, ProjectType::INPUT);
    ILabelCompare<Arc> icomp;
    ArcSort(&A1, icomp);
    ArcSort(&A2, icomp);
    VLOG(1) << "Check intersection is commutative.";
    IntersectFst<Arc> I1(A1, A2);
    IntersectFst<Arc> I2(A2, A1);
    EXPECT_TRUE(this->Equiv(I1, I2));
  }
}

TYPED_TEST_P(ComposeTest, FiltersEpsilon) {
  using Arc = TypeParam;
  using Weight = typename Arc::Weight;
  if (!(Weight::Properties() & kCommutative)) return;
  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    VectorFst<Arc> T1, T2;
    CHECK_OK(this->MakeRandFst(&T1));
    CHECK_OK(this->MakeRandFst(&T2));
    VectorFst<Arc> S1(T1), S2(T2);
    OLabelCompare<Arc> ocomp;

    ArcSort(&S1, ocomp);
    using M = Matcher<Fst<Arc>>;
    ComposeFst<Arc> C1(S1, S2);
    VLOG(1) << "Check all epsilon filters lead to equivalent results.";
    ComposeFst<Arc> C2(
        S1, S2, ComposeFstOptions<Arc, M, AltSequenceComposeFilter<M>>());
    ComposeFst<Arc> C3(S1, S2,
                       ComposeFstOptions<Arc, M, MatchComposeFilter<M>>());

    EXPECT_TRUE(this->Equiv(C1, C2));
    EXPECT_TRUE(this->Equiv(C1, C3));

    if ((Weight::Properties() & kIdempotent) ||
        S1.Properties(kNoOEpsilons, false) ||
        S2.Properties(kNoIEpsilons, false)) {
      ComposeFst<Arc> C4(S1, S2,
                         ComposeFstOptions<Arc, M, TrivialComposeFilter<M>>());
      EXPECT_TRUE(this->Equiv(C1, C4));
      ComposeFst<Arc> C5(S1, S2,
                         ComposeFstOptions<Arc, M, NoMatchComposeFilter<M>>());
      EXPECT_TRUE(this->Equiv(C1, C5));
    }
  }
}

TYPED_TEST_P(ComposeTest, FiltersNull) {
  using Arc = TypeParam;
  using Weight = typename Arc::Weight;
  if (!(Weight::Properties() & kCommutative)) return;
  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    VectorFst<Arc> T1, T2;
    CHECK_OK(this->MakeRandFst(&T1));
    CHECK_OK(this->MakeRandFst(&T2));
    VectorFst<Arc> S1(T1), S2(T2);
    OLabelCompare<Arc> ocomp;

    ArcSort(&S1, ocomp);
    using M = Matcher<Fst<Arc>>;
    ComposeFst<Arc> C1(S1, S2);
    if (S1.Properties(kNoOEpsilons, false) &&
        S2.Properties(kNoIEpsilons, false)) {
      ComposeFst<Arc> C6(S1, S2,
                         ComposeFstOptions<Arc, M, NullComposeFilter<M>>());
      EXPECT_TRUE(this->Equiv(C1, C6));
    }
  }
}

TYPED_TEST_P(ComposeTest, LookAhead) {
  using Arc = TypeParam;
  using Weight = typename Arc::Weight;
  if (!(Weight::Properties() & kCommutative)) return;
  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    VectorFst<Arc> T1, T2;
    CHECK_OK(this->MakeRandFst(&T1));
    CHECK_OK(this->MakeRandFst(&T2));
    VectorFst<Arc> S1(T1), S2(T2);
    OLabelCompare<Arc> ocomp;

    ArcSort(&S1, ocomp);
    VLOG(1) << "Check look-ahead filters lead to equivalent results.";
    VectorFst<Arc> C1, C2;
    Compose(S1, S2, &C1);
    LookAheadCompose(S1, S2, &C2);
    EXPECT_TRUE(this->Equiv(C1, C2));
  }
}

template <class Arc>
class SortTest : public AlgoTestBase<Arc> {};
TYPED_TEST_SUITE_P(SortTest);

TYPED_TEST_P(SortTest, ArcSortedEquivalent) {
  using Arc = TypeParam;
  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    VectorFst<Arc> T;
    CHECK_OK(this->MakeRandFst(&T));
    ILabelCompare<Arc> icomp;
    VectorFst<Arc> S1(T);
    VLOG(1) << "Check arc sorted Fst is equivalent to its input.";
    ArcSort(&S1, icomp);
    EXPECT_TRUE(this->Equiv(T, S1));
  }
}

TYPED_TEST_P(SortTest, DestructiveDelayedEquivalent) {
  using Arc = TypeParam;
  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    VectorFst<Arc> T;
    CHECK_OK(this->MakeRandFst(&T));
    ILabelCompare<Arc> icomp;
    VectorFst<Arc> S1(T);
    ArcSort(&S1, icomp);
    VLOG(1) << "Check destructive and delayed arcsort are equivalent.";
    ArcSortFst<Arc, ILabelCompare<Arc>> S2(T, icomp);
    EXPECT_TRUE(this->Equiv(S1, S2));
  }
}

TYPED_TEST_P(SortTest, InversionsIdentity) {
  using Arc = TypeParam;
  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    VectorFst<Arc> T;
    CHECK_OK(this->MakeRandFst(&T));
    ILabelCompare<Arc> icomp;
    OLabelCompare<Arc> ocomp;
    VLOG(1) << "Check ilabel sorting vs. olabel sorting with inversions.";
    VectorFst<Arc> S1(T);
    VectorFst<Arc> S2(T);
    ArcSort(&S1, icomp);
    Invert(&S2);
    ArcSort(&S2, ocomp);
    Invert(&S2);
    EXPECT_TRUE(this->Equiv(S1, S2));
  }
}

TYPED_TEST_P(SortTest, TopologicallySortedEquivalent) {
  using Arc = TypeParam;
  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    VectorFst<Arc> T;
    CHECK_OK(this->MakeRandFst(&T));
    VLOG(1) << "Check topologically sorted Fst is equivalent to its input.";
    VectorFst<Arc> S3(T);
    TopSort(&S3);
    EXPECT_TRUE(this->Equiv(T, S3));
  }
}

TYPED_TEST_P(SortTest, ReverseReverseIdentity) {
  using Arc = TypeParam;
  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    VectorFst<Arc> T;
    CHECK_OK(this->MakeRandFst(&T));
    VLOG(1) << "Check reverse(reverse(T)) = T";
    for (int j = 0; j < 2; ++j) {
      VectorFst<ReverseArc<Arc>> R1;
      VectorFst<Arc> R2;
      bool require_superinitial = (j == 1);
      Reverse(T, &R1, require_superinitial);
      Reverse(R1, &R2, require_superinitial);
      EXPECT_TRUE(this->Equiv(T, R2));
    }
  }
}

template <class Arc>
class OptimizeTest : public AlgoTestBase<Arc> {};
TYPED_TEST_SUITE_P(OptimizeTest);

TYPED_TEST_P(OptimizeTest, ConnectEquivalent) {
  using Arc = TypeParam;
  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    VectorFst<Arc> T;
    CHECK_OK(this->MakeRandFst(&T));
    VLOG(1) << "Check connected FST is equivalent to its input.";
    VectorFst<Arc> C1(T);
    Connect(&C1);
    EXPECT_TRUE(this->Equiv(T, C1));
  }
}

TYPED_TEST_P(OptimizeTest, RmEpsilonDestructive) {
  using Arc = TypeParam;
  using Weight = typename Arc::Weight;
  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    VectorFst<Arc> T;
    CHECK_OK(this->MakeRandFst(&T));
    uint64_t tprops = T.Properties(kFstProperties, true);
    uint64_t wprops = Weight::Properties();
    if ((wprops & kSemiring) == kSemiring &&
        (tprops & kAcyclic || wprops & kIdempotent)) {
      VectorFst<Arc> R1(T);
      RmEpsilon(&R1);
      VLOG(1) << "Check epsilon-removed FST is equivalent to its input.";
      EXPECT_TRUE(this->Equiv(T, R1));
    }
  }
}

TYPED_TEST_P(OptimizeTest, RmEpsilonDelayed) {
  using Arc = TypeParam;
  using Weight = typename Arc::Weight;
  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    VectorFst<Arc> T;
    CHECK_OK(this->MakeRandFst(&T));
    uint64_t tprops = T.Properties(kFstProperties, true);
    uint64_t wprops = Weight::Properties();
    if ((wprops & kSemiring) == kSemiring &&
        (tprops & kAcyclic || wprops & kIdempotent)) {
      VectorFst<Arc> R1(T);
      RmEpsilon(&R1);
      RmEpsilonFst<Arc> R2(T);
      VLOG(1)
          << "Check destructive and delayed epsilon removal are equivalent.";
      EXPECT_TRUE(this->Equiv(R1, R2));
    }
  }
}

TYPED_TEST_P(OptimizeTest, RmEpsilonLargeProportion) {
  using Arc = TypeParam;
  using Weight = typename Arc::Weight;
  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    VectorFst<Arc> T;
    CHECK_OK(this->MakeRandFst(&T));
    uint64_t tprops = T.Properties(kFstProperties, true);
    uint64_t wprops = Weight::Properties();
    if ((wprops & kSemiring) == kSemiring &&
        (tprops & kAcyclic || wprops & kIdempotent)) {
      VLOG(1) << "Check an FST with a large proportion of epsilon transitions:";
      // Maps all transitions of T to epsilon-transitions and append
      // a non-epsilon transition.
      VectorFst<Arc> U;
      ArcMap(T, &U, EpsMapper<Arc>());
      VectorFst<Arc> V;
      V.SetStart(V.AddState());
      Arc arc(1, 1, Weight::One(), V.AddState());
      V.AddArc(V.Start(), arc);
      V.SetFinal(arc.nextstate, Weight::One());
      Concat(&U, V);
      // Check that epsilon-removal preserves the shortest-distance
      // from the initial state to the final states.
      std::vector<Weight> d;
      ShortestDistance(U, &d, true);
      // U is empty, U.Start() is -1, so we don't need to check vs kNoStateId.
      Weight w = U.Start() < d.size() ? d[U.Start()] : Weight::Zero();
      VectorFst<Arc> U1(U);
      RmEpsilon(&U1);
      ShortestDistance(U1, &d, true);
      Weight w1 = U1.Start() < d.size() ? d[U1.Start()] : Weight::Zero();
      EXPECT_TRUE(ApproxEqual(w, w1, this->kTestDelta));
      RmEpsilonFst<Arc> U2(U);
      ShortestDistance(U2, &d, true);
      Weight w2 = U2.Start() < d.size() ? d[U2.Start()] : Weight::Zero();
      EXPECT_TRUE(ApproxEqual(w, w2, this->kTestDelta));
    }
  }
}

TYPED_TEST_P(OptimizeTest, DeterminizeFSAEquivalent) {
  using Arc = TypeParam;
  using Weight = typename Arc::Weight;
  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    VectorFst<Arc> T;
    CHECK_OK(this->MakeRandFst(&T));
    uint64_t tprops = T.Properties(kFstProperties, true);
    uint64_t wprops = Weight::Properties();

    VectorFst<Arc> A(T);
    Project(&A, ProjectType::INPUT);
    if ((wprops & kSemiring) == kSemiring && tprops & kAcyclic) {
      DeterminizeFst<Arc> D(A);
      VLOG(1) << "Check determinized FSA is equivalent to its input.";
      EXPECT_TRUE(this->Equiv(A, D));
    }
  }
}

TYPED_TEST_P(OptimizeTest, DeterminizeFSTEquivalent) {
  using Arc = TypeParam;
  using Weight = typename Arc::Weight;
  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    VectorFst<Arc> T;
    CHECK_OK(this->MakeRandFst(&T));
    uint64_t tprops = T.Properties(kFstProperties, true);
    uint64_t wprops = Weight::Properties();
    if ((wprops & kSemiring) == kSemiring && tprops & kAcyclic) {
      VLOG(1) << "Check determinized FST is equivalent to its input.";
      DeterminizeFstOptions<Arc> opts;
      opts.type = DETERMINIZE_NONFUNCTIONAL;
      DeterminizeFst<Arc> DT(T, opts);
      EXPECT_TRUE(this->Equiv(T, DT));
    }
  }
}

TYPED_TEST_P(OptimizeTest, PruningDeterminization) {
  using Arc = TypeParam;
  using Weight = typename Arc::Weight;
  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    VectorFst<Arc> T;
    CHECK_OK(this->MakeRandFst(&T));
    uint64_t tprops = T.Properties(kFstProperties, true);
    uint64_t wprops = Weight::Properties();

    VectorFst<Arc> A(T);
    Project(&A, ProjectType::INPUT);
    if ((wprops & kSemiring) == kSemiring && tprops & kAcyclic) {
      if ((wprops & (kPath | kCommutative)) == (kPath | kCommutative)) {
        VLOG(1) << "Check pruning in determinization";
        VectorFst<Arc> P;
        const Weight threshold = this->generate_();
        DeterminizeOptions<Arc> opts;
        opts.weight_threshold = threshold;
        Determinize(A, &P, opts);
        EXPECT_TRUE(P.Properties(kIDeterministic, true));
        EXPECT_TRUE(this->PruneEquiv(A, P, threshold));
      }
    }
  }
}

TYPED_TEST_P(OptimizeTest, DisambiguateMinDeterminization) {
  using Arc = TypeParam;
  using Weight = typename Arc::Weight;
  using Label = typename Arc::Label;
  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    VectorFst<Arc> T;
    CHECK_OK(this->MakeRandFst(&T));
    uint64_t tprops = T.Properties(kFstProperties, true);
    uint64_t wprops = Weight::Properties();
    if ((wprops & kSemiring) == kSemiring && tprops & kAcyclic) {
      if ((wprops & kPath) == kPath) {
        VLOG(1) << "Check min-determinization";
        // Ensures no input epsilons
        VectorFst<Arc> R(T);
        std::vector<std::pair<Label, Label>> ipairs;
        ipairs.push_back(std::make_pair(0, 1));
        std::vector<std::pair<Label, Label>> opairs;
        Relabel(&R, ipairs, opairs);

        VectorFst<Arc> M;
        DeterminizeOptions<Arc> opts;
        opts.type = DETERMINIZE_DISAMBIGUATE;
        Determinize(R, &M, opts);
        EXPECT_TRUE(M.Properties(kIDeterministic, true));
        EXPECT_TRUE(this->MinRelated(M, R));
      }
    }
  }
}

TYPED_TEST_P(OptimizeTest, DeterminizeMinimize) {
  using Arc = TypeParam;
  using Weight = typename Arc::Weight;
  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    VectorFst<Arc> T;
    CHECK_OK(this->MakeRandFst(&T));
    uint64_t tprops = T.Properties(kFstProperties, true);
    uint64_t wprops = Weight::Properties();

    VectorFst<Arc> A(T);
    Project(&A, ProjectType::INPUT);
    if ((wprops & kSemiring) == kSemiring && tprops & kAcyclic) {
      DeterminizeFst<Arc> D(A);
      VLOG(1) << "Check size(min(det(A))) <= size(det(A)) and min(det(A)) "
                 "equiv det(A)";
      VectorFst<Arc> M(D);
      int n = M.NumStates();
      Minimize(&M, static_cast<MutableFst<Arc>*>(nullptr), this->kDelta);
      EXPECT_TRUE(this->Equiv(D, M));
      EXPECT_LE(M.NumStates(), n);
    }
  }
}

TYPED_TEST_P(OptimizeTest, DeterminizeRevuzBrozozowski) {
  using Arc = TypeParam;
  using Weight = typename Arc::Weight;
  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    VectorFst<Arc> T;
    CHECK_OK(this->MakeRandFst(&T));
    uint64_t tprops = T.Properties(kFstProperties, true);
    uint64_t wprops = Weight::Properties();

    VectorFst<Arc> A(T);
    Project(&A, ProjectType::INPUT);
    if ((wprops & kSemiring) == kSemiring && tprops & kAcyclic) {
      DeterminizeFst<Arc> D(A);
      VectorFst<Arc> M(D);
      Minimize(&M, static_cast<MutableFst<Arc>*>(nullptr), this->kDelta);
      int n = M.NumStates();
      if (n && (wprops & kIdempotent) == kIdempotent &&
          A.Properties(kNoEpsilons, true)) {
        VLOG(1) << "Check that Revuz's algorithm leads to the same number of "
                   "states as Brozozowski's algorithm";

        // Skip test if A is the empty machine or contains epsilons or
        // if the semiring is not idempotent (to avoid floating point
        // errors)
        VectorFst<ReverseArc<Arc>> R;
        Reverse(A, &R);
        RmEpsilon(&R);
        DeterminizeFst<ReverseArc<Arc>> DR(R);
        VectorFst<Arc> RD;
        Reverse(DR, &RD);
        DeterminizeFst<Arc> DRD(RD);
        {
          VectorFst<Arc> M(DRD);
          EXPECT_EQ(n + 1, M.NumStates());  // Accounts for the epsilon
                                            // transition to the initial state
        }
      }
    }
  }
}

TYPED_TEST_P(OptimizeTest, DisambiguateFSAEquivalent) {
  using Arc = TypeParam;
  using Weight = typename Arc::Weight;
  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    VectorFst<Arc> T;
    CHECK_OK(this->MakeRandFst(&T));
    uint64_t tprops = T.Properties(kFstProperties, true);
    uint64_t wprops = Weight::Properties();

    VectorFst<Arc> A(T);
    Project(&A, ProjectType::INPUT);
    if ((wprops & kSemiring) == kSemiring && tprops & kAcyclic) {
      VectorFst<Arc> R(A), D;
      VLOG(1) << "Check disambiguated FSA is equivalent to its input.";
      RmEpsilon(&R);

      Disambiguate(R, &D);
      EXPECT_TRUE(this->Equiv(R, D));
    }
  }
}

TYPED_TEST_P(OptimizeTest, DisambiguateUnambiguous) {
  using Arc = TypeParam;
  using Weight = typename Arc::Weight;
  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    VectorFst<Arc> T;
    CHECK_OK(this->MakeRandFst(&T));
    uint64_t tprops = T.Properties(kFstProperties, true);
    uint64_t wprops = Weight::Properties();

    VectorFst<Arc> A(T);
    Project(&A, ProjectType::INPUT);
    if ((wprops & kSemiring) == kSemiring && tprops & kAcyclic) {
      VectorFst<Arc> R(A), D;
      RmEpsilon(&R);

      Disambiguate(R, &D);
      EXPECT_TRUE(this->Equiv(R, D));
      VLOG(1) << "Check disambiguated FSA is unambiguous";
      EXPECT_TRUE(this->Unambiguous(D));

#if 0
      // TODO: find out why this fails
      if ((wprops & (kPath | kCommutative)) == (kPath | kCommutative)) {
        VLOG(1) << "Check pruning in disambiguation";
        VectorFst<Arc> P;
        const Weight threshold = this->generate_();
        DisambiguateOptions<Arc> opts;
        opts.weight_threshold = threshold;
        Disambiguate(R, &P, opts);
        EXPECT_TRUE(this->Unambiguous(P));
        EXPECT_TRUE(this->PruneEquiv(A, P, threshold));
      }
#endif
    }
  }
}

TYPED_TEST_P(OptimizeTest, ReweightEquivalent) {
  using Arc = TypeParam;
  using Weight = typename Arc::Weight;
  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    VectorFst<Arc> T;
    CHECK_OK(this->MakeRandFst(&T));
    if (Arc::Type() == LogArc::Type() || Arc::Type() == StdArc::Type()) {
      std::vector<Weight> potential;
      VectorFst<Arc> RI(T);
      VectorFst<Arc> RF(T);
      while (potential.size() < RI.NumStates()) {
        potential.push_back(this->generate_());
      }
      VLOG(1) << "Check reweight(T) equiv T";
      Reweight(&RI, potential, REWEIGHT_TO_INITIAL);
      EXPECT_TRUE(this->Equiv(T, RI));

      Reweight(&RF, potential, REWEIGHT_TO_FINAL);
      EXPECT_TRUE(this->Equiv(T, RF));
    }
  }
}

TYPED_TEST_P(OptimizeTest, PushRightSemiring) {
  using Arc = TypeParam;
  using Weight = typename Arc::Weight;
  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    VectorFst<Arc> T;
    CHECK_OK(this->MakeRandFst(&T));
    uint64_t tprops = T.Properties(kFstProperties, true);
    uint64_t wprops = Weight::Properties();
    if ((wprops & kIdempotent) || (tprops & kAcyclic)) {
      if (wprops & kRightSemiring) {
        VLOG(1) << "Check pushed FST is equivalent to input FST.";
        // Pushing towards the final state.
        VectorFst<Arc> P1;
        Push<Arc, REWEIGHT_TO_FINAL>(T, &P1, kPushLabels);
        EXPECT_TRUE(this->Equiv(T, P1));

        VectorFst<Arc> P2;
        Push<Arc, REWEIGHT_TO_FINAL>(T, &P2, kPushWeights);
        EXPECT_TRUE(this->Equiv(T, P2));

        VectorFst<Arc> P3;
        Push<Arc, REWEIGHT_TO_FINAL>(T, &P3, kPushLabels | kPushWeights);
        EXPECT_TRUE(this->Equiv(T, P3));
      }
    }
  }
}

TYPED_TEST_P(OptimizeTest, PushLeftSemiring) {
  using Arc = TypeParam;
  using Weight = typename Arc::Weight;
  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    VectorFst<Arc> T;
    CHECK_OK(this->MakeRandFst(&T));
    uint64_t tprops = T.Properties(kFstProperties, true);
    uint64_t wprops = Weight::Properties();
    if ((wprops & kIdempotent) || (tprops & kAcyclic)) {
      if (wprops & kLeftSemiring) {
        VLOG(1) << "Check pushed FST is equivalent to input FST.";
        // Pushing towards the initial state.
        VectorFst<Arc> P1;
        Push<Arc, REWEIGHT_TO_INITIAL>(T, &P1, kPushLabels);
        EXPECT_TRUE(this->Equiv(T, P1));

        VectorFst<Arc> P2;
        Push<Arc, REWEIGHT_TO_INITIAL>(T, &P2, kPushWeights);
        EXPECT_TRUE(this->Equiv(T, P2));

        VectorFst<Arc> P3;
        Push<Arc, REWEIGHT_TO_INITIAL>(T, &P3, kPushLabels | kPushWeights);
        EXPECT_TRUE(this->Equiv(T, P3));
      }
    }
  }
}

TYPED_TEST_P(OptimizeTest, PruningAlgorithm) {
  using Arc = TypeParam;
  using Weight = typename Arc::Weight;
  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    VectorFst<Arc> T;
    CHECK_OK(this->MakeRandFst(&T));
    uint64_t wprops = Weight::Properties();
    if constexpr (IsPath<Weight>::value) {
      if ((wprops & (kPath | kCommutative)) == (kPath | kCommutative)) {
        VLOG(1) << "Check pruning algorithm";
        VLOG(1) << "Check equiv. of constructive and destructive algorithms";
        const Weight threshold = this->generate_();
        VectorFst<Arc> P1(T);
        Prune(&P1, threshold);
        VectorFst<Arc> P2;
        Prune(T, &P2, threshold);
        EXPECT_TRUE(this->Equiv(P1, P2));
      }
    }
  }
}

TYPED_TEST_P(OptimizeTest, PruningReverseEquiv) {
  using Arc = TypeParam;
  using Weight = typename Arc::Weight;
  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    VectorFst<Arc> T;
    CHECK_OK(this->MakeRandFst(&T));
    uint64_t wprops = Weight::Properties();
    if constexpr (IsPath<Weight>::value) {
      if ((wprops & (kPath | kCommutative)) == (kPath | kCommutative)) {
        const Weight threshold = this->generate_();
        VectorFst<Arc> P1(T);
        Prune(&P1, threshold);
        VLOG(1) << "Check prune(reverse) equiv reverse(prune)";
        VectorFst<ReverseArc<Arc>> R;
        VectorFst<Arc> P2;
        Reverse(T, &R);
        Prune(&R, threshold.Reverse());
        Reverse(R, &P2);
        EXPECT_TRUE(this->Equiv(P1, P2));
      }
    }
  }
}

TYPED_TEST_P(OptimizeTest, PruningShortestDistance) {
  using Arc = TypeParam;
  using Weight = typename Arc::Weight;
  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    VectorFst<Arc> T;
    CHECK_OK(this->MakeRandFst(&T));
    uint64_t wprops = Weight::Properties();

    VectorFst<Arc> A(T);
    Project(&A, ProjectType::INPUT);
    if constexpr (IsPath<Weight>::value) {
      if ((wprops & (kPath | kCommutative)) == (kPath | kCommutative)) {
        const Weight threshold = this->generate_();
        VectorFst<Arc> P;
        Prune(A, &P, threshold);
        VLOG(1) << "Check: ShortestDistance(A - prune(A)) > "
                   "ShortestDistance(A) times Threshold";
        EXPECT_TRUE(this->PruneEquiv(A, P, threshold));
      }
    }
  }
}

TYPED_TEST_P(OptimizeTest, SynchronizeEquivalent) {
  using Arc = TypeParam;
  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    VectorFst<Arc> T;
    CHECK_OK(this->MakeRandFst(&T));
    uint64_t tprops = T.Properties(kFstProperties, true);
    if (tprops & kAcyclic) {
      VLOG(1) << "Check synchronize(T) equiv T";
      SynchronizeFst<Arc> S(T);
      EXPECT_TRUE(this->Equiv(T, S));
    }
  }
}

template <class Arc>
class SearchTest : public AlgoTestBase<Arc> {};
TYPED_TEST_SUITE_P(SearchTest);

TYPED_TEST_P(SearchTest, ShortestPathEquivalent) {
  using Arc = TypeParam;
  using Weight = typename Arc::Weight;
  if constexpr (IsPath<Weight>::value) {
    for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
      VectorFst<Arc> T;
      CHECK_OK(this->MakeRandFst(&T));
      uint64_t wprops = Weight::Properties();

      if ((wprops & (kPath | kRightSemiring)) == (kPath | kRightSemiring)) {
        VLOG(1) << "Check 1-best weight.";
        VectorFst<Arc> path;
        ShortestPath(T, &path);
        Weight tsum = ShortestDistance(T);
        Weight psum = ShortestDistance(path);
        EXPECT_TRUE(ApproxEqual(tsum, psum, this->kTestDelta));
      }
    }
  }
}

TYPED_TEST_P(SearchTest, NShortestPathsEquivalent) {
  using Arc = TypeParam;
  using Weight = typename Arc::Weight;
  if constexpr (IsPath<Weight>::value) {
    for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
      VectorFst<Arc> T;
      CHECK_OK(this->MakeRandFst(&T));
      uint64_t wprops = Weight::Properties();

      VectorFst<Arc> A(T);
      Project(&A, ProjectType::INPUT);

      if ((wprops & (kPath | kSemiring)) == (kPath | kSemiring)) {
        VLOG(1) << "Check n-best weights";
        VectorFst<Arc> R(A);
        RmEpsilon(&R, /*connect=*/true, Arc::Weight::Zero(), kNoStateId,
                  this->kDelta);
        const int nshortest = std::uniform_int_distribution<>(
            0, this->kNumRandomShortestPaths + 1)(this->rand_);
        VectorFst<Arc> paths;
        ShortestPath(R, &paths, nshortest, /*unique=*/true,
                     /*first_path=*/false, Weight::Zero(),
                     this->kNumShortestStates, this->kDelta);
        std::vector<Weight> distance;
        ShortestDistance(paths, &distance, true, this->kDelta);
        typename Arc::StateId pstart = paths.Start();
        if (pstart != kNoStateId) {
          ArcIterator<Fst<Arc>> piter(paths, pstart);
          for (; !piter.Done(); piter.Next()) {
            typename Arc::StateId s = piter.Value().nextstate;
            Weight nsum = (s != kNoStateId && s < distance.size())
                              ? Times(piter.Value().weight, distance[s])
                              : Weight::Zero();
            VectorFst<Arc> path;
            ShortestPath(R, &path, 1, false, false, Weight::Zero(), kNoStateId,
                         this->kDelta);
            Weight dsum = ShortestDistance(path, this->kDelta);
            EXPECT_TRUE(ApproxEqual(nsum, dsum, this->kTestDelta));
            ArcMap(&path, RmWeightMapper<Arc>());
            VectorFst<Arc> S;
            Difference(R, path, &S);
            R = S;
          }
        }
      }
    }
  }
}

// This should work for any commutative,
// idempotent semiring when restricted to the unweighted case
// (being isomorphic to the boolean semiring).
template <class Arc>
class UnweightedTest : public AlgoTestBase<Arc> {};
TYPED_TEST_SUITE_P(UnweightedTest);

TYPED_TEST_P(UnweightedTest, UnionDestructive) {
  using Arc = TypeParam;
  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    VectorFst<Arc> T1, T2;
    CHECK_OK(this->MakeRandFst(&T1));
    CHECK_OK(this->MakeRandFst(&T2));
    VectorFst<Arc> A1(T1), A2(T2);
    Project(&A1, ProjectType::OUTPUT);
    Project(&A2, ProjectType::INPUT);
    ArcMap(&A1, RmWeightMapper<Arc>());
    ArcMap(&A2, RmWeightMapper<Arc>());
    VLOG(1) << "Check the union contains its arguments (destructive).";
    VectorFst<Arc> U(A1);
    Union(&U, A2);

    EXPECT_TRUE(this->Subset(A1, U));
    EXPECT_TRUE(this->Subset(A2, U));
  }
}

TYPED_TEST_P(UnweightedTest, UnionDelayed) {
  using Arc = TypeParam;
  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    VectorFst<Arc> T1, T2;
    CHECK_OK(this->MakeRandFst(&T1));
    CHECK_OK(this->MakeRandFst(&T2));
    VectorFst<Arc> A1(T1), A2(T2);
    Project(&A1, ProjectType::OUTPUT);
    Project(&A2, ProjectType::INPUT);
    ArcMap(&A1, RmWeightMapper<Arc>());
    ArcMap(&A2, RmWeightMapper<Arc>());
    VLOG(1) << "Check the union contains its arguments (delayed).";
    UnionFst<Arc> U(A1, A2);

    EXPECT_TRUE(this->Subset(A1, U));
    EXPECT_TRUE(this->Subset(A2, U));
  }
}

TYPED_TEST_P(UnweightedTest, ClosureDestructive) {
  using Arc = TypeParam;
  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    VectorFst<Arc> T1;
    CHECK_OK(this->MakeRandFst(&T1));
    VectorFst<Arc> A1(T1);
    Project(&A1, ProjectType::OUTPUT);
    ArcMap(&A1, RmWeightMapper<Arc>());
    const int n = std::uniform_int_distribution<>(0, 4)(this->rand_);
    VectorFst<Arc> C(this->one_fst_);
    VLOG(1) << "Check if A^n c A* (destructive).";
    for (int j = 0; j < n; ++j) Concat(&C, A1);
    VectorFst<Arc> S(A1);
    Closure(&S, CLOSURE_STAR);
    EXPECT_TRUE(this->Subset(C, S));
  }
}

TYPED_TEST_P(UnweightedTest, ClosureDelayed) {
  using Arc = TypeParam;
  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    VectorFst<Arc> T1;
    CHECK_OK(this->MakeRandFst(&T1));
    VectorFst<Arc> A1(T1);
    Project(&A1, ProjectType::OUTPUT);
    ArcMap(&A1, RmWeightMapper<Arc>());
    const int n = std::uniform_int_distribution<>(0, 4)(this->rand_);
    VLOG(1) << "Check if A^n c A* (delayed).";
    std::unique_ptr<Fst<Arc>> C =
        std::make_unique<VectorFst<Arc>>(this->one_fst_);
    for (int j = 0; j < n; ++j) {
      C = std::make_unique<ConcatFst<Arc>>(*C, A1);
    }
    ClosureFst<Arc> S(A1, CLOSURE_STAR);
    EXPECT_TRUE(this->Subset(*C, S));
  }
}

TYPED_TEST_P(UnweightedTest, IntersectContained) {
  using Arc = TypeParam;
  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    VectorFst<Arc> T1, T2;
    CHECK_OK(this->MakeRandFst(&T1));
    CHECK_OK(this->MakeRandFst(&T2));
    VectorFst<Arc> A1(T1), A2(T2);
    Project(&A1, ProjectType::OUTPUT);
    Project(&A2, ProjectType::INPUT);
    ArcMap(&A1, RmWeightMapper<Arc>());
    ArcMap(&A2, RmWeightMapper<Arc>());
    ILabelCompare<Arc> comp;
    ArcSort(&A1, comp);
    ArcSort(&A2, comp);
    VLOG(1) << "Check the intersection is contained in its arguments.";
    IntersectFst<Arc> I1(A1, A2);
    EXPECT_TRUE(this->Subset(I1, A1));
    EXPECT_TRUE(this->Subset(I1, A2));
  }
}

TYPED_TEST_P(UnweightedTest, UnionDistributesIntersect) {
  using Arc = TypeParam;
  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    VectorFst<Arc> T1, T2, T3;
    CHECK_OK(this->MakeRandFst(&T1));
    CHECK_OK(this->MakeRandFst(&T2));
    CHECK_OK(this->MakeRandFst(&T3));
    VectorFst<Arc> A1(T1), A2(T2), A3(T3);
    Project(&A1, ProjectType::OUTPUT);
    Project(&A2, ProjectType::INPUT);
    Project(&A3, ProjectType::INPUT);
    ArcMap(&A1, RmWeightMapper<Arc>());
    ArcMap(&A2, RmWeightMapper<Arc>());
    ArcMap(&A3, RmWeightMapper<Arc>());
    ILabelCompare<Arc> comp;
    ArcSort(&A1, comp);
    ArcSort(&A2, comp);
    ArcSort(&A3, comp);
    VLOG(1) << "Check union distributes over intersection.";
    IntersectFst<Arc> I1(A1, A2);
    UnionFst<Arc> U1(I1, A3);
    UnionFst<Arc> U2(A1, A3);
    UnionFst<Arc> U3(A2, A3);
    ArcSortFst<Arc, ILabelCompare<Arc>> S4(U3, comp);
    IntersectFst<Arc> I2(U2, S4);
    EXPECT_TRUE(this->FsaEquiv(U1, I2));
  }
}

TYPED_TEST_P(UnweightedTest, ComplementSigmaStar) {
  using Arc = TypeParam;
  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    VectorFst<Arc> T1, T2;
    CHECK_OK(this->MakeRandFst(&T1));
    CHECK_OK(this->MakeRandFst(&T2));
    VectorFst<Arc> A1(T1), A2(T2);
    Project(&A1, ProjectType::OUTPUT);
    Project(&A2, ProjectType::INPUT);
    ArcMap(&A1, RmWeightMapper<Arc>());
    ArcMap(&A2, RmWeightMapper<Arc>());
    ILabelCompare<Arc> comp;
    ArcSort(&A1, comp);
    ArcSort(&A2, comp);
    VectorFst<Arc> C1, C2;
    this->Complement(A1, &C1);
    this->Complement(A2, &C2);
    ArcSort(&C1, comp);
    ArcSort(&C2, comp);
    VLOG(1) << "Check S U S' = Sigma*";
    UnionFst<Arc> U(A1, C1);
    EXPECT_TRUE(this->FsaEquiv(U, this->univ_fst_));
  }
}

TYPED_TEST_P(UnweightedTest, ComplementEmpty) {
  using Arc = TypeParam;
  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    VectorFst<Arc> T1, T2;
    CHECK_OK(this->MakeRandFst(&T1));
    CHECK_OK(this->MakeRandFst(&T2));
    VectorFst<Arc> A1(T1), A2(T2);
    Project(&A1, ProjectType::OUTPUT);
    Project(&A2, ProjectType::INPUT);
    ArcMap(&A1, RmWeightMapper<Arc>());
    ArcMap(&A2, RmWeightMapper<Arc>());
    ILabelCompare<Arc> comp;
    ArcSort(&A1, comp);
    ArcSort(&A2, comp);
    VectorFst<Arc> C1, C2;
    this->Complement(A1, &C1);
    this->Complement(A2, &C2);
    ArcSort(&C1, comp);
    ArcSort(&C2, comp);
    VLOG(1) << "Check S n S' = {}";
    IntersectFst<Arc> I(A1, C1);
    EXPECT_TRUE(this->FsaEquiv(I, this->zero_fst_));
  }
}

TYPED_TEST_P(UnweightedTest, DeMorganUnion) {
  using Arc = TypeParam;
  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    VectorFst<Arc> T1, T2;
    CHECK_OK(this->MakeRandFst(&T1));
    CHECK_OK(this->MakeRandFst(&T2));
    VectorFst<Arc> A1(T1), A2(T2);
    Project(&A1, ProjectType::OUTPUT);
    Project(&A2, ProjectType::INPUT);
    ArcMap(&A1, RmWeightMapper<Arc>());
    ArcMap(&A2, RmWeightMapper<Arc>());
    ILabelCompare<Arc> comp;
    ArcSort(&A1, comp);
    ArcSort(&A2, comp);
    VectorFst<Arc> C1, C2;
    this->Complement(A1, &C1);
    this->Complement(A2, &C2);
    ArcSort(&C1, comp);
    ArcSort(&C2, comp);
    VLOG(1) << "Check (S1' U S2') == (S1 n S2)'";
    UnionFst<Arc> U(C1, C2);
    IntersectFst<Arc> I(A1, A2);
    VectorFst<Arc> C3;
    this->Complement(I, &C3);
    EXPECT_TRUE(this->FsaEquiv(U, C3));
  }
}

TYPED_TEST_P(UnweightedTest, DeMorganIntersect) {
  using Arc = TypeParam;
  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    VectorFst<Arc> T1, T2;
    CHECK_OK(this->MakeRandFst(&T1));
    CHECK_OK(this->MakeRandFst(&T2));
    VectorFst<Arc> A1(T1), A2(T2);
    Project(&A1, ProjectType::OUTPUT);
    Project(&A2, ProjectType::INPUT);
    ArcMap(&A1, RmWeightMapper<Arc>());
    ArcMap(&A2, RmWeightMapper<Arc>());
    ILabelCompare<Arc> comp;
    ArcSort(&A1, comp);
    ArcSort(&A2, comp);
    VectorFst<Arc> C1, C2;
    this->Complement(A1, &C1);
    this->Complement(A2, &C2);
    ArcSort(&C1, comp);
    ArcSort(&C2, comp);
    VLOG(1) << "Check (S1' n S2') == (S1 U S2)'";
    IntersectFst<Arc> I(C1, C2);
    UnionFst<Arc> U(A1, A2);
    VectorFst<Arc> C3;
    this->Complement(U, &C3);
    EXPECT_TRUE(this->FsaEquiv(I, C3));
  }
}

TYPED_TEST_P(UnweightedTest, DeterminizeEquivalent) {
  using Arc = TypeParam;
  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    VectorFst<Arc> T;
    CHECK_OK(this->MakeRandFst(&T));
    VectorFst<Arc> A(T);
    Project(&A, ProjectType::OUTPUT);
    ArcMap(&A, RmWeightMapper<Arc>());
    VLOG(1) << "Check determinized FSA is equivalent to its input.";
    DeterminizeFst<Arc> D(A);
    EXPECT_TRUE(this->FsaEquiv(A, D));
  }
}

TYPED_TEST_P(UnweightedTest, DisambiguateEquivalent) {
  using Arc = TypeParam;
  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    VectorFst<Arc> T;
    CHECK_OK(this->MakeRandFst(&T));
    VectorFst<Arc> A(T);
    Project(&A, ProjectType::OUTPUT);
    ArcMap(&A, RmWeightMapper<Arc>());
    VLOG(1) << "Check disambiguated FSA is equivalent to its input.";
    VectorFst<Arc> R(A), D;
    RmEpsilon(&R);

    Disambiguate(R, &D);
    EXPECT_TRUE(this->FsaEquiv(R, D));
  }
}

TYPED_TEST_P(UnweightedTest, MinimizeEquivalent) {
  using Arc = TypeParam;
  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    VectorFst<Arc> T;
    CHECK_OK(this->MakeRandFst(&T));
    VectorFst<Arc> A(T);
    Project(&A, ProjectType::OUTPUT);
    ArcMap(&A, RmWeightMapper<Arc>());
    VLOG(1) << "Check minimized FSA is equivalent to its input.";
    RmEpsilonFst<Arc> R(A);
    DeterminizeFst<Arc> D(R);
    VectorFst<Arc> M(D);
    Minimize(&M, static_cast<MutableFst<Arc>*>(nullptr), this->kDelta);
    EXPECT_TRUE(this->FsaEquiv(A, M));
  }
}

TYPED_TEST_P(UnweightedTest, HopcroftRevuzAlgorithm) {
  using Arc = TypeParam;
  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    VectorFst<Arc> T;
    CHECK_OK(this->MakeRandFst(&T));
    VectorFst<Arc> A(T);
    Project(&A, ProjectType::OUTPUT);
    ArcMap(&A, RmWeightMapper<Arc>());
    int n;
    {
      RmEpsilonFst<Arc> R(A);
      DeterminizeFst<Arc> D(R);
      VectorFst<Arc> M(D);
      Minimize(&M, static_cast<MutableFst<Arc>*>(nullptr), this->kDelta);
      n = M.NumStates();
    }
    if (n) {
      VLOG(1) << "Check that Hopcroft's and Revuz's algorithms lead to the "
                 "same number of states as Brozozowski's algorithm";
      VectorFst<Arc> R;
      Reverse(A, &R);
      RmEpsilon(&R);
      DeterminizeFst<Arc> DR(R);
      VectorFst<Arc> RD;
      Reverse(DR, &RD);
      DeterminizeFst<Arc> DRD(RD);
      VectorFst<Arc> M(DRD);
      EXPECT_EQ(n + 1, M.NumStates());  // Accounts for the epsilon transition
                                        // to the initial state.
    }
  }
}

REGISTER_TYPED_TEST_SUITE_P(
    RationalTest, UnionEquivalent, ConcatEquivalent, ClosureStarEquivalent,
    ClosurePlusEquivalent, UnionAssociativeDestructive, UnionAssociativeDelayed,
    UnionAssociativeDestructiveDelayed, ConcatAssociativeDestructive,
    ConcatAssociativeDelayed, ConcatAssociativeDestructiveDelayed,
    ConcatLeftDistributesUnionDestructive,
    ConcatRightDistributesUnionDestructive, ConcatLeftDistributesUnionDelayed,
    ConcatRightDistributesUnionDelayed, TTStarDestructive, TStarTDestructive,
    TTStarDelayed, TStarTDelayed);
REGISTER_TYPED_TEST_SUITE_P(MapTest, ProjectEquivalent, InvertEquivalent,
                            IdentityDestructivePi1, IdentityDestructivePi2,
                            IdentityDelayedPi1, IdentityDelayedPi2,
                            RelabelDestructive, RelabelDelayed,
                            EncodeDestructive, EncodeDelayed,
                            GallicConstructive, GallicDelayed);
REGISTER_TYPED_TEST_SUITE_P(ComposeTest, Associative, LeftDistributesUnion,
                            RightDistributesUnion, IntersectionCommutative,
                            FiltersEpsilon, FiltersNull, LookAhead);
REGISTER_TYPED_TEST_SUITE_P(SortTest, ArcSortedEquivalent,
                            DestructiveDelayedEquivalent, InversionsIdentity,
                            TopologicallySortedEquivalent,
                            ReverseReverseIdentity);
REGISTER_TYPED_TEST_SUITE_P(OptimizeTest, ConnectEquivalent,
                            RmEpsilonDestructive, RmEpsilonDelayed,
                            RmEpsilonLargeProportion, DeterminizeFSAEquivalent,
                            DeterminizeFSTEquivalent, PruningDeterminization,
                            DisambiguateMinDeterminization, DeterminizeMinimize,
                            DeterminizeRevuzBrozozowski,
                            DisambiguateFSAEquivalent, DisambiguateUnambiguous,
                            ReweightEquivalent, PushRightSemiring,
                            PushLeftSemiring, PruningAlgorithm,
                            PruningReverseEquiv, PruningShortestDistance,
                            SynchronizeEquivalent);
REGISTER_TYPED_TEST_SUITE_P(UnweightedTest, UnionDestructive, UnionDelayed,
                            ClosureDestructive, ClosureDelayed,
                            IntersectContained, UnionDistributesIntersect,
                            ComplementSigmaStar, ComplementEmpty, DeMorganUnion,
                            DeMorganIntersect, DeterminizeEquivalent,
                            DisambiguateEquivalent, MinimizeEquivalent,
                            HopcroftRevuzAlgorithm);
REGISTER_TYPED_TEST_SUITE_P(SearchTest, ShortestPathEquivalent,
                            NShortestPathsEquivalent);

}  // namespace fst

#endif  // OPENFST_TEST_ALGO_TEST_H_
