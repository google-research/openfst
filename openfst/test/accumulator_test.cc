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
// Unit test for the Accumulator classes.

#include "openfst/lib/accumulator.h"

#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "openfst/lib/arc-map.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/float-weight.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/replace.h"
#include "openfst/lib/vector-fst.h"

namespace fst {
namespace {

using Arc = StdArc;
using Label = Arc::Label;
using StateId = Arc::StateId;
using Weight = Arc::Weight;

template <class A, class A1, class A2>
void TestAccumulator(const Fst<A>& fst, A1* accum1, A2* accum2) {
  using W = typename A::Weight;
  for (StateIterator<Fst<A>> siter(fst); !siter.Done(); siter.Next()) {
    const StateId s = siter.Value();
    accum1->SetState(s);
    accum2->SetState(s);
    ArcIterator<Fst<A>> aiter(fst, s);
    const size_t narcs = fst.NumArcs(s);
    const size_t mid = narcs / 2;
    W w1 = accum1->Sum(W::Zero(), &aiter, 0, mid);
    W w2 = accum2->Sum(W::Zero(), &aiter, 0, mid);
    EXPECT_TRUE(ApproxEqual(w1, w2))
        << s << ": " << w1.Value() << " " << w2.Value();
    w1 = accum1->Sum(W::Zero(), &aiter, mid, narcs);
    w2 = accum2->Sum(W::Zero(), &aiter, mid, narcs);
    EXPECT_TRUE(ApproxEqual(w1, w2))
        << s << ": " << w1.Value() << " " << w2.Value();
    w1 = accum1->Sum(W::Zero(), &aiter, 0, narcs);
    w2 = accum2->Sum(W::Zero(), &aiter, 0, narcs);
    EXPECT_TRUE(ApproxEqual(w1, w2))
        << s << ": " << w1.Value() << " " << w2.Value();
  }
}

class AccumulatorTest : public testing::Test {
 protected:
  void SetUp() override {
    fst_.reset(VectorFst<Arc>::Read(JoinPath(
        std::string("."),
        "openfst/test/testdata/accumulator/ac.fst")));
  }

  std::unique_ptr<const VectorFst<Arc>> fst_;
};

TEST_F(AccumulatorTest, LogAccumulatorTest) {
  using LArc = Log64Arc;
  auto accum = std::make_unique<LogAccumulator<LArc>>();
  VectorFst<LArc> lfst;
  ArcMap(*fst_, &lfst, WeightConvertMapper<Arc, LArc>());
  accum->Init(lfst);
  auto ref_accum = std::make_unique<DefaultAccumulator<LArc>>();
  TestAccumulator(lfst, accum.get(), ref_accum.get());

  LogAccumulator<LArc> copy_accum(*accum);
  DefaultAccumulator<LArc> copy_ref_accum(*ref_accum);
  accum = nullptr;
  ref_accum = nullptr;

  TestAccumulator(lfst, &copy_accum, &copy_ref_accum);
}

TEST_F(AccumulatorTest, FastLogAccumulatorTest) {
  auto accum = std::make_unique<FastLogAccumulator<Arc>>();
  accum->Init(*fst_);
  auto ref_accum = std::make_unique<LogAccumulator<Arc>>();
  TestAccumulator(*fst_, accum.get(), ref_accum.get());

  FastLogAccumulator<Arc> copy_accum(*accum);
  LogAccumulator<Arc> copy_ref_accum(*ref_accum);
  accum = nullptr;
  ref_accum = nullptr;

  TestAccumulator(*fst_, &copy_accum, &copy_ref_accum);
}

class ImmutableFastLogAccumulatorData : public FastLogAccumulatorData {
 public:
  ImmutableFastLogAccumulatorData() : FastLogAccumulatorData(20, 20) {}
  bool IsMutable() const override { return false; }
  void SetData(std::vector<double>* weights,
               std::vector<int>* weight_positions) override {}
};

VectorFst<Arc> SetWeightsToInfinity(const VectorFst<Arc>& fst) {
  VectorFst<Arc> inf_fst(fst);
  fst::TimesMapper<Arc> mapper(Arc::Weight::Zero());
  fst::ArcMap(&inf_fst, &mapper);
  return inf_fst;
}

TEST_F(AccumulatorTest, FastLogAccumulatorTestWithInfinity) {
  const auto inf_fst = SetWeightsToInfinity(*fst_);

  std::shared_ptr<FastLogAccumulatorData> immutable_data =
      std::make_shared<ImmutableFastLogAccumulatorData>();
  auto accum = std::make_unique<FastLogAccumulator<Arc>>(immutable_data);
  accum->Init(inf_fst);
  auto ref_accum = std::make_unique<LogAccumulator<Arc>>();
  TestAccumulator(inf_fst, accum.get(), ref_accum.get());

  FastLogAccumulator<Arc> copy_accum(*accum);
  LogAccumulator<Arc> copy_ref_accum(*ref_accum);

  TestAccumulator(inf_fst, &copy_accum, &copy_ref_accum);
}

TEST_F(AccumulatorTest, CacheLogAccumulatorTest) {
  LogAccumulator<Arc> ref_accum;
  // Test with GC disabled
  {
    auto accum = std::make_unique<CacheLogAccumulator<Arc>>();
    accum->Init(*fst_);
    TestAccumulator(*fst_, accum.get(), &ref_accum);

    for (const bool safe : {false, true}) {
      auto copy_accum =
          std::make_unique<CacheLogAccumulator<Arc>>(*accum, safe);
      accum = nullptr;
      TestAccumulator(*fst_, copy_accum.get(), &ref_accum);
      accum = std::move(copy_accum);
    }
  }
  // Test with GC enabled
  {
    auto accum =
        std::make_unique<CacheLogAccumulator<Arc>>(10, true, 4 * 20 * 2);
    accum->Init(*fst_);
    TestAccumulator(*fst_, accum.get(), &ref_accum);

    for (const bool safe : {false, true}) {
      auto copy_accum =
          std::make_unique<CacheLogAccumulator<Arc>>(*accum, safe);
      accum = nullptr;
      TestAccumulator(*fst_, copy_accum.get(), &ref_accum);
      accum = std::move(copy_accum);
    }
  }
}

TEST_F(AccumulatorTest, ReplaceAccumulatorTest) {
  VectorFst<Arc> root_fst;
  root_fst.SetStart(root_fst.AddState());
  Arc arc(0, -2, Weight::One(), root_fst.AddState());
  root_fst.AddArc(root_fst.Start(), arc);
  root_fst.SetFinal(arc.nextstate, Weight::One());
  std::vector<std::pair<Label, const Fst<Arc>*>> fst_tuples;
  fst_tuples.push_back(std::make_pair(-1, &root_fst));
  fst_tuples.push_back(std::make_pair(-2, fst_.get()));
  auto state_table =
      std::make_unique<DefaultReplaceStateTable<Arc>>(fst_tuples, -1);
  auto accum = std::make_unique<ReplaceAccumulator<FastLogAccumulator<Arc>>>();
  accum->Init(fst_tuples, state_table.get());
  ReplaceFstOptions<Arc> opts;
  opts.root = -1;
  opts.state_table = state_table.release();
  ReplaceFst<Arc> replace(fst_tuples, opts);
  LogAccumulator<Arc> ref_accum;
  TestAccumulator(replace, accum.get(), &ref_accum);

  ReplaceAccumulator<FastLogAccumulator<Arc>> copy_accum(*accum);
  accum = nullptr;
  TestAccumulator(replace, &copy_accum, &ref_accum);
}

}  // namespace
}  // namespace fst
