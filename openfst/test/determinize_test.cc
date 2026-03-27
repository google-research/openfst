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

#include <memory>
#include <string>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/equal.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/properties.h"
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

}  // namespace
}  // namespace fst
