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
// Unit test for Determinize.

#include "openfst/lib/determinize.h"

#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "absl/flags/flag.h"
#include "absl/flags/reflection.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/equal.h"
#include "openfst/lib/float-weight.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/properties.h"
#include "openfst/lib/shortest-distance.h"
#include "openfst/lib/string-weight.h"
#include "openfst/lib/util.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/lib/verify.h"
#include "openfst/lib/weight.h"
#include "openfst/script/determinize.h"
#include "openfst/script/equal.h"
#include "openfst/script/fst-class.h"
#include "openfst/script/verify.h"
#include "openfst/script/weight-class.h"

namespace fst {
namespace {

using Arc = StdArc;

// Determinization filter that drops transitions with input label > max_label,
// and raises FSTERROR if constructed from an FST without max_label (mirroring
// Disambiguator's RelationDeterminizeFilter<Arc, CommonFuture>).
template <class A>
class MaxLabelDeterminizeFilter : public DefaultDeterminizeFilter<A> {
 public:
  using Base = DefaultDeterminizeFilter<A>;
  using typename Base::Element;
  using typename Base::Label;
  using typename Base::LabelMap;

  template <class B>
  struct rebind {
    using Other = MaxLabelDeterminizeFilter<B>;
  };

  explicit MaxLabelDeterminizeFilter(const Fst<A>& fst)
      : Base(fst), max_label_(std::numeric_limits<Label>::max()) {
    FSTERROR() << "MaxLabelDeterminizeFilter: max_label not provided";
  }

  MaxLabelDeterminizeFilter(const Fst<A>& fst, Label max_label)
      : Base(fst), max_label_(max_label) {}

  template <class Filter>
  MaxLabelDeterminizeFilter(const Fst<A>& fst, std::unique_ptr<Filter> filter)
      : Base(fst), max_label_(filter->MaxLabel()) {}

  MaxLabelDeterminizeFilter(const MaxLabelDeterminizeFilter& filter,
                            const Fst<A>* fst = nullptr)
      : Base(filter, fst), max_label_(filter.max_label_) {}

  bool FilterArc(const A& arc, const Element& src_element,
                 Element&& dest_element, LabelMap* label_map) const {
    if (arc.ilabel > max_label_) return false;
    return Base::FilterArc(arc, src_element, std::move(dest_element),
                           label_map);
  }

  Label MaxLabel() const { return max_label_; }

 private:
  Label max_label_;
};

// Pass-through subclass of DefaultDeterminizeFilter that preserves identical
// filtering semantics while forcing DeterminizeFsaImpl onto the generic
// LabelMap path (since std::is_same_v<Filter, DefaultDeterminizeFilter<A>> is
// false).
template <class A>
class PassthroughDeterminizeFilter : public DefaultDeterminizeFilter<A> {
 public:
  using Base = DefaultDeterminizeFilter<A>;

  template <class B>
  struct rebind {
    using Other = PassthroughDeterminizeFilter<B>;
  };

  explicit PassthroughDeterminizeFilter(const Fst<A>& fst) : Base(fst) {}

  template <class Filter>
  PassthroughDeterminizeFilter(const Fst<A>& fst,
                               std::unique_ptr<Filter> filter)
      : Base(fst, std::move(filter)) {}

