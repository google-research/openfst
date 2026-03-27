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
// Unit test for weight factoring.

#include "openfst/lib/factor-weight.h"

#include <memory>
#include <string>
#include <utility>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/equal.h"
#include "openfst/lib/float-weight.h"
#include "openfst/lib/fst-decl.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/lib/verify.h"

namespace fst {
namespace {

using Arc = StdArc;
using StateId = Arc::StateId;
using Weight = Arc::Weight;
using Label = Arc::Label;

// Factor a TropicalWeight w as x+y with x = 1 if w > 1.
class LessThanOneFactor {
 public:
  explicit LessThanOneFactor(const TropicalWeight& w)
      : weight_(w), done_(w.Value() < 1.0) {}

  bool Done() const { return done_; }

  void Next() { done_ = true; }

  std::pair<TropicalWeight, TropicalWeight> Value() const {
    return std::make_pair(TropicalWeight(1.0),
                          TropicalWeight(weight_.Value() - 1.0));
  }

  void Reset() { done_ = weight_.Value() < 1.0; }

 private:
  TropicalWeight weight_;
  bool done_;
};

class FactorWeightTest : public testing::Test {
 protected:
  void SetUp() override {
    const std::string path = JoinPath(
        std::string("."),
        "openfst/test/testdata/factor-weight");
    const std::string fw1_name = JoinPath(path, "f1.fst");
    const std::string fw2_name = JoinPath(path, "f2.fst");
    const std::string fw3_name = JoinPath(path, "f3.fst");
    const std::string fw4_name = JoinPath(path, "f4.fst");
    const std::string fw5_name = JoinPath(path, "f5.fst");
    const std::string fw6_name = JoinPath(path, "f6.fst");

    fwfst1_.reset(VectorFst<Arc>::Read(fw1_name));
    fwfst2_.reset(VectorFst<Arc>::Read(fw2_name));
    fwfst3_.reset(VectorFst<Arc>::Read(fw3_name));
    fwfst4_.reset(VectorFst<Arc>::Read(fw4_name));
    fwfst5_.reset(VectorFst<Arc>::Read(fw5_name));
    fwfst6_.reset(VectorFst<Arc>::Read(fw6_name));
  }

  std::unique_ptr<const VectorFst<Arc>> fwfst1_;
  std::unique_ptr<const VectorFst<Arc>> fwfst2_;
  std::unique_ptr<const VectorFst<Arc>> fwfst3_;
  std::unique_ptr<const VectorFst<Arc>> fwfst4_;
  std::unique_ptr<const VectorFst<Arc>> fwfst5_;
  std::unique_ptr<const VectorFst<Arc>> fwfst6_;
};

TEST_F(FactorWeightTest, FactorWeightFst) {
  VectorFst<Arc> nfst;

  FactorWeightFst<Arc, LessThanOneFactor> fwfst(*fwfst1_);
  ASSERT_TRUE(Verify(fwfst));
  ASSERT_TRUE(Equal(*fwfst2_, fwfst));

  FactorWeightFst<Arc, LessThanOneFactor> nfwfst(nfst);
  ASSERT_TRUE(Verify(nfwfst));
  ASSERT_TRUE(Equal(nfst, nfwfst));

  // No weights.
  FactorWeightOptions<Arc> fnwopts;
  fnwopts.mode = 0;
  FactorWeightFst<Arc, LessThanOneFactor> fnwfst(*fwfst1_, fnwopts);
  ASSERT_TRUE(Verify(fnwfst));
  ASSERT_TRUE(Equal(*fwfst1_, fnwfst));

  // Arc weights.
  FactorWeightOptions<Arc> fawopts;
  fawopts.mode = kFactorArcWeights;
  FactorWeightFst<Arc, LessThanOneFactor> fawfst(*fwfst1_, fawopts);
  ASSERT_TRUE(Verify(fawfst));
  ASSERT_TRUE(Equal(*fwfst3_, fawfst));

  // Final weights.
  FactorWeightOptions<Arc> ffwopts;
  ffwopts.mode = kFactorFinalWeights;
  FactorWeightFst<Arc, LessThanOneFactor> ffwfst(*fwfst1_, ffwopts);
  ASSERT_TRUE(Verify(ffwfst));
  ASSERT_TRUE(Equal(*fwfst4_, ffwfst));

  for (const bool safe : {false, true}) {
    FactorWeightFst<Arc, LessThanOneFactor> cfst(fwfst, safe);
    ASSERT_TRUE(Verify(cfst));
    ASSERT_TRUE(Equal(*fwfst2_, cfst));
  }
}

TEST_F(FactorWeightTest, OneFactor) {
  FactorWeightOptions<Arc> fawopts;
  fawopts.mode = kFactorArcWeights;
  FactorWeightFst<Arc, OneFactor<Arc::Weight>> fwfst(*fwfst5_, fawopts);
  ASSERT_TRUE(Verify(fwfst));
  ASSERT_TRUE(Equal(*fwfst6_, fwfst));
}
}  // namespace
}  // namespace fst
