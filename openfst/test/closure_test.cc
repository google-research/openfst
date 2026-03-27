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
// Unit test for Closure.

#include "openfst/lib/closure.h"

#include <memory>
#include <string>
#include <utility>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/equal.h"
#include "openfst/lib/rational.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/lib/verify.h"
#include "openfst/script/closure.h"
#include "openfst/script/equal.h"
#include "openfst/script/fst-class.h"

namespace fst {
namespace {

using Arc = StdArc;
using StateId = Arc::StateId;
using Weight = Arc::Weight;
using Label = Arc::Label;

class ClosureTest : public testing::Test {
 protected:
  void SetUp() override {
    const std::string path =
        JoinPath(std::string("."),
                       "openfst/test/testdata/closure");
    const std::string closure1_name = JoinPath(path, "c1.fst");
    const std::string closure2_name = JoinPath(path, "c2.fst");
    const std::string closure3_name = JoinPath(path, "c3.fst");
    const std::string closure4_name = JoinPath(path, "c4.fst");
    const std::string closure5_name = JoinPath(path, "c5.fst");
    const std::string closure6_name = JoinPath(path, "c6.fst");
    const std::string closure7_name = JoinPath(path, "c7.fst");

    cfst1_.reset(VectorFst<Arc>::Read(closure1_name));
    // cfst2_ = Closure_star(cfst1_)
    cfst2_.reset(VectorFst<Arc>::Read(closure2_name));
    // cfst3_ = Closure_plus(cfst1_)
    cfst3_.reset(VectorFst<Arc>::Read(closure3_name));
    //  cfst4_ = ClosureFst_star(cfst1_)
    cfst4_.reset(VectorFst<Arc>::Read(closure4_name));
    // cfst5_ = ClosureVectorFst_plus(cfst1_)
    cfst5_.reset(VectorFst<Arc>::Read(closure5_name));
    // cfst6_ = ClosureVectorFst(&cfst4_)
    cfst6_.reset(VectorFst<Arc>::Read(closure6_name));
    // cfst7_ = ClosureVectorFst(&cfst5_)
    cfst7_.reset(VectorFst<Arc>::Read(closure7_name));
  }

  std::unique_ptr<const VectorFst<Arc>> cfst1_;
  std::unique_ptr<const VectorFst<Arc>> cfst2_;
  std::unique_ptr<const VectorFst<Arc>> cfst3_;
  std::unique_ptr<const VectorFst<Arc>> cfst4_;
  std::unique_ptr<const VectorFst<Arc>> cfst5_;
  std::unique_ptr<const VectorFst<Arc>> cfst6_;
  std::unique_ptr<const VectorFst<Arc>> cfst7_;
};

TEST_F(ClosureTest, MutableClosure) {
  VectorFst<Arc> nfst;

  VectorFst<Arc> efst;
  efst.AddState();
  efst.SetStart(0);
  efst.SetFinal(0, Weight::One());

  VectorFst<Arc> vfst1(*cfst1_);
  VectorFst<Arc> vfst2(*cfst1_);
  VectorFst<Arc> nfst1(nfst);
  VectorFst<Arc> nfst2(nfst);

  Closure(&vfst1, CLOSURE_STAR);
  ASSERT_TRUE(Verify(vfst1));
  ASSERT_TRUE(Equal(*cfst2_, vfst1));

  Closure(&nfst1, CLOSURE_STAR);
  ASSERT_TRUE(Verify(nfst1));
  ASSERT_TRUE(Equal(nfst1, efst));

  Closure(&vfst2, CLOSURE_PLUS);
  ASSERT_TRUE(Verify(vfst2));
  ASSERT_TRUE(Equal(*cfst3_, vfst2));

  Closure(&nfst2, CLOSURE_PLUS);
  ASSERT_TRUE(Verify(nfst2));
  ASSERT_TRUE(Equal(nfst2, nfst));
}

TEST_F(ClosureTest, FstClassClosure) {
  namespace s = fst::script;

  auto nfst_u = std::make_unique<VectorFst<Arc>>();
  s::VectorFstClass nfst(std::move(nfst_u));

  auto efst_u = std::make_unique<VectorFst<Arc>>();
  efst_u->AddState();
  efst_u->SetStart(0);
  efst_u->SetFinal(0, Weight::One());
  s::VectorFstClass efst(std::move(efst_u));

  s::MutableFstClass vfst1(*cfst1_);
  s::MutableFstClass vfst2(*cfst1_);
  s::MutableFstClass nfst1(nfst);
  s::MutableFstClass nfst2(nfst);
  s::MutableFstClass cfst2(*cfst2_);
  s::MutableFstClass cfst3(*cfst3_);

  Closure(&vfst1, CLOSURE_STAR);
  ASSERT_TRUE(Equal(cfst2, vfst1));

  Closure(&nfst1, CLOSURE_STAR);
  ASSERT_TRUE(Equal(nfst1, efst));

  Closure(&vfst2, CLOSURE_PLUS);
  ASSERT_TRUE(Equal(cfst3, vfst2));

  Closure(&nfst2, CLOSURE_PLUS);
  ASSERT_TRUE(Equal(nfst2, nfst));
}

TEST_F(ClosureTest, ClosureFst) {
  VectorFst<Arc> nfst;

  ClosureFst<Arc> dfst1(*cfst1_, CLOSURE_STAR);
  ASSERT_TRUE(Verify(dfst1));
  ASSERT_TRUE(Equal(*cfst4_, dfst1));

  ClosureFst<Arc> ndfst1(nfst, CLOSURE_STAR);
  ASSERT_TRUE(Verify(ndfst1));

  ClosureFst<Arc> dfst2(*cfst1_, CLOSURE_PLUS);
  ASSERT_TRUE(Verify(dfst2));
  ASSERT_TRUE(Equal(*cfst5_, dfst2));

  ClosureFst<Arc> ndfst2(nfst, CLOSURE_PLUS);
  ASSERT_TRUE(Verify(ndfst2));

  for (const bool safe : {false, true}) {
    ClosureFst<Arc> cfst(dfst1, safe);
    ASSERT_TRUE(Verify(cfst));
    ASSERT_TRUE(Equal(*cfst4_, cfst));
  }
}

TEST_F(ClosureTest, RationalClosure) {
  ClosureFst<Arc> dfst1(*cfst1_, CLOSURE_STAR);
  Closure(&dfst1, CLOSURE_STAR);
  ASSERT_TRUE(Verify(dfst1));
  ASSERT_TRUE(Equal(*cfst6_, dfst1));

  ClosureFst<Arc> dfst2(*cfst1_, CLOSURE_PLUS);
  Closure(&dfst2, CLOSURE_PLUS);
  ASSERT_TRUE(Verify(dfst2));
  ASSERT_TRUE(Equal(*cfst7_, dfst2));
}

}  // namespace
}  // namespace fst
