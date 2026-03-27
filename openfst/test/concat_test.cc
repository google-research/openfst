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
// Unit test for Concat.

#include "openfst/lib/concat.h"

#include <memory>
#include <string>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/equal.h"
#include "openfst/lib/rational.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/lib/verify.h"
#include "openfst/script/concat.h"
#include "openfst/script/equal.h"
#include "openfst/script/fst-class.h"

namespace fst {
namespace {

using Arc = StdArc;

class ConcatTest : public ::testing::Test {
 protected:
  void SetUp() override {
    const std::string path =
        JoinPath(std::string("."),
                       "openfst/test/testdata/concat");
    const std::string concat1_name = JoinPath(path, "c1.fst");
    const std::string concat2_name = JoinPath(path, "c2.fst");
    const std::string concat3_name = JoinPath(path, "c3.fst");
    const std::string concat4_name = JoinPath(path, "c4.fst");
    const std::string concat5_name = JoinPath(path, "c5.fst");
    const std::string concat6_name = JoinPath(path, "c6.fst");
    const std::string concat7_name = JoinPath(path, "c7.fst");
    const std::string concat8_name = JoinPath(path, "c8.fst");

    cfst1_.reset(VectorFst<Arc>::Read(concat1_name));
    cfst2_.reset(VectorFst<Arc>::Read(concat2_name));
    // cfst3_ = Concat(cfst1_, cfst2_)
    cfst3_.reset(VectorFst<Arc>::Read(concat3_name));
    //  cfst4_ = ConcatFst(cfst1_, cfst2_)
    cfst4_.reset(VectorFst<Arc>::Read(concat4_name));
    // cfst5_ = ConcatFst(cfst1_, nullfst)
    cfst5_.reset(VectorFst<Arc>::Read(concat5_name));
    // cfst6_ = Concat(&cfst3_, cfst1_);
    cfst6_.reset(VectorFst<Arc>::Read(concat6_name));
    // cfst7_ = Concat(cfst1_, &cfst2_)
    cfst7_.reset(VectorFst<Arc>::Read(concat7_name));
    // cfst8_ = Concat(cfst1_, &Concat(cfst2_, cfst1_))
    cfst8_.reset(VectorFst<Arc>::Read(concat8_name));
  }

  std::unique_ptr<const VectorFst<Arc>> cfst1_;
  std::unique_ptr<const VectorFst<Arc>> cfst2_;
  std::unique_ptr<const VectorFst<Arc>> cfst3_;
  std::unique_ptr<const VectorFst<Arc>> cfst4_;
  std::unique_ptr<const VectorFst<Arc>> cfst5_;
  std::unique_ptr<const VectorFst<Arc>> cfst6_;
  std::unique_ptr<const VectorFst<Arc>> cfst7_;
  std::unique_ptr<const VectorFst<Arc>> cfst8_;
};

TEST_F(ConcatTest, MutableConcat) {
  {
    VectorFst<Arc> vfst1(*cfst1_);
    Concat(&vfst1, *cfst2_);
    ASSERT_TRUE(Verify(vfst1));
    ASSERT_TRUE(Equal(*cfst3_, vfst1));
  }

  {
    VectorFst<Arc> vfst2(*cfst2_);
    Concat(*cfst1_, &vfst2);
    ASSERT_TRUE(Verify(vfst2));
    ASSERT_TRUE(Equal(*cfst7_, vfst2));
  }

  // The same, but with a vector of left-hand arguments.
  {
    VectorFst<Arc> vfst2(*cfst2_);
    Concat({cfst1_.get()}, &vfst2);
    ASSERT_TRUE(Verify(vfst2));
    ASSERT_TRUE(Equal(*cfst7_, vfst2));
  }

  // Test with an empty arg.
  {
    VectorFst<Arc> nfst;
    Concat(&nfst, *cfst2_);
    ASSERT_TRUE(Verify(nfst));
    ASSERT_TRUE(Equal(nfst, VectorFst<Arc>()));
  }
}

TEST_F(ConcatTest, FstClassConcat) {
  namespace s = fst::script;

  s::VectorFstClass cfst1(*cfst1_);
  s::VectorFstClass cfst2(*cfst2_);
  s::VectorFstClass cfst3(*cfst3_);
  s::VectorFstClass vfst1(*cfst1_);
  s::VectorFstClass vfst2(*cfst2_);
  s::VectorFstClass cfst7(*cfst7_);
  s::VectorFstClass nfst(Arc::Type());
  s::VectorFstClass nfst1(Arc::Type());

  // Modifies first argument.
  Concat(&vfst1, cfst2);
  ASSERT_TRUE(Equal(cfst3, vfst1));

  // Modifies second argument.
  Concat(cfst1, &vfst2);
  ASSERT_TRUE(Equal(cfst7, vfst2));

  // Concats and modifies an empty FST.
  Concat(&nfst1, cfst2);
  ASSERT_TRUE(Equal(nfst1, nfst));
}

TEST_F(ConcatTest, ConcatFst) {
  VectorFst<Arc> nfst;

  ConcatFst<Arc> dfst1(*cfst1_, *cfst2_);
  ASSERT_TRUE(Verify(dfst1));
  ASSERT_TRUE(Equal(*cfst4_, dfst1));

  ConcatFst<Arc> dfst2(*cfst1_, nfst);
  ASSERT_TRUE(Verify(dfst2));
  ASSERT_TRUE(Equal(*cfst5_, dfst2));

  ConcatFst<Arc> dfst3(nfst, nfst);
  ASSERT_TRUE(Verify(dfst3));

  for (const bool safe : {false, true}) {
    ConcatFst<Arc> cfst(dfst1, safe);
    ASSERT_TRUE(Verify(cfst));
    ASSERT_TRUE(Equal(*cfst4_, cfst));
  }
}

TEST_F(ConcatTest, RationalConcat) {
  ConcatFst<Arc> dfst3(*cfst1_, *cfst2_);
  Concat(&dfst3, *cfst1_);
  ASSERT_TRUE(Verify(dfst3));
  ASSERT_TRUE(Equal(*cfst6_, dfst3));

  ConcatFst<Arc> dfst4(*cfst2_, *cfst1_);
  Concat(*cfst1_, &dfst4);
  ASSERT_TRUE(Verify(dfst4));
  ASSERT_TRUE(Equal(*cfst8_, dfst4));
}

}  // namespace
}  // namespace fst
