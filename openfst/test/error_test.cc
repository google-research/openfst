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
// Unit test for non-fatal error handling.
// Applies when FLAGS_fst_error_fatal = false.

#include <memory>
#include <vector>

#include "gtest/gtest.h"
#include "absl/flags/flag.h"
#include "openfst/lib/arc-map.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/arcsort.h"
#include "openfst/lib/cache.h"
#include "openfst/lib/closure.h"
#include "openfst/lib/compact-fst.h"
#include "openfst/lib/compose.h"
#include "openfst/lib/concat.h"
#include "openfst/lib/const-fst.h"
#include "openfst/lib/determinize.h"
#include "openfst/lib/difference.h"
#include "openfst/lib/epsnormalize.h"
#include "openfst/lib/equivalent.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/intersect.h"
#include "openfst/lib/invert.h"
#include "openfst/lib/matcher.h"
#include "openfst/lib/project.h"
#include "openfst/lib/properties.h"
#include "openfst/lib/randequivalent.h"
#include "openfst/lib/randgen.h"
#include "openfst/lib/rational.h"
#include "openfst/lib/relabel.h"
#include "openfst/lib/reverse.h"
#include "openfst/lib/rmepsilon.h"
#include "openfst/lib/shortest-distance.h"
#include "openfst/lib/shortest-path.h"
#include "openfst/lib/state-map.h"
#include "openfst/lib/symbol-table.h"
#include "openfst/lib/synchronize.h"
#include "openfst/lib/union.h"
#include "openfst/lib/util.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/lib/verify.h"
#include "openfst/lib/weight.h"

namespace fst {
namespace {

using Arc = StdArc;
using StateId = Arc::StateId;
using Weight = Arc::Weight;
using Label = Arc::Label;

class ErrorTest : public testing::Test {
 protected:
  void SetUp() override {
    trans_ilabeluns_cyc_nondeterm_.AddState();
    trans_ilabeluns_cyc_nondeterm_.SetStart(0);
    trans_ilabeluns_cyc_nondeterm_.SetFinal(0, Weight::One());
    trans_ilabeluns_cyc_nondeterm_.AddArc(0, Arc(2, 1, Weight::One(), 0));
    trans_ilabeluns_cyc_nondeterm_.AddArc(0, Arc(2, 2, Weight::One(), 0));
    trans_ilabeluns_cyc_nondeterm_.AddArc(0, Arc(1, 3, Weight::One(), 0));

    syms1_ = std::make_unique<SymbolTable>("syms1");
    syms1_->AddSymbol("a", 1);
    syms1_->AddSymbol("b", 2);
    trans_ilabeluns_cyc_nondeterm_.SetInputSymbols(syms1_.get());

    accept_ilabeluns_cyc_nondeterm_ = trans_ilabeluns_cyc_nondeterm_;
    Project(&accept_ilabeluns_cyc_nondeterm_, ProjectType::INPUT);

    syms2_ = std::make_unique<SymbolTable>("syms2");
    syms2_->AddSymbol("c", 1);
    syms2_->AddSymbol("d", 2);
    accept_ilabeluns_cyc_nondeterm_.SetInputSymbols(syms2_.get());
    accept_ilabeluns_cyc_nondeterm_.SetOutputSymbols(syms2_.get());

    accept_ilabelsort_cyc_nondeterm_ = trans_ilabeluns_cyc_nondeterm_;
    Project(&accept_ilabelsort_cyc_nondeterm_, ProjectType::OUTPUT);

    empty_nosyms_error_.SetProperties(kError, kError);

    trans_ilabeluns_cyc_nondeterm_error_ = trans_ilabeluns_cyc_nondeterm_;
    trans_ilabeluns_cyc_nondeterm_error_.SetProperties(kError, kError);

    accept_ilabeluns_cyc_nondeterm_error_ = accept_ilabeluns_cyc_nondeterm_;
    accept_ilabeluns_cyc_nondeterm_error_.SetProperties(kError, kError);

    accept_ilabelsort_cyc_nondeterm_error_ = accept_ilabelsort_cyc_nondeterm_;
    accept_ilabelsort_cyc_nondeterm_error_.SetProperties(kError, kError);

    nanweight_final_.AddState();
    nanweight_final_.SetStart(0);
    nanweight_final_.SetFinal(0, Weight::NoWeight());

    nanweight_arc_.AddState();
    nanweight_arc_.SetStart(0);
    nanweight_arc_.SetFinal(0, Weight::One());
    nanweight_arc_.AddArc(0, Arc(1, 3, Weight::NoWeight(), 0));
  }