  PassthroughDeterminizeFilter(const PassthroughDeterminizeFilter& filter,
                               const Fst<A>* fst = nullptr)
      : Base(filter, fst) {}
};

// Returns true iff `fst1` and `fst2` are equal with exactly equal weights.
// Equal() compares weights only up to kDelta by default.
template <class A>
bool ExactlyEqual(const Fst<A>& fst1, const Fst<A>& fst2) {
  using Weight = typename A::Weight;
  return Equal(fst1, fst2,
               [](const Weight& w1, const Weight& w2) { return w1 == w2; });
}

// Determinizes `fst` on the DefaultDeterminizeFilter fast path of
// DeterminizeFsaImpl::Expand.
template <class A>
VectorFst<A> DeterminizeOnDefaultPath(
    const Fst<A>& fst, DeterminizeType type = DETERMINIZE_FUNCTIONAL,
    float delta = kDelta) {
  DeterminizeFstOptions<A> opts;
  opts.delta = delta;
  opts.type = type;
  return VectorFst<A>(DeterminizeFst<A>(fst, opts));
}

// Determinizes `fst` on the generic LabelMap path of
// DeterminizeFsaImpl::Expand.
template <class A>
VectorFst<A> DeterminizeOnGenericPath(
    const Fst<A>& fst, DeterminizeType type = DETERMINIZE_FUNCTIONAL,
    float delta = kDelta) {
  DeterminizeFstOptions<A, DefaultCommonDivisor<typename A::Weight>,
                        PassthroughDeterminizeFilter<A>>
      opts;
  opts.delta = delta;
  opts.type = type;
  return VectorFst<A>(DeterminizeFst<A>(fst, opts));
}

// Returns a cyclic nondeterministic acceptor with unsorted labels, parallel
// arcs (same label and destination state), and a final state without outgoing
// arcs.
VectorFst<Arc> MakeParallelArcAcceptor() {
  VectorFst<Arc> fst;
  fst.AddStates(4);
  fst.SetStart(0);
  fst.SetFinal(3, Arc::Weight::One());
  fst.AddArc(0, Arc(3, 3, Arc::Weight(1.5f), 2));
  fst.AddArc(0, Arc(1, 1, Arc::Weight(2.0f), 1));
  fst.AddArc(0, Arc(1, 1, Arc::Weight(0.5f), 1));
  fst.AddArc(0, Arc(1, 1, Arc::Weight(1.0f), 2));
  fst.AddArc(0, Arc(2, 2, Arc::Weight(0.25f), 3));
  fst.AddArc(1, Arc(1, 1, Arc::Weight(0.75f), 0));
  fst.AddArc(1, Arc(2, 2, Arc::Weight(0.5f), 3));
  fst.AddArc(2, Arc(2, 2, Arc::Weight(1.25f), 3));
  fst.AddArc(2, Arc(2, 2, Arc::Weight(0.25f), 3));
  return fst;
}

class DeterminizeTest : public testing::Test {
 protected:
  void SetUp() override {
    const std::string path =
        JoinPath(std::string("."),
                       "openfst/test/testdata/determinize");
    const std::string determinize1_name = JoinPath(path, "d1.fst");
    const std::string determinize2_name = JoinPath(path, "d2.fst");
    const std::string determinize3_name = JoinPath(path, "d3.fst");
    const std::string determinize4_name = JoinPath(path, "d4.fst");
    const std::string determinize5_name = JoinPath(path, "d5.fst");
    const std::string determinize6_name = JoinPath(path, "d6.fst");
    const std::string determinize7_name = JoinPath(path, "d7.fst");
    const std::string determinize8_name = JoinPath(path, "d8.fst");
    const std::string determinize9_name = JoinPath(path, "d9.fst");

    dfst1_.reset(VectorFst<Arc>::Read(determinize1_name));
    dfst2_.reset(VectorFst<Arc>::Read(determinize2_name));
    dfst3_.reset(VectorFst<Arc>::Read(determinize3_name));
    dfst4_.reset(VectorFst<Arc>::Read(determinize4_name));
    dfst5_.reset(VectorFst<Arc>::Read(determinize5_name));
    dfst6_.reset(VectorFst<Arc>::Read(determinize6_name));
    dfst7_.reset(VectorFst<Arc>::Read(determinize7_name));
    dfst8_.reset(VectorFst<Arc>::Read(determinize8_name));
    dfst9_.reset(VectorFst<Arc>::Read(determinize9_name));
  }

