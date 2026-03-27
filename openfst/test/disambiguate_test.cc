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
// Unit test for Disambiguate.

#include "openfst/lib/disambiguate.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "absl/flags/flag.h"
#include "openfst/lib/arc-map.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/determinize.h"
#include "openfst/lib/equal.h"
#include "openfst/lib/expanded-fst.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/properties.h"
#include "openfst/lib/randequivalent.h"
#include "openfst/lib/randgen.h"
#include "openfst/lib/relabel.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/lib/verify.h"
#include "openfst/lib/weight.h"
#include "openfst/script/disambiguate.h"
#include "openfst/script/equal.h"
#include "openfst/script/fst-class.h"
#include "openfst/script/verify.h"
#include "openfst/script/weight-class.h"

ABSL_FLAG(uint64_t, seed, 403, "random seed");

namespace fst {
namespace {

using Arc = LogArc;
using StateId = Arc::StateId;
using Weight = Arc::Weight;
using Label = Arc::Label;

// Mapper that converts an acceptor into a transducer by
// incrementing the output labels by 1.
template <class A>
struct TransMapper {
  using FromArc = A;
  using ToArc = A;

  A operator()(const A& iarc) const {
    A oarc(iarc);
    if (iarc.nextstate != kNoStateId)  // non-final
      ++oarc.olabel;
    return oarc;
  }

  MapFinalAction FinalAction() const { return MAP_NO_SUPERFINAL; }

  MapSymbolsAction InputSymbolsAction() const { return MAP_COPY_SYMBOLS; }

  MapSymbolsAction OutputSymbolsAction() const { return MAP_COPY_SYMBOLS; }

  uint64_t Properties(uint64_t props) const {
    return props & kOLabelInvariantProperties;
  }
};

class DisambiguateTest : public ::testing::Test {
 protected:
  void SetUp() override {
    const std::string path = JoinPath(
        std::string("."),
        "openfst/test/testdata/disambiguate");
    const std::string disambiguate1_name = JoinPath(path, "d1.fst");
    const std::string disambiguate2_name = JoinPath(path, "d2.fst");
    const std::string disambiguate3_name = JoinPath(path, "d3.fst");
    const std::string disambiguate4_name = JoinPath(path, "d4.fst");
    const std::string disambiguate5_name = JoinPath(path, "d5.fst");
    const std::string disambiguate6_name = JoinPath(path, "d6.fst");
    const std::string disambiguate7_name = JoinPath(path, "d7.fst");

    // Ambiguous FSA1.
    dfst1_.reset(VectorFst<Arc>::Read(disambiguate1_name));
    // dfst2 = Disambiguate(dfst1)
    dfst2_.reset(VectorFst<Arc>::Read(disambiguate2_name));
    // Unambiguous FSA3.
    dfst3_.reset(VectorFst<Arc>::Read(disambiguate3_name));
    // dfst4 = Disambiguate(dfst3)
    dfst4_.reset(VectorFst<Arc>::Read(disambiguate4_name));
    // Ambiguous FSA5.
    dfst5_.reset(VectorFst<Arc>::Read(disambiguate5_name));
    // dfst6 = Disambiguate(dfst5)
    dfst6_.reset(VectorFst<Arc>::Read(disambiguate6_name));
    // dfst7 = PreDisambiguate(dfst1, w/ AllRelatedFilter)
    dfst7_.reset(VectorFst<Arc>::Read(disambiguate7_name));
  }

  // Tests (partially) if two FSTs have the same states
  // and arcs irrespective of state and arc orderings.
  bool UnorderedEqual(const Fst<Arc>& fst1, const Fst<Arc>& fst2) const {
    const UniformArcSelector<Arc> uniform_selector(0);
    const RandGenOptions<UniformArcSelector<Arc>> opts(uniform_selector, 25);
    const bool fst_equiv =
        RandEquivalent(fst1, fst2, /*npath=*/100, opts,
                       /*delta=*/.05, /*seed=*/absl::GetFlag(FLAGS_seed));
    const bool state_size_equiv = CountStates(fst1) == CountStates(fst2);
    const bool arc_size_equiv = CountArcs(fst1) == CountArcs(fst2);
    return fst_equiv && state_size_equiv && arc_size_equiv;
  }

  template <class T>
  struct AllRelated {
    bool operator()(const T& t1, const T& t2) const { return true; }
  };