  template <class M>
  void ComposeMatcherTest(const Fst<Arc>& fst1, const Fst<Arc>& fst2,
                          Label special_label) {
    ComposeFstOptions<Arc, M> copts(CacheOptions(),
                                    new M(fst1, MATCH_NONE, kNoLabel),
                                    new M(fst2, MATCH_INPUT, special_label));
    ComposeFst<Arc> cfst(fst1, fst2, copts);
    // Not yet a problem since unexpanded.
    ASSERT_FALSE(cfst.Properties(kError, false));
    VectorFst<Arc> vfst(cfst);
    // Now a problem since expanded.
    ASSERT_TRUE(cfst.Properties(kError, false));
  }

  std::unique_ptr<SymbolTable> syms1_;
  std::unique_ptr<SymbolTable> syms2_;

  VectorFst<Arc> empty_nosyms_;
  // Non-empty transducer, ilabel-unsorted, cyclic, non-deterministic, with
  // symbols.
  VectorFst<Arc> trans_ilabeluns_cyc_nondeterm_;
  // Non-empty acceptor, ilabel-unsorted, cyclic, non-determistic, with
  // symbols.
  VectorFst<Arc> accept_ilabeluns_cyc_nondeterm_;
  // Non-empty acceptor, ilabel-sorted, cyclic, non-deterministic, with
  // symbols.
  VectorFst<Arc> accept_ilabelsort_cyc_nondeterm_;
  VectorFst<Arc> empty_nosyms_error_;
  VectorFst<Arc> trans_ilabeluns_cyc_nondeterm_error_;
  VectorFst<Arc> accept_ilabeluns_cyc_nondeterm_error_;
  VectorFst<Arc> accept_ilabelsort_cyc_nondeterm_error_;
  // Contains NoWeight() final weight.
  VectorFst<Arc> nanweight_final_;
  // Contains NoWeight() arc weight.
  VectorFst<Arc> nanweight_arc_;
};

TEST_F(ErrorTest, VectorFstErrorTest) {
  ASSERT_TRUE(Verify(empty_nosyms_));
  ASSERT_TRUE(Verify(trans_ilabeluns_cyc_nondeterm_));
  ASSERT_TRUE(Verify(accept_ilabeluns_cyc_nondeterm_));
  ASSERT_TRUE(Verify(accept_ilabelsort_cyc_nondeterm_));
  ASSERT_FALSE(Verify(empty_nosyms_error_));
  ASSERT_FALSE(Verify(trans_ilabeluns_cyc_nondeterm_error_));
  ASSERT_FALSE(Verify(accept_ilabeluns_cyc_nondeterm_error_));
  ASSERT_FALSE(Verify(accept_ilabelsort_cyc_nondeterm_error_));
  ASSERT_FALSE(Verify(nanweight_final_));
  ASSERT_FALSE(Verify(nanweight_arc_));

  // kError is sticky.
  empty_nosyms_error_.SetProperties(0, kError);
  ASSERT_TRUE(empty_nosyms_error_.Properties(kError, false));

  VectorFst<Arc> vfst(empty_nosyms_error_);
  ASSERT_TRUE(vfst.Properties(kError, false));
}

TEST_F(ErrorTest, ConstFstErrorTest) {
  ConstFst<Arc> cfst(empty_nosyms_error_);
  ASSERT_TRUE(cfst.Properties(kError, false));
}

TEST_F(ErrorTest, CompactFstErrorTest) {
  CompactAcceptorFst<Arc> c1fst(empty_nosyms_error_);
  ASSERT_TRUE(c1fst.Properties(kError, false));
  CompactAcceptorFst<Arc> c2fst(trans_ilabeluns_cyc_nondeterm_error_);
  ASSERT_TRUE(c2fst.Properties(kError, false));
}

TEST_F(ErrorTest, ArcMapErrorTest) {
  VectorFst<Arc> ofst1, ofst2;
  IdentityArcMapper<Arc> mapper;
  ArcMap(empty_nosyms_error_, &ofst1, mapper);
  ASSERT_TRUE(ofst1.Properties(kError, false));

  ArcMap(trans_ilabeluns_cyc_nondeterm_error_, &ofst2, mapper);
  ASSERT_TRUE(ofst2.Properties(kError, false));

  ArcMapFst afst(empty_nosyms_error_, mapper);
  ASSERT_TRUE(afst.Properties(kError, false));
}

TEST_F(ErrorTest, ArcSortErrorTest) {
  ArcSortFst<Arc, ILabelCompare<Arc>> afst(empty_nosyms_error_,
                                           ILabelCompare<Arc>());
  ASSERT_TRUE(afst.Properties(kError, false));
}

TEST_F(ErrorTest, ComposeErrorTest) {
  VectorFst<Arc> ofst1, ofst2, ofst3, ofst4, ofst5;
  Compose(empty_nosyms_, empty_nosyms_error_, &ofst1);
  ASSERT_TRUE(ofst1.Properties(kError, false));

  Compose(empty_nosyms_error_, empty_nosyms_, &ofst2);
  ASSERT_TRUE(ofst2.Properties(kError, false));

  // Missing symbol table (OK).
  Compose(empty_nosyms_, accept_ilabeluns_cyc_nondeterm_, &ofst3);
  ASSERT_FALSE(ofst3.Properties(kError, false));

  // Non-matching symbol tables (not OK).
  Compose(accept_ilabeluns_cyc_nondeterm_, trans_ilabeluns_cyc_nondeterm_,
          &ofst3);
  ASSERT_TRUE(ofst3.Properties(kError, false));

  // Unsorted.
  Compose(accept_ilabeluns_cyc_nondeterm_, accept_ilabeluns_cyc_nondeterm_,
          &ofst4);
  ASSERT_TRUE(ofst4.Properties(kError, false));

  // Sigma matcher that matches on existing label.
  using TestSigmaMatcher = SigmaMatcher<Matcher<Fst<Arc>>>;
  ComposeMatcherTest<TestSigmaMatcher>(accept_ilabelsort_cyc_nondeterm_,
                                       accept_ilabelsort_cyc_nondeterm_, 1);

  // Rho matcher that matches on existing label.
  using TestRhoMatcher = RhoMatcher<Matcher<Fst<Arc>>>;
  ComposeMatcherTest<TestRhoMatcher>(accept_ilabelsort_cyc_nondeterm_,
                                     accept_ilabelsort_cyc_nondeterm_, 1);

  // Phi matcher that matches on existing label.
  using TestPhiMatcher = PhiMatcher<Matcher<Fst<Arc>>>;
  ComposeMatcherTest<TestPhiMatcher>(accept_ilabelsort_cyc_nondeterm_,
                                     accept_ilabelsort_cyc_nondeterm_, 1);
}

TEST_F(ErrorTest, ClosureErrorTest) {
  ClosureFst<Arc> cfst(empty_nosyms_error_, CLOSURE_STAR);
  ASSERT_TRUE(cfst.Properties(kError, false));
}

TEST_F(ErrorTest, ConcatErrorTest) {
  VectorFst<Arc> ofst1, ofst2(trans_ilabeluns_cyc_nondeterm_),
      ofst3(trans_ilabeluns_cyc_nondeterm_);
  VectorFst<Arc> ofst4, ofst5(empty_nosyms_);
  Concat(&ofst1, empty_nosyms_error_);
  ASSERT_TRUE(ofst1.Properties(kError, false));

  Concat(trans_ilabeluns_cyc_nondeterm_error_, &ofst2);
  ASSERT_TRUE(ofst2.Properties(kError, false));

  Concat(&ofst3, trans_ilabeluns_cyc_nondeterm_error_);
  ASSERT_TRUE(ofst3.Properties(kError, false));

  Concat(trans_ilabeluns_cyc_nondeterm_error_, &ofst4);
  ASSERT_TRUE(ofst4.Properties(kError, false));

  ConcatFst<Arc> cfst1(empty_nosyms_error_, empty_nosyms_);
  ASSERT_TRUE(cfst1.Properties(kError, false));

  ConcatFst<Arc> cfst2(empty_nosyms_, empty_nosyms_error_);
  ASSERT_TRUE(cfst2.Properties(kError, false));

  // non-matching symbols
  Concat(&ofst5, accept_ilabeluns_cyc_nondeterm_);
  ASSERT_TRUE(ofst3.Properties(kError, false));

  // Missing symbol table (OK).
  ConcatFst<Arc> cfst3(empty_nosyms_, accept_ilabeluns_cyc_nondeterm_);
  ASSERT_FALSE(cfst3.Properties(kError, false));

  // Non-matching symbol tables (not OK).
  ConcatFst<Arc> cfst4(trans_ilabeluns_cyc_nondeterm_,
                       accept_ilabeluns_cyc_nondeterm_);
  ASSERT_TRUE(cfst4.Properties(kError, false));
}

TEST_F(ErrorTest, DeterminizeErrorTest) {
  VectorFst<Arc> ofst1, ofst2;
  Determinize(empty_nosyms_error_, &ofst1);
  ASSERT_TRUE(ofst1.Properties(kError, false));

  // Non-functional.
  Determinize(trans_ilabeluns_cyc_nondeterm_, &ofst2);
  ASSERT_TRUE(ofst2.Properties(kError, false));
}

TEST_F(ErrorTest, DifferenceErrorTest) {
  VectorFst<Arc> ofst1, ofst2, ofst3(accept_ilabeluns_cyc_nondeterm_), ofst4;
  Difference(empty_nosyms_, empty_nosyms_error_, &ofst1);
  ASSERT_TRUE(ofst1.Properties(kError, false));

  Difference(empty_nosyms_error_, empty_nosyms_, &ofst2);
  ASSERT_TRUE(ofst2.Properties(kError, false));

  // Non-matching symbols.
  Difference(empty_nosyms_, accept_ilabeluns_cyc_nondeterm_, &ofst3);
  ASSERT_TRUE(ofst3.Properties(kError, false));

  // Unsorted and non-deterministic.
  Difference(accept_ilabeluns_cyc_nondeterm_, accept_ilabeluns_cyc_nondeterm_,
             &ofst4);
  ASSERT_TRUE(ofst4.Properties(kError, false));
}

TEST_F(ErrorTest, EpsNormalizeErrorTest) {
  VectorFst<Arc> ofst;
  EpsNormalize(empty_nosyms_error_, &ofst, EPS_NORM_INPUT);
  ASSERT_TRUE(ofst.Properties(kError, false));
}

TEST_F(ErrorTest, EquivalentErrorTest) {
  bool error;
  ASSERT_FALSE(Equivalent(empty_nosyms_error_, empty_nosyms_, kDelta, &error));
  ASSERT_TRUE(error);

  // Non-matching symbols.
  ASSERT_FALSE(Equivalent(empty_nosyms_, accept_ilabeluns_cyc_nondeterm_,
                          kDelta, &error));
  ASSERT_TRUE(error);

  // Non-deteterministic.
  ASSERT_FALSE(Equivalent(accept_ilabeluns_cyc_nondeterm_,
                          accept_ilabeluns_cyc_nondeterm_, kDelta, &error));
  ASSERT_TRUE(error);
}

TEST_F(ErrorTest, IntersectErrorTest) {
  VectorFst<Arc> ofst1, ofst2, ofst3, ofst4;
  Intersect(empty_nosyms_, empty_nosyms_error_, &ofst1);
  ASSERT_TRUE(ofst1.Properties(kError, false));

  Intersect(empty_nosyms_error_, empty_nosyms_, &ofst2);
  ASSERT_TRUE(ofst2.Properties(kError, false));

  // Missing symbol table (OK).
  Intersect(empty_nosyms_, accept_ilabeluns_cyc_nondeterm_, &ofst3);
  ASSERT_FALSE(ofst3.Properties(kError, false));

  // Non-matching symbol tables (not OK).
  Intersect(accept_ilabeluns_cyc_nondeterm_, trans_ilabeluns_cyc_nondeterm_,
            &ofst3);
  ASSERT_TRUE(ofst3.Properties(kError, false));

  // Unsorted.
  Intersect(accept_ilabeluns_cyc_nondeterm_, accept_ilabeluns_cyc_nondeterm_,
            &ofst4);
  ASSERT_TRUE(ofst4.Properties(kError, false));
}

TEST_F(ErrorTest, InvertErrorTest) {
  InvertFst<Arc> ifst(empty_nosyms_error_);
  ASSERT_TRUE(ifst.Properties(kError, false));
}

TEST_F(ErrorTest, ProjectErrorTest) {
  ProjectFst<Arc> pfst(empty_nosyms_error_, ProjectType::INPUT);
  ASSERT_TRUE(pfst.Properties(kError, false));
}

// Prune with non-path weight is a compile-time error.

TEST_F(ErrorTest, RandEquivalentErrorTest) {
  bool error;

  ASSERT_FALSE(RandEquivalent(empty_nosyms_error_, empty_nosyms_, 1, kDelta, 1,
                              1, &error));
  ASSERT_TRUE(error);

  // Missing symbol table (OK).
  ASSERT_TRUE(RandEquivalent(empty_nosyms_, accept_ilabeluns_cyc_nondeterm_, 1,
                             kDelta, 1, 1, &error));
  ASSERT_FALSE(error);

  // Non-matching symbol tables (not OK).
  ASSERT_FALSE(RandEquivalent(trans_ilabeluns_cyc_nondeterm_,
                              accept_ilabeluns_cyc_nondeterm_, 1, kDelta, 1, 1,
                              &error));
  ASSERT_TRUE(error);
}

TEST_F(ErrorTest, RandGenErrorTest) {
  VectorFst<Arc> ofst;
  RandGen(empty_nosyms_error_, &ofst);
  ASSERT_TRUE(ofst.Properties(kError, false));
}

TEST_F(ErrorTest, RelabelErrorTest) {
  RelabelFst<Arc> rfst(empty_nosyms_error_, syms1_.get(), syms1_.get());
  ASSERT_TRUE(rfst.Properties(kError, false));
}

TEST_F(ErrorTest, ReverseErrorTest) {
  VectorFst<Arc> ofst;
  Reverse(empty_nosyms_error_, &ofst);
  ASSERT_TRUE(ofst.Properties(kError, false));
}

TEST_F(ErrorTest, RmEpsilonErrorTest) {
  // Bad inputs.
  RmEpsilonFst<Arc> rfst1(empty_nosyms_error_);
  ASSERT_TRUE(rfst1.Properties(kError, false));
  RmEpsilonFst<Arc> rfst2(trans_ilabeluns_cyc_nondeterm_error_);
  ASSERT_TRUE(rfst2.Properties(kError, false));
  VectorFst<Arc> ofst2(trans_ilabeluns_cyc_nondeterm_error_);
  RmEpsilon(&ofst2);
  ASSERT_TRUE(ofst2.Properties(kError, false));

  // NaN on an epsilon transition.
  VectorFst<Arc> nanfst(nanweight_final_);
  Concat(&nanfst, trans_ilabeluns_cyc_nondeterm_);
  RmEpsilonFst<Arc> rfst3(nanfst);
  ASSERT_FALSE(rfst3.Properties(kError, false));
  rfst3.NumArcs(rfst3.Start());
  ASSERT_TRUE(rfst3.Properties(kError, false));
  VectorFst<Arc> ofst3(nanfst);
  RmEpsilon(&ofst3);
  ASSERT_TRUE(ofst3.Properties(kError, false));
}

TEST_F(ErrorTest, ShortestDistanceErrorTest) {
  for (const bool reverse : {false, true}) {
    std::vector<Weight> distance;
    ShortestDistance(empty_nosyms_error_, &distance, reverse);
    ASSERT_EQ(distance.size(), 1);
    ASSERT_FALSE(distance[0].Member());
    distance.clear();
    ShortestDistance(trans_ilabeluns_cyc_nondeterm_error_, &distance, reverse);
    ASSERT_EQ(distance.size(), 1);
    ASSERT_FALSE(distance[0].Member());
    distance.clear();
    ShortestDistance(nanweight_final_, &distance, reverse);
    ASSERT_EQ(distance.size(), 1);
    ASSERT_TRUE(reverse == 1 ? !distance[0].Member() : distance[0].Member());
    distance.clear();
    ShortestDistance(nanweight_arc_, &distance, reverse);
    ASSERT_EQ(distance.size(), 1);
    ASSERT_FALSE(distance[0].Member());
  }

  ASSERT_FALSE(ShortestDistance(empty_nosyms_error_).Member());
  ASSERT_FALSE(ShortestDistance(trans_ilabeluns_cyc_nondeterm_error_).Member());
  ASSERT_FALSE(ShortestDistance(nanweight_final_).Member());
  ASSERT_FALSE(ShortestDistance(nanweight_arc_).Member());
}

TEST_F(ErrorTest, ShortestPathErrorTest) {
  for (auto n = 1; n < 3; ++n) {
    // Bad inputs.
    VectorFst<Arc> ofst;
    ShortestPath(empty_nosyms_error_, &ofst, n);
    ASSERT_TRUE(ofst.Properties(kError, false));
    ShortestPath(trans_ilabeluns_cyc_nondeterm_error_, &ofst, n);
    ASSERT_TRUE(ofst.Properties(kError, false));
    ShortestPath(nanweight_final_, &ofst, n);
    ASSERT_TRUE(ofst.Properties(kError, false));
    ShortestPath(nanweight_arc_, &ofst, n);
    ASSERT_TRUE(ofst.Properties(kError, false));
    // Log semiring is a compilation error, so don't try to test it.
  }
}

TEST_F(ErrorTest, StateMapErrorTest) {
  VectorFst<Arc> ofst1, ofst2;
  IdentityStateMapper<Arc> mapper1(empty_nosyms_error_);
  IdentityStateMapper<Arc> mapper2(trans_ilabeluns_cyc_nondeterm_error_);

  StateMap(empty_nosyms_error_, &ofst1, mapper1);
  ASSERT_TRUE(ofst1.Properties(kError, false));

  StateMap(trans_ilabeluns_cyc_nondeterm_error_, &ofst2, mapper2);
  ASSERT_TRUE(ofst2.Properties(kError, false));

  StateMapFst<Arc, Arc, IdentityStateMapper<Arc>> afst(empty_nosyms_error_,
                                                       mapper1);
  ASSERT_TRUE(afst.Properties(kError, false));
}

TEST_F(ErrorTest, SynchronizeErrorTest) {
  VectorFst<Arc> ofst;

  Synchronize(empty_nosyms_error_, &ofst);
  ASSERT_TRUE(ofst.Properties(kError, false));

  SynchronizeFst<Arc> sfst(empty_nosyms_error_);
  ASSERT_TRUE(sfst.Properties(kError, false));
}

TEST_F(ErrorTest, UnionErrorTest) {
  VectorFst<Arc> ofst1, ofst2, ofst3(empty_nosyms_);
  Union(&ofst1, empty_nosyms_error_);
  ASSERT_TRUE(ofst1.Properties(kError, false));

  Union(&ofst2, trans_ilabeluns_cyc_nondeterm_error_);
  ASSERT_TRUE(ofst2.Properties(kError, false));

  UnionFst<Arc> ufst1(empty_nosyms_error_, empty_nosyms_);
  ASSERT_TRUE(ufst1.Properties(kError, false));

  UnionFst<Arc> ufst2(empty_nosyms_, empty_nosyms_error_);
  ASSERT_TRUE(ufst2.Properties(kError, false));

  // TODO: restore when voice actions issues fixed
  // non-matching symbols
  Union(&ofst3, accept_ilabeluns_cyc_nondeterm_);
  ASSERT_TRUE(ofst2.Properties(kError, false));

  // Missing symbol table (OK).
  UnionFst<Arc> ufst3(empty_nosyms_, accept_ilabeluns_cyc_nondeterm_);
  ASSERT_FALSE(ufst3.Properties(kError, false));

  // Non-matching symbol tables (not OK).
  UnionFst<Arc> ufst4(trans_ilabeluns_cyc_nondeterm_,
                      accept_ilabeluns_cyc_nondeterm_);
  ASSERT_TRUE(ufst4.Properties(kError, false));
}

}  // namespace
}  // namespace fst

int main(int argc, char** argv) {
  absl::SetFlag(&FLAGS_fst_error_fatal, false);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
