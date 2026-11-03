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
// Unit test for PDT expansion to an FST.

#include "openfst/extensions/pdt/expand.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "absl/flags/flag.h"
#include "absl/flags/reflection.h"
#include "absl/strings/string_view.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/equal.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/properties.h"
#include "openfst/lib/prune.h"
#include "openfst/lib/topsort.h"
#include "openfst/lib/util.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/lib/verify.h"

namespace fst {
namespace {

using Arc = StdArc;
using Label = Arc::Label;
using StateId = Arc::StateId;
using Weight = Arc::Weight;

constexpr absl::string_view kTestDir =
    "openfst/extensions/pdt/testdata";

class ExpandTest : public testing::Test {
 protected:
  void SetUp() override {
    const std::string expand1_name =
        JoinPath(std::string("."), kTestDir, "e1.fst");
    const std::string expand2_name =
        JoinPath(std::string("."), kTestDir, "e2.fst");
    const std::string expand3_name =
        JoinPath(std::string("."), kTestDir, "e3.fst");
    const std::string expand4_name =
        JoinPath(std::string("."), kTestDir, "e4.fst");

    const std::string parens_name =
        JoinPath(std::string("."), kTestDir, "eparen.pairs");

    efst1_.reset(VectorFst<Arc>::Read(expand1_name));
    // efst2_ = PdtExpand(efst1_).
    efst2_.reset(VectorFst<Arc>::Read(expand2_name));
    // efst3_ = PdtExpand(efst1_) with keep_parentheses = true.
    efst3_.reset(VectorFst<Arc>::Read(expand3_name));
    // Used for testing pruning.
    efst4_.reset(VectorFst<Arc>::Read(expand4_name));

    ASSERT_TRUE(ReadLabelPairs(parens_name, &parens_));
  }

  absl::FlagSaver flag_saver_;
  std::unique_ptr<const VectorFst<Arc>> efst1_;
  std::unique_ptr<const VectorFst<Arc>> efst2_;
  std::unique_ptr<const VectorFst<Arc>> efst3_;
  std::unique_ptr<const VectorFst<Arc>> efst4_;
  std::vector<std::pair<Label, Label>> parens_;
};

TEST_F(ExpandTest, PdtExpandFst) {
  PdtExpandFst<Arc> xfst(*efst1_, parens_);
  ASSERT_TRUE(Verify(xfst));

  PdtExpandFstOptions<Arc> opts;
  opts.keep_parentheses = true;
  PdtExpandFst<Arc> yfst(*efst1_, parens_, opts);
  ASSERT_TRUE(Verify(yfst));
}

TEST_F(ExpandTest, PdtExpand) {
  VectorFst<Arc> vfst;
  Expand(*efst1_, parens_, &vfst);
  ASSERT_TRUE(Verify(vfst));
  ASSERT_TRUE(Equal(*efst2_, vfst));

  Expand(*efst1_, parens_, &vfst, true, true);
  ASSERT_TRUE(Verify(vfst));
  ASSERT_TRUE(Equal(*efst3_, vfst));
}

TEST_F(ExpandTest, PdtExpandAndPrune) {
  VectorFst<Arc> vfst1;
  VectorFst<Arc> vfst2;
  for (float f = 0.0; f < 10.0; f += 1.0) {
    Expand(*efst4_, parens_, &vfst1);
    Prune(&vfst1, Arc::Weight(f));
    TopSort(&vfst1);

    Expand(*efst4_, parens_, &vfst2,
           PdtExpandOptions<Arc>(true, false, Arc::Weight(f)));
    TopSort(&vfst2);

    ASSERT_TRUE(Verify(vfst1));
    ASSERT_TRUE(Verify(vfst2));
    ASSERT_TRUE(Equal(vfst1, vfst2));
  }
}

TEST_F(ExpandTest, PdtExpandAndPruneSmallPdt) {
  // Small PDT (< 10 states in reversed FST) exercising PdtBalanceData::Reverse
  // when num_states < num_split.
  VectorFst<Arc> pfst;
  const auto s0 = pfst.AddState();
  const auto s1 = pfst.AddState();
  const auto s2 = pfst.AddState();
  const auto s3 = pfst.AddState();
  pfst.SetStart(s0);
  pfst.SetFinal(s3, Weight::One());
  const std::vector<std::pair<Label, Label>> parens = {{10, 11}};
  pfst.AddArc(s0, Arc(10, 10, Weight(0.5f), s1));
  pfst.AddArc(s1, Arc(1, 1, Weight(0.25f), s2));
  pfst.AddArc(s1, Arc(2, 2, Weight(2.25f), s2));
  pfst.AddArc(s2, Arc(11, 11, Weight(0.25f), s3));

  for (float f : {0.0f, 1.0f, 3.0f}) {
    VectorFst<Arc> vfst1;
    Expand(pfst, parens, &vfst1);
    Prune(&vfst1, Weight(f));
    TopSort(&vfst1);

    VectorFst<Arc> vfst2;
    Expand(pfst, parens, &vfst2, PdtExpandOptions<Arc>(true, false, Weight(f)));
    TopSort(&vfst2);

    ASSERT_TRUE(Verify(vfst1));
    ASSERT_TRUE(Verify(vfst2));
    EXPECT_TRUE(Equal(vfst1, vfst2));
  }
}

TEST_F(ExpandTest, PdtExpandAndPruneEmptyPdt) {
  const VectorFst<Arc> empty_fst;
  for (bool connect : {true, false}) {
    VectorFst<Arc> ofst;
    ofst.AddState();
    ofst.SetStart(0);
    Expand(empty_fst, parens_, &ofst,
           PdtExpandOptions<Arc>(connect, false, Weight::One()));
    ASSERT_TRUE(Verify(ofst));
    EXPECT_EQ(ofst.NumStates(), 0);
    EXPECT_EQ(ofst.Start(), kNoStateId);
  }
}

TEST_F(ExpandTest, DuplicateParenthesesSetsError) {
  absl::SetFlag(&FLAGS_fst_error_fatal, false);

  VectorFst<Arc> fst;
  fst.AddState();
  fst.SetStart(0);
  fst.SetFinal(0, Weight::One());

  for (const auto& bad_parens :
       std::vector<std::vector<std::pair<Label, Label>>>{
           {{10, 10}},
           {{10, 11}, {10, 12}},
           {{10, 11}, {12, 11}},
           {{10, 11}, {11, 12}}}) {
    PdtExpandFst<Arc> xfst(fst, bad_parens);
    EXPECT_TRUE(xfst.Properties(kError, true));
    EXPECT_EQ(xfst.Start(), kNoStateId);

    VectorFst<Arc> ofst;
    Expand(fst, bad_parens, &ofst);
    EXPECT_TRUE(ofst.Properties(kError, true));

    VectorFst<Arc> pruned_ofst;
    Expand(fst, bad_parens, &pruned_ofst,
           PdtExpandOptions<Arc>(true, false, Weight::One()));
    EXPECT_TRUE(pruned_ofst.Properties(kError, true));
  }
}

}  // namespace
}  // namespace fst
