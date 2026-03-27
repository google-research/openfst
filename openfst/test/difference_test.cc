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
// Unit test for Difference.

#include "openfst/lib/difference.h"

#include <memory>
#include <string>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/compose.h"
#include "openfst/lib/equal.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/lib/verify.h"
#include "openfst/script/difference.h"
#include "openfst/script/equal.h"
#include "openfst/script/fst-class.h"

namespace fst {
namespace {

using Arc = StdArc;
using StateId = Arc::StateId;
using Weight = Arc::Weight;
using Label = Arc::Label;

class DifferenceTest : public testing::Test {
 protected:
  void SetUp() override {
    const std::string path =
        JoinPath(std::string("."),
                       "openfst/test/testdata/difference");
    const std::string difference1_name = JoinPath(path, "d1.fst");
    const std::string difference2_name = JoinPath(path, "d2.fst");
    const std::string difference3_name = JoinPath(path, "d3.fst");
    const std::string difference4_name = JoinPath(path, "d4.fst");
    const std::string difference5_name = JoinPath(path, "d5.fst");

    ifst1_.reset(VectorFst<Arc>::Read(difference1_name));
    ifst2_.reset(VectorFst<Arc>::Read(difference2_name));
    // ifst3_ = DifferenceFst(ifst1_, ifst2_)
    ifst3_.reset(VectorFst<Arc>::Read(difference3_name));
    ifst4_.reset(VectorFst<Arc>::Read(difference4_name));
    ifst5_.reset(VectorFst<Arc>::Read(difference5_name));
  }

  std::unique_ptr<const VectorFst<Arc>> ifst1_;
  std::unique_ptr<const VectorFst<Arc>> ifst2_;
  std::unique_ptr<const VectorFst<Arc>> ifst3_;
  std::unique_ptr<const VectorFst<Arc>> ifst4_;
  std::unique_ptr<const VectorFst<Arc>> ifst5_;
};

TEST_F(DifferenceTest, DifferenceFst) {
  VectorFst<Arc> nfst;

  DifferenceFst<Arc> dfst1(*ifst1_, *ifst2_);
  ASSERT_TRUE(Verify(dfst1));
  ASSERT_TRUE(Equal(*ifst3_, dfst1));

  DifferenceFst<Arc> ndfst1(*ifst1_, nfst);
  ASSERT_TRUE(Verify(ndfst1));
  ASSERT_TRUE(Equal(*ifst1_, ndfst1));

  DifferenceFst<Arc> ndfst2(nfst, nfst);
  ASSERT_TRUE(Verify(ndfst2));
  ASSERT_TRUE(Equal(nfst, ndfst2));

  for (const bool safe : {false, true}) {
    DifferenceFst<Arc> cfst(dfst1, safe);
    ASSERT_TRUE(Verify(cfst));
    ASSERT_TRUE(Equal(*ifst3_, cfst));
  }
}

TEST_F(DifferenceTest, FstClassDifference) {
  namespace s = fst::script;

  s::FstClass ifst1(*ifst1_);
  s::FstClass ifst2(*ifst2_);
  s::FstClass ifst3(*ifst3_);
  s::FstClass ifst4(*ifst4_);
  s::FstClass ifst5(*ifst5_);

  s::VectorFstClass ofst3(ifst1.ArcType());
  const ComposeOptions opts(false);
  Difference(ifst1, ifst2, &ofst3, opts);
  ASSERT_TRUE(Equal(ofst3, ifst3));

  s::VectorFstClass ofst5(ifst4.ArcType());
  Difference(ifst4, ifst2, &ofst5);
  ASSERT_TRUE(Equal(ofst5, ifst5));
}

}  // namespace
}  // namespace fst