  std::unique_ptr<const VectorFst<Arc>> dfst1_;
  std::unique_ptr<const VectorFst<Arc>> dfst2_;
  std::unique_ptr<const VectorFst<Arc>> dfst3_;
  std::unique_ptr<const VectorFst<Arc>> dfst4_;
  std::unique_ptr<const VectorFst<Arc>> dfst5_;
  std::unique_ptr<const VectorFst<Arc>> dfst6_;
  std::unique_ptr<const VectorFst<Arc>> dfst7_;
  std::unique_ptr<const VectorFst<Arc>> dfst8_;
  std::unique_ptr<const VectorFst<Arc>> dfst9_;
};

TEST_F(DeterminizeTest, AcceptorDeterminize) {
  VectorFst<Arc> nfst;

  DeterminizeFst<Arc> nofst(nfst);
  ASSERT_TRUE(Verify(nofst));
  ASSERT_TRUE(nofst.Properties(kIDeterministic, true));
  ASSERT_TRUE(Equal(nfst, nofst));

  DeterminizeFst<Arc> ofst(*dfst1_);
  ASSERT_TRUE(Verify(ofst));
  ASSERT_TRUE(ofst.Properties(kIDeterministic, true));
  ASSERT_TRUE(Equal(*dfst2_, ofst));

  for (const bool safe : {false, true}) {
    DeterminizeFst<Arc> cfst(ofst, safe);
    ASSERT_TRUE(Verify(cfst));
    ASSERT_TRUE(cfst.Properties(kIDeterministic, true));
    ASSERT_TRUE(Equal(*dfst2_, cfst));
  }
}

TEST_F(DeterminizeTest, TransducerDeterminize) {
  // Functional transducer.
  DeterminizeFst<Arc> ofst(*dfst3_);
  ASSERT_TRUE(Verify(ofst));
  ASSERT_TRUE(ofst.Properties(kIDeterministic, true));
  ASSERT_TRUE(Equal(*dfst4_, ofst));

  for (const bool safe : {false, true}) {
    SCOPED_TRACE(safe);
    DeterminizeFst<Arc> cfst(ofst, safe);
    ASSERT_TRUE(Verify(cfst));
    ASSERT_TRUE(cfst.Properties(kIDeterministic, true));
    ASSERT_TRUE(Equal(*dfst4_, cfst));
  }

  // Non-functional transducer.
  DeterminizeFstOptions<Arc> opts;
  opts.type = DETERMINIZE_NONFUNCTIONAL;
  DeterminizeFst<Arc> ofst2(*dfst6_, opts);
  ASSERT_TRUE(Verify(ofst2));
  ASSERT_FALSE(ofst2.Properties(kIDeterministic, true));
  ASSERT_TRUE(Equal(*dfst8_, ofst2));

  // Non-functional transducer w/ p-subsequential symbols.
  opts.subsequential_label = 4;
  opts.increment_subsequential_label = true;
  DeterminizeFst<Arc> ofst3(*dfst6_, opts);
  ASSERT_TRUE(Verify(ofst3));
  ASSERT_TRUE(ofst3.Properties(kIDeterministic, true));
  ASSERT_TRUE(ofst3.Properties(kNoIEpsilons, true));
  ASSERT_TRUE(Equal(*dfst9_, ofst3));

  for (const bool safe : {false, true}) {
    SCOPED_TRACE(safe);
    DeterminizeFst<Arc> cfst3(ofst3, safe);
    ASSERT_TRUE(Verify(cfst3));
    ASSERT_TRUE(cfst3.Properties(kIDeterministic, true));
    ASSERT_TRUE(cfst3.Properties(kNoIEpsilons, true));
    ASSERT_TRUE(Equal(*dfst9_, cfst3));
  }
}

TEST_F(DeterminizeTest, TransducerDeterminizeCopyPreservesFilter) {
  // Nondeterministic functional transducer with two parallel 1:10 transitions
  // (tropical weights 1.0 and 2.0 -> min 1.0) and a 2:20 transition that
  // MaxLabelDeterminizeFilter(..., /*max_label=*/1) filters out.
  VectorFst<Arc> ifst;
  ifst.AddStates(3);
  ifst.SetStart(0);
  ifst.SetFinal(1, Arc::Weight::One());
  ifst.SetFinal(2, Arc::Weight::One());
  ifst.AddArc(0, Arc(1, 10, Arc::Weight(1.0), 1));
  ifst.AddArc(0, Arc(1, 10, Arc::Weight(2.0), 1));
  ifst.AddArc(0, Arc(2, 20, Arc::Weight(0.5), 2));

  VectorFst<Arc> expected;
  expected.AddStates(2);
  expected.SetStart(0);
  expected.SetFinal(1, Arc::Weight::One());
  expected.AddArc(0, Arc(1, 10, Arc::Weight(1.0), 1));

  using Divisor = DefaultCommonDivisor<Arc::Weight>;
  using Filter = MaxLabelDeterminizeFilter<Arc>;
  DeterminizeFstOptions<Arc, Divisor, Filter> opts;
  opts.filter = new Filter(ifst, /*max_label=*/1);

  DeterminizeFst<Arc> ofst(ifst, opts);
  std::unique_ptr<DeterminizeFst<Arc>> unaccessed_copy(ofst.Copy(true));

  ASSERT_TRUE(Verify(ofst));
  EXPECT_EQ(ofst.Properties(kError, true), 0);
  EXPECT_TRUE(ofst.Properties(kIDeterministic, true));
  EXPECT_TRUE(Equal(expected, ofst));

  ASSERT_TRUE(Verify(*unaccessed_copy));
  EXPECT_EQ(unaccessed_copy->Properties(kError, true), 0);
  EXPECT_TRUE(unaccessed_copy->Properties(kIDeterministic, true));
  EXPECT_TRUE(Equal(expected, *unaccessed_copy));

  for (const bool safe : {false, true}) {
    SCOPED_TRACE(safe);
    DeterminizeFst<Arc> accessed_copy(ofst, safe);
    ASSERT_TRUE(Verify(accessed_copy));
    EXPECT_EQ(accessed_copy.Properties(kError, true), 0);
    EXPECT_TRUE(accessed_copy.Properties(kIDeterministic, true));
    EXPECT_TRUE(Equal(expected, accessed_copy));
  }
}

TEST_F(DeterminizeTest, TransducerDeterminizeWithStateTableError) {
  absl::FlagSaver fs;
  absl::SetFlag(&FLAGS_fst_error_fatal, false);

  using Filter = DefaultDeterminizeFilter<Arc>;
  using StateTable =
      DefaultDeterminizeStateTable<Arc, typename Filter::FilterState>;
  DeterminizeFstOptions<Arc> opts;
  opts.filter = new Filter(*dfst3_);
  opts.state_table = new StateTable();

  DeterminizeFst<Arc> ofst(*dfst3_, opts);
  EXPECT_EQ(ofst.Properties(kError, false), kError);
  EXPECT_EQ(ofst.Properties(kError, true), kError);
  EXPECT_EQ(ofst.Start(), kNoStateId);

  for (const bool safe : {false, true}) {
    DeterminizeFst<Arc> cfst(ofst, safe);
    EXPECT_EQ(cfst.Properties(kError, false), kError);
    EXPECT_EQ(cfst.Start(), kNoStateId);
  }

  VectorFst<Arc> vfst(ofst);
  EXPECT_EQ(vfst.Properties(kError, false), kError);
}

TEST_F(DeterminizeTest, PrunedDeterminize) {
  VectorFst<Arc> nfst, ofst;
  DeterminizeOptions<Arc> opts(kDelta, Arc::Weight(0.5), 10);

  Determinize(nfst, &ofst, opts);
  ASSERT_TRUE(Verify(ofst));
  ASSERT_TRUE(ofst.Properties(kIDeterministic, true));
  ASSERT_TRUE(Equal(nfst, ofst));

  Determinize(*dfst1_, &ofst, opts);
  ASSERT_TRUE(Verify(ofst));
  ASSERT_TRUE(ofst.Properties(kIDeterministic, true));
  ASSERT_TRUE(Equal(*dfst5_, ofst));
}

// Builds a nondeterministic acceptor with two paths sharing prefix label 1:
// - suboptimal path 1 -> 2 with weight 0.1 + 0.4 = 0.5, and
// - optimal path 1 -> 3 with weight 1.0 + (-1.0) = 0.0.
// On label 1, DeterminizeFsaImpl factors out min(0.1, 1.0) = 0.1 and quantizes
// the optimal branch's residual 0.9 upward (e.g., to 922/1024 = 0.900390625 at
// kDelta, or 1.0 at delta = 0.25), shifting its DFA path estimate slightly
// above the exact NFA shortest distance 0.0.
VectorFst<Arc> MakeQuantizedResidualAcceptor() {
  VectorFst<Arc> ifst;
  ifst.AddStates(5);
  ifst.SetStart(0);
  ifst.AddArc(0, Arc(1, 1, Arc::Weight(0.1f), 1));
  ifst.AddArc(1, Arc(2, 2, Arc::Weight(0.4f), 2));
  ifst.SetFinal(2, Arc::Weight::One());
  ifst.AddArc(0, Arc(1, 1, Arc::Weight(1.0f), 3));
  ifst.AddArc(3, Arc(3, 3, Arc::Weight(-1.0f), 4));
  ifst.SetFinal(4, Arc::Weight::One());
  return ifst;
}

// Verifies that pruned acceptor Determinize with weight_threshold = One()
// preserves the optimal path when subset residual quantization rounds its DFA
// distance estimate upward within kDelta.
TEST_F(DeterminizeTest, PrunedAcceptorDeterminizeKeepsQuantizedShortestPath) {
  const VectorFst<Arc> ifst = MakeQuantizedResidualAcceptor();
  VectorFst<Arc> ofst;
  const DeterminizeOptions<Arc> opts(kDelta, Arc::Weight::One());

  Determinize(ifst, &ofst, opts);

  VectorFst<Arc> expected;
  expected.AddStates(3);
  expected.SetStart(0);
  expected.AddArc(0, Arc(1, 1, Arc::Weight(0.1f), 1));
  expected.AddArc(1, Arc(3, 3, Arc::Weight(-0.099609375f), 2));
  expected.SetFinal(2, Arc::Weight::One());
  ASSERT_TRUE(Verify(ofst));
  EXPECT_TRUE(ofst.Properties(kIDeterministic, true));
  EXPECT_TRUE(Equal(expected, ofst));
}

// Builds a 4-state acyclic acceptor with two zero-weight optimal paths
// (0 --3/0--> 2 and 0 --2/0--> 1 --2/0--> 3 --2/0--> 2) alongside parallel
// step_weight arcs on 1 -> 3 and 3 -> 2. When Reverse(ifst) is relaxed with
// delta >= step_weight, both 2/0 relaxations are suppressed and idistance[1]
// accumulates 2 * step_weight.
VectorFst<Arc> MakeMultiHopRelaxationAcceptor(float step_weight) {
  VectorFst<Arc> ifst;
  ifst.AddStates(4);
  ifst.SetStart(0);
  ifst.AddArc(0, Arc(2, 2, Arc::Weight::One(), 1));
  ifst.AddArc(0, Arc(3, 3, Arc::Weight::One(), 2));
  ifst.SetFinal(2, Arc::Weight::One());
  ifst.AddArc(1, Arc(1, 1, Arc::Weight(step_weight), 3));
  ifst.AddArc(1, Arc(2, 2, Arc::Weight::One(), 3));
  ifst.AddArc(3, Arc(1, 1, Arc::Weight(step_weight), 2));
  ifst.AddArc(3, Arc(2, 2, Arc::Weight::One(), 2));
  return ifst;
}

// Verifies that pruned acceptor Determinize caps ShortestDistance relaxation
// delta at kShortestDelta when opts.delta > kShortestDelta, while forwarding
// tighter caller deltas when 0 < opts.delta < kShortestDelta.
TEST_F(DeterminizeTest, PrunedAcceptorDeterminizeCapsShortestDistanceDelta) {
  // At step_weight = 7/8192 < kDelta, passing kDelta directly to
  // ShortestDistance would leave idistance[1] = 14/8192 > kDelta and prune the
  // zero-weight path 2 -> 2 -> 2.
  const VectorFst<Arc> coarse_ifst =
      MakeMultiHopRelaxationAcceptor(7.0f / 8192.0f);
  VectorFst<Arc> coarse_ofst;
  Determinize(coarse_ifst, &coarse_ofst,
              DeterminizeOptions<Arc>(kDelta, Arc::Weight::One()));
  ASSERT_TRUE(Verify(coarse_ofst));
  EXPECT_TRUE(coarse_ofst.Properties(kIDeterministic, true));
  EXPECT_TRUE(Equal(coarse_ifst, coarse_ofst));

  // At step_weight = 7e-7 < kShortestDelta and opts.delta = 1e-7, leaving
  // ShortestDistance at default kShortestDelta would leave
  // idistance[1] = 1.4e-6 > 1e-7 and prune 2 -> 2 -> 2.
  const VectorFst<Arc> tight_ifst = MakeMultiHopRelaxationAcceptor(7e-7f);
  VectorFst<Arc> tight_ofst;
  Determinize(tight_ifst, &tight_ofst,
              DeterminizeOptions<Arc>(/*delta=*/1e-7f, Arc::Weight::One()));

  VectorFst<Arc> expected_tight;
  expected_tight.AddStates(4);
  expected_tight.SetStart(0);
  expected_tight.AddArc(0, Arc(2, 2, Arc::Weight::One(), 1));
  expected_tight.AddArc(0, Arc(3, 3, Arc::Weight::One(), 2));
  expected_tight.SetFinal(2, Arc::Weight::One());
  expected_tight.AddArc(1, Arc(2, 2, Arc::Weight::One(), 3));
  expected_tight.AddArc(3, Arc(2, 2, Arc::Weight::One(), 2));
  ASSERT_TRUE(Verify(tight_ofst));
  EXPECT_TRUE(tight_ofst.Properties(kIDeterministic, true));
  EXPECT_TRUE(Equal(expected_tight, tight_ofst));
}

// Verifies that pruned Determinize forwards custom opts.delta appropriately on
// both the acceptor and transducer paths.
TEST_F(DeterminizeTest, PrunedDeterminizeForwardsCustomDelta) {
  const VectorFst<Arc> acceptor = MakeQuantizedResidualAcceptor();
  VectorFst<Arc> acceptor_ofst;
  // At delta = 0.25, residual 0.9 quantizes to 1.0 (+0.1 > kDelta), which
  // exceeds weight_threshold = One() unless opts.delta is forwarded to Prune.
  const DeterminizeOptions<Arc> coarse_opts(/*delta=*/0.25f,
                                            Arc::Weight::One());

  Determinize(acceptor, &acceptor_ofst, coarse_opts);

  VectorFst<Arc> expected_acceptor;
  expected_acceptor.AddStates(3);
  expected_acceptor.SetStart(0);
  expected_acceptor.AddArc(0, Arc(1, 1, Arc::Weight(0.1f), 1));
  expected_acceptor.AddArc(1, Arc(3, 3, Arc::Weight::One(), 2));
  expected_acceptor.SetFinal(2, Arc::Weight::One());
  ASSERT_TRUE(Verify(acceptor_ofst));
  EXPECT_TRUE(acceptor_ofst.Properties(kIDeterministic, true));
  EXPECT_TRUE(Equal(expected_acceptor, acceptor_ofst));

  // On the transducer path, tight 0 < opts.delta < kDelta (1e-9f, smaller than
  // the 1-ULP gap 2^-24 ≈ 5.96e-8f) is forwarded to Prune, pruning a
  // single-path transducer whose forward weight rounds up by 1 ULP
  // (-0.99999988f > -0.99999994f), whereas default kDelta keeps it.
  VectorFst<Arc> float_transducer;
  float_transducer.AddStates(3);
  float_transducer.SetStart(0);
  float_transducer.AddArc(0, Arc(1, 10, Arc::Weight(-1.0f), 1));
  float_transducer.AddArc(1, Arc(2, 20, Arc::Weight(4e-8f), 2));
  float_transducer.SetFinal(2, Arc::Weight(4e-8f));

  VectorFst<Arc> default_transducer_ofst;
  Determinize(float_transducer, &default_transducer_ofst,
              DeterminizeOptions<Arc>(kDelta, Arc::Weight::One()));
  ASSERT_TRUE(Verify(default_transducer_ofst));
  EXPECT_TRUE(Equal(float_transducer, default_transducer_ofst));

  VectorFst<Arc> tight_transducer_ofst;
  Determinize(float_transducer, &tight_transducer_ofst,
              DeterminizeOptions<Arc>(/*delta=*/1e-9f, Arc::Weight::One()));
  VectorFst<Arc> expected_tight_transducer;
  expected_tight_transducer.AddStates(2);
  expected_tight_transducer.SetStart(0);
  expected_tight_transducer.AddArc(0, Arc(1, 10, Arc::Weight(-1.0f), 1));
  ASSERT_TRUE(Verify(tight_transducer_ofst));
  EXPECT_TRUE(Equal(expected_tight_transducer, tight_transducer_ofst));
  EXPECT_EQ(ShortestDistance(tight_transducer_ofst), Arc::Weight::Zero());

  // Coarse subset-quantization opts.delta (0.25) on the transducer path is
  // capped at kDelta for Prune, so suboptimal path 1:10 -> 2:20 (weight 0.1)
  // is pruned when optimal path 3:30 -> 4:40 has weight 0.0.
  VectorFst<Arc> transducer;
  transducer.AddStates(5);
  transducer.SetStart(0);
  transducer.AddArc(0, Arc(1, 10, Arc::Weight(0.1f), 1));
  transducer.AddArc(1, Arc(2, 20, Arc::Weight::One(), 2));
  transducer.SetFinal(2, Arc::Weight::One());
  transducer.AddArc(0, Arc(3, 30, Arc::Weight::One(), 3));
  transducer.AddArc(3, Arc(4, 40, Arc::Weight::One(), 4));
  transducer.SetFinal(4, Arc::Weight::One());

  VectorFst<Arc> transducer_ofst;
  Determinize(transducer, &transducer_ofst, coarse_opts);
  VectorFst<Arc> expected_transducer;
  expected_transducer.AddStates(3);
  expected_transducer.SetStart(0);
  expected_transducer.AddArc(0, Arc(3, 30, Arc::Weight::One(), 1));
  expected_transducer.AddArc(1, Arc(4, 40, Arc::Weight::One(), 2));
  expected_transducer.SetFinal(2, Arc::Weight::One());
  ASSERT_TRUE(Verify(transducer_ofst));
  EXPECT_TRUE(transducer_ofst.Properties(kIDeterministic, true));
  EXPECT_TRUE(Equal(expected_transducer, transducer_ofst));
}

TEST_F(DeterminizeTest, MinDeterminize) {
  VectorFst<Arc> ofst;
  DeterminizeOptions<Arc> opts;
  opts.type = DETERMINIZE_DISAMBIGUATE;

  Determinize(*dfst6_, &ofst, opts);
  ASSERT_TRUE(Verify(ofst));
  ASSERT_TRUE(ofst.Properties(kIDeterministic, true));
  ASSERT_TRUE(Equal(*dfst7_, ofst));
}

TEST_F(DeterminizeTest, FstClassDeterminize) {
  namespace s = fst::script;

  s::FstClass dfst1(*dfst1_);
  s::FstClass dfst5(*dfst5_);
  s::VectorFstClass ofst(dfst1.ArcType());

  const s::WeightClass threshold1(dfst1.WeightType(), "0.5");
  const s::DeterminizeOptions opts1(kDelta, threshold1, 10);
  Determinize(dfst1, &ofst, opts1);
  ASSERT_TRUE(ofst.Properties(kIDeterministic, true));
  ASSERT_TRUE(Equal(dfst5, ofst));

  s::FstClass dfst6(*dfst6_);
  s::FstClass dfst7(*dfst7_);
  const s::WeightClass threshold6(s::WeightClass::Zero(dfst6.WeightType()));
  const s::DeterminizeOptions opts6(kDelta, threshold6, kNoStateId, 0,
                                    DETERMINIZE_DISAMBIGUATE);
  Determinize(dfst6, &ofst, opts6);
  ASSERT_TRUE(Verify(ofst));
  ASSERT_TRUE(ofst.Properties(kIDeterministic, true));
  ASSERT_TRUE(Equal(dfst7, ofst));
}

// The DefaultDeterminizeFilter fast path of DeterminizeFsaImpl::Expand must
// give exactly the same result, including weights, as the generic LabelMap
// path used by other filters.
TEST_F(DeterminizeTest, DefaultExpandMatchesGenericPathOnAcceptors) {
  const VectorFst<Arc> nfa = MakeParallelArcAcceptor();

  EXPECT_TRUE(ExactlyEqual(DeterminizeOnGenericPath(nfa),
                           DeterminizeOnDefaultPath(nfa)));
  EXPECT_TRUE(ExactlyEqual(DeterminizeOnGenericPath(*dfst1_),
                           DeterminizeOnDefaultPath(*dfst1_)));

  // Non-default `opts.delta` (`0.5f`) collapses residuals `0.1f` and `0.2f` to
  // `0.0f` (`2` DFA states), whereas default `kDelta` keeps them distinct (`3`
  // DFA states), verifying that ExpandDefault quantizes with `delta_`.
  VectorFst<Arc> coarse_nfa;
  coarse_nfa.AddStates(3);
  coarse_nfa.SetStart(0);
  coarse_nfa.SetFinal(1, Arc::Weight::One());
  coarse_nfa.SetFinal(2, Arc::Weight::One());
  coarse_nfa.AddArc(0, Arc(1, 1, Arc::Weight::One(), 1));
  coarse_nfa.AddArc(0, Arc(1, 1, Arc::Weight(0.1f), 2));
  coarse_nfa.AddArc(0, Arc(2, 2, Arc::Weight::One(), 1));
  coarse_nfa.AddArc(0, Arc(2, 2, Arc::Weight(0.2f), 2));

  EXPECT_EQ(DeterminizeOnDefaultPath(coarse_nfa).NumStates(), 3);
  const VectorFst<Arc> default_coarse =
      DeterminizeOnDefaultPath(coarse_nfa, DETERMINIZE_FUNCTIONAL, 0.5f);
  EXPECT_EQ(default_coarse.NumStates(), 2);
  EXPECT_TRUE(ExactlyEqual(
      DeterminizeOnGenericPath(coarse_nfa, DETERMINIZE_FUNCTIONAL, 0.5f),
      default_coarse));
}

// Same as above for transducers, which are determinized as acceptors over
// gallic weights.
TEST_F(DeterminizeTest, DefaultExpandMatchesGenericPathOnTransducers) {
  EXPECT_TRUE(ExactlyEqual(DeterminizeOnGenericPath(*dfst3_),
                           DeterminizeOnDefaultPath(*dfst3_)));
  EXPECT_TRUE(ExactlyEqual(
      DeterminizeOnGenericPath(*dfst6_, DETERMINIZE_NONFUNCTIONAL),
      DeterminizeOnDefaultPath(*dfst6_, DETERMINIZE_NONFUNCTIONAL)));
  EXPECT_TRUE(ExactlyEqual(
      DeterminizeOnGenericPath(*dfst6_, DETERMINIZE_DISAMBIGUATE),
      DeterminizeOnDefaultPath(*dfst6_, DETERMINIZE_DISAMBIGUATE)));
}

// Under DETERMINIZE_DISAMBIGUATE, parallel arcs with equal weights and
// different output labels keep the output of the first arc: Plus() for
// GALLIC_MIN returns its second argument on a tie, and parallel arcs are
// combined last arc first.
TEST_F(DeterminizeTest, DefaultExpandKeepsFirstOutputOnDisambiguateTie) {
  VectorFst<Arc> ifst;
  ifst.AddStates(2);
  ifst.SetStart(0);
  ifst.SetFinal(1, Arc::Weight::One());
  ifst.AddArc(0, Arc(1, 10, Arc::Weight(1.0f), 1));
  ifst.AddArc(0, Arc(1, 20, Arc::Weight(1.0f), 1));

  VectorFst<Arc> expected;
  expected.AddStates(3);
  expected.SetStart(0);
  expected.SetFinal(2, Arc::Weight::One());
  expected.AddArc(0, Arc(1, 0, Arc::Weight(1.0f), 1));
  expected.AddArc(1, Arc(0, 10, Arc::Weight::One(), 2));

  EXPECT_TRUE(ExactlyEqual(
      expected, DeterminizeOnDefaultPath(ifst, DETERMINIZE_DISAMBIGUATE)));
}

// LogWeight Plus() is not associative in floating point, so three or more
// parallel arcs must be combined in the same order on both paths for the arc
// weights to be exactly equal.
TEST_F(DeterminizeTest, DefaultExpandMatchesGenericPathOnLogWeights) {
  VectorFst<LogArc> nfa;
  nfa.AddStates(3);
  nfa.SetStart(0);
  nfa.SetFinal(1, LogWeight::One());
  nfa.SetFinal(2, LogWeight(0.3f));
  // Label 1 uses an explicit IEEE-754 absorption triple where forward
  // Plus(Plus(1.5f, 18.5f), 18.5f) == 1.5f differs by 1 ULP from reverse
  // Plus(Plus(18.5f, 18.5f), 1.5f) == 1.49999988f.
  nfa.AddArc(0, LogArc(1, 1, LogWeight(1.5f), 1));
  nfa.AddArc(0, LogArc(1, 1, LogWeight(18.5f), 1));
  nfa.AddArc(0, LogArc(1, 1, LogWeight(18.5f), 1));
  // Labels 2..8 combine three parallel arcs to state 1 alongside a second
  // destination state 2 to exercise multi-destination common_divisor_ and
  // LogWeight residual Divide/Quantize.
  for (int label = 2; label <= 8; ++label) {
    nfa.AddArc(0, LogArc(label, label, LogWeight(0.1f * label), 1));
    nfa.AddArc(0, LogArc(label, label, LogWeight(0.37f * label), 1));
    nfa.AddArc(0, LogArc(label, label, LogWeight(0.71f * label), 1));
    nfa.AddArc(0, LogArc(label, label, LogWeight(0.53f * label), 2));
  }

  EXPECT_TRUE(ExactlyEqual(DeterminizeOnGenericPath(nfa),
                           DeterminizeOnDefaultPath(nfa)));
}

// The output distances computed for pruned determinization are the same on
// both paths.
TEST_F(DeterminizeTest, DefaultExpandMatchesGenericPathOnDistances) {
  const VectorFst<Arc> nfa = MakeParallelArcAcceptor();
  const std::vector<Arc::Weight> in_dist(nfa.NumStates(), Arc::Weight(1.0f));
  std::vector<Arc::Weight> default_out_dist;
  std::vector<Arc::Weight> generic_out_dist;
  const DeterminizeFst<Arc> default_det(nfa, &in_dist, &default_out_dist,
                                        DeterminizeFstOptions<Arc>());
  const DeterminizeFst<Arc> generic_det(
      nfa, &in_dist, &generic_out_dist,
      DeterminizeFstOptions<Arc, DefaultCommonDivisor<Arc::Weight>,
                            PassthroughDeterminizeFilter<Arc>>());

  // Expanding the FSTs fills in the output distances.
  EXPECT_TRUE(
      ExactlyEqual(VectorFst<Arc>(generic_det), VectorFst<Arc>(default_det)));
  ASSERT_FALSE(default_out_dist.empty());
  EXPECT_EQ(generic_out_dist, default_out_dist);
}

// Copies of a partially expanded DeterminizeFst expand to the same result as
// the original.
TEST_F(DeterminizeTest, DefaultExpandSupportsCopiesOfPartiallyExpandedFst) {
  const VectorFst<Arc> nfa = MakeParallelArcAcceptor();
  const VectorFst<Arc> expected = DeterminizeOnGenericPath(nfa);
  const DeterminizeFst<Arc> partial(nfa);
  ASSERT_NE(partial.Start(), kNoStateId);
  // Expands the start state only.
  const ArcIterator<DeterminizeFst<Arc>> aiter(partial, partial.Start());
  ASSERT_FALSE(aiter.Done());

  for (const bool safe : {false, true}) {
    SCOPED_TRACE(safe);
    const DeterminizeFst<Arc> copy(partial, safe);
    EXPECT_TRUE(ExactlyEqual(expected, copy));
  }
}

// A sum of parallel arcs that is not a member of the semiring sets kError as
// soon as the state is expanded, before any final weight is computed. The
// input is an acceptor over GALLIC_RESTRICT weights, whose Plus() fails on
// unequal strings.
TEST_F(DeterminizeTest, DefaultExpandSetsErrorOnNonMemberSum) {
  const absl::FlagSaver fs;
  absl::SetFlag(&FLAGS_fst_error_fatal, false);

  using GArc = GallicArc<Arc, GALLIC_RESTRICT>;
  using GWeight = GArc::Weight;
  VectorFst<GArc> nfa;
  nfa.AddStates(2);
  nfa.SetStart(0);
  nfa.SetFinal(1, GWeight::One());
  nfa.AddArc(0, GArc(1, 1, GWeight(GWeight::SW(10), Arc::Weight::One()), 1));
  nfa.AddArc(0, GArc(1, 1, GWeight(GWeight::SW(20), Arc::Weight::One()), 1));

  const DeterminizeFst<GArc> det(nfa);
  ASSERT_NE(det.Start(), kNoStateId);
  ASSERT_EQ(det.Properties(kError, false), 0);
  // Expands the start state only.
  const ArcIterator<DeterminizeFst<GArc>> aiter(det, det.Start());

  EXPECT_EQ(det.Properties(kError, false), kError);
}

}  // namespace
}  // namespace fst
