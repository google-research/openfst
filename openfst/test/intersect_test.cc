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
// Unit test for Intersect.

#include "openfst/lib/intersect.h"

#include <memory>
#include <string>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/compose.h"
#include "openfst/lib/equal.h"
#include "openfst/lib/properties.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/lib/verify.h"
#include "openfst/script/equal.h"
#include "openfst/script/fst-class.h"
#include "openfst/script/intersect.h"

namespace fst {
namespace {

using Arc = StdArc;

class IntersectTest : public testing::Test {
 protected:
  void SetUp() override {
    const std::string path =
        JoinPath(std::string("."),
                       "openfst/test/testdata/intersect");
    const std::string intersect1_name = JoinPath(path, "i1.fst");
    const std::string intersect2_name = JoinPath(path, "i2.fst");
    const std::string intersect3_name = JoinPath(path, "i3.fst");
    const std::string intersect4_name = JoinPath(path, "i4.fst");
    const std::string intersect5_name = JoinPath(path, "i5.fst");

    ifst1_.reset(VectorFst<Arc>::Read(intersect1_name));
    ifst2_.reset(VectorFst<Arc>::Read(intersect2_name));
    // ifst3_ = IntersectFst(ifst1_, ifst2_)
    ifst3_.reset(VectorFst<Arc>::Read(intersect3_name));
    ifst4_.reset(VectorFst<Arc>::Read(intersect4_name));
    ifst5_.reset(VectorFst<Arc>::Read(intersect5_name));
  }

  std::unique_ptr<const VectorFst<Arc>> ifst1_;
  std::unique_ptr<const VectorFst<Arc>> ifst2_;
  std::unique_ptr<const VectorFst<Arc>> ifst3_;
  std::unique_ptr<const VectorFst<Arc>> ifst4_;
  std::unique_ptr<const VectorFst<Arc>> ifst5_;
};

TEST_F(IntersectTest, Intersec) {
  VectorFst<Arc> nfst;
  VectorFst<Arc> vfst1(*ifst1_);
  VectorFst<Arc> vfst2(*ifst2_);

  // Clears sort properties.
  vfst1.SetProperties(0, kOLabelSorted);
  vfst2.SetProperties(0, kILabelSorted);

  IntersectFst<Arc> dfst1(*ifst1_, *ifst2_);
  ASSERT_TRUE(Verify(dfst1));
  VectorFst<Arc> ofst(dfst1);
  ASSERT_TRUE(Equal(*ifst3_, dfst1));

  IntersectFst<Arc> dfst2(vfst1, *ifst2_);
  ASSERT_TRUE(Verify(dfst2));
  ASSERT_TRUE(Equal(*ifst3_, dfst2));

  IntersectFst<Arc> dfst3(*ifst1_, vfst2);
  ASSERT_TRUE(Verify(dfst3));
  ASSERT_TRUE(Equal(*ifst3_, dfst3));

  IntersectFst<Arc> ndfst1(nfst, *ifst2_);
  ASSERT_TRUE(Verify(ndfst1));
  ASSERT_TRUE(Equal(nfst, ndfst1));

  IntersectFst<Arc> ndfst2(*ifst1_, nfst);
  ASSERT_TRUE(Verify(ndfst2));
  ASSERT_TRUE(Equal(nfst, ndfst2));

  for (const bool safe : {false, true}) {
    ComposeFst<Arc> cfst(dfst1, safe);
    ASSERT_TRUE(Verify(cfst));
    ASSERT_TRUE(Equal(*ifst3_, cfst));
  }
}

TEST_F(IntersectTest, FstClassIntersect) {
  namespace s = fst::script;

  s::FstClass ifst1(*ifst1_);
  s::FstClass ifst2(*ifst2_);
  s::FstClass ifst3(*ifst3_);
  s::FstClass ifst4(*ifst4_);
  s::FstClass ifst5(*ifst5_);

  s::VectorFstClass ofst3(ifst1.ArcType());
  Intersect(ifst1, ifst2, &ofst3);
  ASSERT_TRUE(Equal(ofst3, ifst3));

  s::VectorFstClass ofst5(ifst1.ArcType());
  const ComposeOptions opts(false);
  Intersect(ifst1, ifst4, &ofst5, opts);
  ASSERT_TRUE(Equal(ofst5, ifst5));
}

}  // namespace
}  // namespace fst
