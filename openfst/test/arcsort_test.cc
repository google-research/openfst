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
// Unit test for ArcSort.

#include "openfst/lib/arcsort.h"

#include <memory>
#include <string>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/equal.h"
#include "openfst/lib/properties.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/lib/verify.h"
#include "openfst/script/arcsort.h"
#include "openfst/script/equal.h"
#include "openfst/script/fst-class.h"

namespace fst {
namespace {

using Arc = StdArc;

class ArcSortTest : public testing::Test {
 protected:
  void SetUp() override {
    const std::string path =
        JoinPath(std::string("."),
                       "openfst/test/testdata/arcsort");
    const std::string arcsort1_name = JoinPath(path, "a1.fst");
    const std::string arcsort2_name = JoinPath(path, "a2.fst");

    // Sorted on the input side.
    afst1_.reset(VectorFst<Arc>::Read(arcsort1_name));
    // Same FST as above but sorted on the output side.
    afst2_.reset(VectorFst<Arc>::Read(arcsort2_name));
  }

  std::unique_ptr<const VectorFst<Arc>> afst1_;
  std::unique_ptr<const VectorFst<Arc>> afst2_;
};

TEST_F(ArcSortTest, ILabelArcSort) {
  VectorFst<Arc> nfst;

  VectorFst<Arc> vfst(*afst2_);

  ILabelCompare<Arc> icomp;

  ASSERT_TRUE(vfst.Properties(kNotILabelSorted, true));
  ArcSort(&vfst, icomp);
  ASSERT_TRUE(Verify(vfst));
  ASSERT_TRUE(vfst.Properties(kILabelSorted, true));
  ASSERT_TRUE(Equal(*afst1_, vfst));

  ASSERT_TRUE(nfst.Properties(kILabelSorted, true));
  ArcSort(&nfst, icomp);
  ASSERT_TRUE(Verify(nfst));
  ASSERT_TRUE(nfst.Properties(kILabelSorted, true));
}

TEST_F(ArcSortTest, OLabelArcSort) {
  VectorFst<Arc> nfst;

  VectorFst<Arc> vfst(*afst1_);

  OLabelCompare<Arc> ocomp;
  ASSERT_TRUE(vfst.Properties(kNotOLabelSorted, true));
  ArcSort(&vfst, ocomp);
  ASSERT_TRUE(Verify(vfst));
  ASSERT_TRUE(vfst.Properties(kOLabelSorted, true));
  ASSERT_TRUE(Equal(*afst2_, vfst));

  ASSERT_TRUE(nfst.Properties(kOLabelSorted, true));
  ArcSort(&nfst, ocomp);
  ASSERT_TRUE(Verify(nfst));
  ASSERT_TRUE(nfst.Properties(kOLabelSorted, true));
}

TEST_F(ArcSortTest, FstClassArcSort) {
  // Check that it works with OLabel comparisons, and trust the previous
  // tests for other cases (since there's no reason ilabel and olabel, for
  // instance, should be different)
  namespace s = fst::script;

  s::VectorFstClass vfst(*afst1_);
  s::FstClass afst2(*afst2_);
  s::VectorFstClass nfst(vfst.ArcType());

  ASSERT_TRUE(vfst.Properties(kNotOLabelSorted, true));
  ArcSort(&vfst, s::ArcSortType::OLABEL);
  ASSERT_TRUE(vfst.Properties(kOLabelSorted, true));
  ASSERT_TRUE(Equal(afst2, vfst));

  ASSERT_TRUE(nfst.Properties(kOLabelSorted, true));
  ArcSort(&nfst, s::ArcSortType::OLABEL);
  ASSERT_TRUE(nfst.Properties(kOLabelSorted, true));
}

TEST_F(ArcSortTest, ILabelArcSortFst) {
  VectorFst<Arc> nfst;

  ILabelCompare<Arc> icomp;

  ArcSortFst<Arc, ILabelCompare<Arc>> dfst1(*afst2_, icomp);
  ASSERT_TRUE(Verify(dfst1));
  ASSERT_TRUE(dfst1.Properties(kILabelSorted, true));
  ASSERT_TRUE(Equal(*afst1_, dfst1));

  ArcSortFst<Arc, ILabelCompare<Arc>> ndfst1(nfst, icomp);
  ASSERT_TRUE(Verify(ndfst1));
  ASSERT_TRUE(ndfst1.Properties(kILabelSorted, true));
  ASSERT_TRUE(Equal(nfst, ndfst1));

  for (const bool safe : {false, true}) {
    ArcSortFst<Arc, ILabelCompare<Arc>> cfst(dfst1, safe);
    ASSERT_TRUE(Verify(cfst));
    ASSERT_TRUE(cfst.Properties(kILabelSorted, true));
    ASSERT_TRUE(Equal(*afst1_, cfst));
  }
}

TEST_F(ArcSortTest, OLabelArcSortFst) {
  VectorFst<Arc> nfst;

  OLabelCompare<Arc> ocomp;

  ArcSortFst<Arc, OLabelCompare<Arc>> dfst2(*afst1_, ocomp);
  ASSERT_TRUE(Verify(dfst2));
  ASSERT_TRUE(dfst2.Properties(kOLabelSorted, true));
  ASSERT_TRUE(Equal(*afst2_, dfst2));

  ArcSortFst<Arc, OLabelCompare<Arc>> ndfst2(nfst, ocomp);
  ASSERT_TRUE(Verify(ndfst2));
  ASSERT_TRUE(ndfst2.Properties(kOLabelSorted, true));
  ASSERT_TRUE(Equal(nfst, ndfst2));
}

}  // namespace
}  // namespace fst
