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
// Unit test for Union.

#include "openfst/lib/union.h"

#include <memory>
#include <string>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/equal.h"
#include "openfst/lib/rational.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/lib/verify.h"
#include "openfst/script/equal.h"
#include "openfst/script/fst-class.h"
#include "openfst/script/union.h"

namespace fst {
namespace {

using Arc = StdArc;

class UnionTest : public testing::Test {
 protected:
  void SetUp() override {
    const std::string path = JoinPath(
        std::string("."), "openfst/test/testdata/union");
    const std::string union1_name = JoinPath(path, "u1.fst");
    const std::string union2_name = JoinPath(path, "u2.fst");
    const std::string union3_name = JoinPath(path, "u3.fst");
    const std::string union4_name = JoinPath(path, "u4.fst");
    const std::string union5_name = JoinPath(path, "u5.fst");
    const std::string union6_name = JoinPath(path, "u6.fst");

    ufst1_.reset(VectorFst<Arc>::Read(union1_name));
    ufst2_.reset(VectorFst<Arc>::Read(union2_name));
    // ufst3_ = Union(ufst1_, ufst2_)
    ufst3_.reset(VectorFst<Arc>::Read(union3_name));
    //  ufst4_ = UnionFst(ufst1_, ufst2_)
    ufst4_.reset(VectorFst<Arc>::Read(union4_name));
    // ufst5_ = UnionFst(ufst1_, nullfst)
    ufst5_.reset(VectorFst<Arc>::Read(union5_name));
    // ufst6_ = Union(&ufst4_, ufst1_)
    ufst6_.reset(VectorFst<Arc>::Read(union6_name));
  }

  std::unique_ptr<const VectorFst<Arc>> ufst1_;
  std::unique_ptr<const VectorFst<Arc>> ufst2_;
  std::unique_ptr<const VectorFst<Arc>> ufst3_;
  std::unique_ptr<const VectorFst<Arc>> ufst4_;
  std::unique_ptr<const VectorFst<Arc>> ufst5_;
  std::unique_ptr<const VectorFst<Arc>> ufst6_;
};

TEST_F(UnionTest, MutableUnion) {
  {
    VectorFst<Arc> vfst1(*ufst1_);
    Union(&vfst1, *ufst2_);
    ASSERT_TRUE(Verify(vfst1));
    ASSERT_TRUE(Equal(*ufst3_, vfst1));
  }

  // The same, but with a vector of left-hand arguments.
  {
    VectorFst<Arc> vfst1(*ufst1_);
    Union(&vfst1, {ufst2_.get()});
    ASSERT_TRUE(Verify(vfst1));
    ASSERT_TRUE(Equal(*ufst3_, vfst1));
  }

  // Tests with an empty arg.

  {
    VectorFst<Arc> nfst;
    Union(&nfst, *ufst1_);
    ASSERT_TRUE(Verify(nfst));
    ASSERT_TRUE(Equal(*ufst1_, nfst));
  }

  {
    VectorFst<Arc> nfst;
    VectorFst<Arc> vfst2(*ufst2_);
    Union(&vfst2, nfst);
    ASSERT_TRUE(Verify(vfst2));
    ASSERT_TRUE(Equal(*ufst2_, vfst2));
  }
}

TEST_F(UnionTest, UnionFst) {
  VectorFst<Arc> nfst;

  UnionFst<Arc> dfst1(*ufst1_, *ufst2_);
  ASSERT_TRUE(Verify(dfst1));
  ASSERT_TRUE(Equal(*ufst4_, dfst1));

  UnionFst<Arc> dfst2(*ufst1_, nfst);
  ASSERT_TRUE(Verify(dfst2));
  ASSERT_TRUE(Equal(*ufst5_, dfst2));

  UnionFst<Arc> dfst3(nfst, nfst);
  ASSERT_TRUE(Verify(dfst3));

  for (const bool safe : {false, true}) {
    UnionFst<Arc> cfst(dfst1, safe);
    ASSERT_TRUE(Verify(cfst));
    ASSERT_TRUE(Equal(*ufst4_, cfst));
  }
}

TEST_F(UnionTest, RationalUnion) {
  UnionFst<Arc> dfst1(*ufst1_, *ufst2_);
  Union(&dfst1, *ufst1_);
  ASSERT_TRUE(Verify(dfst1));
  ASSERT_TRUE(Equal(*ufst6_, dfst1));
}

TEST_F(UnionTest, FstClassUnion) {
  using script::FstClass;
  using script::MutableFstClass;
  VectorFst<Arc> empty;
  MutableFstClass nfst(empty);

  MutableFstClass vfst1(*ufst1_);
  MutableFstClass vfst2(*ufst1_);
  MutableFstClass nfst1(nfst);
  FstClass ufst1(*ufst1_);
  FstClass ufst2(*ufst2_);
  FstClass ufst3(*ufst3_);

  Union(&vfst1, ufst2);
  ASSERT_TRUE(Equal(ufst3, vfst1));

  Union(&nfst1, ufst1);
  ASSERT_TRUE(Equal(ufst1, nfst1));

  Union(&vfst2, nfst);
  ASSERT_TRUE(Equal(ufst1, vfst2));
}

}  // namespace
}  // namespace fst