  std::unique_ptr<const VectorFst<Arc>> dfst1_;
  std::unique_ptr<const VectorFst<Arc>> dfst2_;
  std::unique_ptr<const VectorFst<Arc>> dfst3_;
  std::unique_ptr<const VectorFst<Arc>> dfst4_;
  std::unique_ptr<const VectorFst<Arc>> dfst5_;
  std::unique_ptr<const VectorFst<Arc>> dfst6_;
  std::unique_ptr<const VectorFst<Arc>> dfst7_;
};

// Tests relation filter when R(p, q) <=> p == q.
TEST_F(DisambiguateTest, EqualFilterDeterminize) {
  using Divisor = DefaultCommonDivisor<Arc::Weight>;
  using Rel = std::equal_to<Arc::StateId>;
  using Filter = internal::RelationDeterminizeFilter<Arc, Rel>;
  DeterminizeFstOptions<Arc, Divisor, Filter> opts;
  DeterminizeFst<Arc> ofst(*dfst1_, opts);
  ASSERT_TRUE(Verify(ofst));
  ASSERT_TRUE(UnorderedEqual(*dfst1_, ofst));
}

// Tests relation filter when R(p, q) for all p, q.
TEST_F(DisambiguateTest, AllFilterDeterminize) {
  using Divisor = DefaultCommonDivisor<Arc::Weight>;
  using Rel = AllRelated<Arc::StateId>;
  using Filter = internal::RelationDeterminizeFilter<Arc, Rel>;
  DeterminizeFstOptions<Arc, Divisor, Filter> opts;
  DeterminizeFst<Arc> ofst(*dfst1_, opts);
  ASSERT_TRUE(Verify(ofst));
  ASSERT_TRUE(Equal(*dfst7_, ofst));
}

TEST_F(DisambiguateTest, Disambiguate) {
  VectorFst<Arc> nfst;
  VectorFst<Arc> ofst;

  Disambiguate(nfst, &ofst);
  ASSERT_TRUE(Verify(ofst));
  ASSERT_TRUE(Equal(nfst, ofst));

  Disambiguate(*dfst1_, &ofst);
  ASSERT_TRUE(Verify(ofst));
  ASSERT_TRUE(Equal(*dfst2_, ofst));

  Disambiguate(*dfst3_, &ofst);
  ASSERT_TRUE(Verify(ofst));

  ASSERT_TRUE(Equal(*dfst4_, ofst));

  Disambiguate(*dfst5_, &ofst);
  ASSERT_TRUE(Verify(ofst));
  ASSERT_TRUE(Equal(*dfst6_, ofst));
}

TEST_F(DisambiguateTest, FstClassDisambiguate) {
  namespace s = fst::script;

  s::FstClass dfst1(*dfst1_);
  s::FstClass dfst2(*dfst2_);
  s::VectorFstClass ofst(dfst1.ArcType());
  const s::WeightClass threshold(s::WeightClass::Zero(dfst1.WeightType()));
  const s::DisambiguateOptions opts(kDelta, threshold);
  Disambiguate(dfst1, &ofst, opts);
  ASSERT_TRUE(Verify(ofst));
  ASSERT_TRUE(Equal(dfst2, ofst));
}

TEST_F(DisambiguateTest, DisambiguateWithOutputs) {
  VectorFst<Arc> ofst;
  TransMapper<Arc> mapper;

  VectorFst<Arc> tfst1;
  VectorFst<Arc> tfst2;
  ArcMap(*dfst1_, &tfst1, mapper);
  ArcMap(*dfst2_, &tfst2, mapper);

  Disambiguate(tfst1, &ofst);
  ASSERT_TRUE(Verify(ofst));
  ASSERT_TRUE(Equal(tfst2, ofst));

  VectorFst<Arc> tfst3;
  VectorFst<Arc> tfst4;
  ArcMap(*dfst3_, &tfst3, mapper);
  ArcMap(*dfst4_, &tfst4, mapper);

  Disambiguate(tfst3, &ofst);
  ASSERT_TRUE(Verify(ofst));
  ASSERT_TRUE(Equal(tfst4, ofst));

  VectorFst<Arc> tfst5;
  VectorFst<Arc> tfst6;
  ArcMap(*dfst5_, &tfst5, mapper);
  ArcMap(*dfst6_, &tfst6, mapper);

  Disambiguate(tfst5, &ofst);
  ASSERT_TRUE(Verify(ofst));
  ASSERT_TRUE(Equal(tfst6, ofst));
}

TEST_F(DisambiguateTest, DisambiguateWithEpsilons) {
  VectorFst<Arc> ofst;
  for (Label l = 1; l <= 4; ++l) {  // Replace label l with epsilon
    VectorFst<Arc> efst1(*dfst1_);
    VectorFst<Arc> efst2(*dfst2_);
    VectorFst<Arc> efst3(*dfst3_);
    VectorFst<Arc> efst4(*dfst4_);
    VectorFst<Arc> efst5(*dfst5_);
    VectorFst<Arc> efst6(*dfst6_);

    std::pair<Label, Label> p(l, 0);
    std::vector<std::pair<Label, Label>> epairs(1, p);
    Relabel(&efst1, epairs, epairs);
    Relabel(&efst2, epairs, epairs);
    Relabel(&efst3, epairs, epairs);
    Relabel(&efst4, epairs, epairs);
    Relabel(&efst5, epairs, epairs);
    Relabel(&efst6, epairs, epairs);

    Disambiguate(efst1, &ofst);
    ASSERT_TRUE(Verify(ofst));
    ASSERT_TRUE(UnorderedEqual(efst2, ofst));

    Disambiguate(efst3, &ofst);
    ASSERT_TRUE(Verify(ofst));
    ASSERT_TRUE(UnorderedEqual(efst4, ofst));

    Disambiguate(efst5, &ofst);
    ASSERT_TRUE(Verify(ofst));
    ASSERT_TRUE(UnorderedEqual(efst6, ofst));
  }
}

}  // namespace
}  // namespace fst
