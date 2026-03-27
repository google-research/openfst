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
// Unit test for Push.

#include "openfst/lib/push.h"

#include <memory>
#include <string>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/equal.h"
#include "openfst/lib/reweight.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/lib/verify.h"
#include "openfst/script/equal.h"
#include "openfst/script/fst-class.h"
#include "openfst/script/push.h"

namespace fst {
namespace {

using Arc = StdArc;

class PushTest : public testing::Test {
 protected:
  void SetUp() override {
    const std::string path = JoinPath(
        std::string("."), "openfst/test/testdata/push");
    const std::string push1_name = JoinPath(path, "p1.fst");
    const std::string push2_name = JoinPath(path, "p2.fst");
    const std::string push3_name = JoinPath(path, "p3.fst");
    const std::string push4_name = JoinPath(path, "p4.fst");
    const std::string push5_name = JoinPath(path, "p5.fst");

    pfst1_.reset(VectorFst<Arc>::Read(push1_name));
    pfst2_.reset(VectorFst<Arc>::Read(push2_name));
    pfst3_.reset(VectorFst<Arc>::Read(push3_name));
    pfst4_.reset(VectorFst<Arc>::Read(push4_name));
    pfst5_.reset(VectorFst<Arc>::Read(push5_name));
  }

  std::unique_ptr<const VectorFst<Arc>> pfst1_;
  std::unique_ptr<const VectorFst<Arc>> pfst2_;
  std::unique_ptr<const VectorFst<Arc>> pfst3_;
  std::unique_ptr<const VectorFst<Arc>> pfst4_;
  std::unique_ptr<const VectorFst<Arc>> pfst5_;
};

TEST_F(PushTest, PushWeight) {
  using fst::script::Push;
  using fst::script::VectorFstClass;

  VectorFstClass pfst2(*pfst2_);
  VectorFstClass pfst3(*pfst3_);

  VectorFstClass vfst1(*pfst1_);
  VectorFstClass vfst2(*pfst1_);

  Push(&vfst1, REWEIGHT_TO_INITIAL);
  ASSERT_TRUE(Equal(pfst2, vfst1));

  Push(&vfst2, REWEIGHT_TO_FINAL);
  ASSERT_TRUE(Equal(pfst3, vfst2));
}

// FstClass test - testing script-level operations
TEST_F(PushTest, PushWeightFstClass) {
  // Most of the Push features in script-land are tested in the bin-test.
  // This just makes sure that the other signature for push is working
  // correctly.

  VectorFst<Arc> nfst1;
  VectorFst<Arc> nfst2;

  VectorFst<Arc> vfst1(*pfst1_);
  VectorFst<Arc> vfst2(*pfst1_);

  Push(&vfst1, REWEIGHT_TO_INITIAL);
  ASSERT_TRUE(Verify(vfst1));
  ASSERT_TRUE(Equal(*pfst2_, vfst1));

  Push(&vfst2, REWEIGHT_TO_FINAL);
  ASSERT_TRUE(Verify(vfst2));
  ASSERT_TRUE(Equal(*pfst3_, vfst2));

  Push(&nfst1, REWEIGHT_TO_INITIAL);
  ASSERT_TRUE(Verify(nfst1));

  Push(&nfst2, REWEIGHT_TO_FINAL);
  ASSERT_TRUE(Verify(nfst2));
}

TEST_F(PushTest, PushLabel) {
  VectorFst<Arc> nfst;

  VectorFst<Arc> nfst1;
  VectorFst<Arc> nfst2;
  VectorFst<Arc> vfst1;
  VectorFst<Arc> vfst2;

  Push<Arc, REWEIGHT_TO_INITIAL>(*pfst1_, &vfst1, kPushLabels);
  ASSERT_TRUE(Verify(vfst1));
  ASSERT_TRUE(Equal(*pfst4_, vfst1));

  Push<Arc, REWEIGHT_TO_FINAL>(*pfst1_, &vfst2, kPushLabels);
  ASSERT_TRUE(Verify(vfst2));
  ASSERT_TRUE(Equal(*pfst5_, vfst2));

  Push<Arc, REWEIGHT_TO_INITIAL>(nfst, &nfst1, kPushLabels);
  Push(&nfst1, REWEIGHT_TO_INITIAL);
  ASSERT_TRUE(Verify(nfst1));

  Push<Arc, REWEIGHT_TO_FINAL>(nfst, &nfst2, kPushLabels);
  ASSERT_TRUE(Verify(nfst2));
}

}  // namespace
}  // namespace fst
