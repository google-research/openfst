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
// Unit test for RmEpsilon.

#include "openfst/lib/rmepsilon.h"

#include <memory>
#include <string>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/equal.h"
#include "openfst/lib/properties.h"
#include "openfst/lib/queue.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/lib/verify.h"
#include "openfst/script/equal.h"
#include "openfst/script/fst-class.h"
#include "openfst/script/rmepsilon.h"
#include "openfst/script/verify.h"
#include "openfst/script/weight-class.h"

namespace fst {
namespace {

using Arc = StdArc;
using StateId = Arc::StateId;
using Weight = Arc::Weight;
using Label = Arc::Label;

class RmEpsilonTest : public testing::Test {
 protected:
  void SetUp() override {
    const std::string path =
        JoinPath(std::string("."),
                       "openfst/test/testdata/rmepsilon");
    const std::string rmeps1_name = JoinPath(path, "r1.fst");
    const std::string rmeps2_name = JoinPath(path, "r2.fst");
    const std::string rmeps3_name = JoinPath(path, "r3.fst");
    const std::string rmeps4_name = JoinPath(path, "r4.fst");

    rfst1_.reset(VectorFst<Arc>::Read(rmeps1_name));
    rfst2_.reset(VectorFst<Arc>::Read(rmeps2_name));
    rfst3_.reset(VectorFst<Arc>::Read(rmeps3_name));
    rfst4_.reset(VectorFst<Arc>::Read(rmeps4_name));
  }

  std::unique_ptr<const VectorFst<Arc>> rfst1_;
  std::unique_ptr<const VectorFst<Arc>> rfst2_;
  std::unique_ptr<const VectorFst<Arc>> rfst3_;
  std::unique_ptr<const VectorFst<Arc>> rfst4_;
};

TEST_F(RmEpsilonTest, RmEpsilon) {
  VectorFst<Arc> nfst1;
  VectorFst<Arc> nfst2;

  VectorFst<Arc> vfst(*rfst1_);

  RmEpsilon(&vfst, false);
  ASSERT_TRUE(Verify(vfst));
  ASSERT_TRUE(vfst.Properties(kNoEpsilons, true));
  ASSERT_TRUE(Equal(*rfst2_, vfst));

  RmEpsilon(&nfst1, false);
  ASSERT_TRUE(Verify(nfst1));
  ASSERT_TRUE(Equal(nfst1, nfst2));
}

TEST_F(RmEpsilonTest, RmEpsilonFstClass) {
  namespace s = fst::script;

  s::VectorFstClass rfst1(*rfst1_);
  s::VectorFstClass rfst2(*rfst2_);

  s::VectorFstClass vfst(rfst1);

  const s::WeightClass threshold = s::WeightClass::Zero(vfst.WeightType());
  const s::RmEpsilonOptions opts(AUTO_QUEUE, false, threshold);

  RmEpsilon(&vfst, opts);
  ASSERT_TRUE(Verify(vfst));
  ASSERT_TRUE(vfst.Properties(kNoEpsilons, true));
  ASSERT_TRUE(Equal(rfst2, vfst));
}

TEST_F(RmEpsilonTest, RmEpsilonFst) {
  VectorFst<Arc> nfst;

  RmEpsilonFst<Arc> dfst(*rfst1_);
  ASSERT_TRUE(Verify(dfst));
  ASSERT_TRUE(dfst.Properties(kNoEpsilons, true));
  ASSERT_TRUE(Equal(*rfst3_, dfst));

  RmEpsilonFst<Arc> ndfst(nfst);
  ASSERT_TRUE(Verify(ndfst));
  ASSERT_TRUE(Equal(nfst, ndfst));

  for (const bool safe : {false, true}) {
    RmEpsilonFst<Arc> cfst(dfst, safe);
    ASSERT_TRUE(Verify(cfst));
    ASSERT_TRUE(cfst.Properties(kNoEpsilons, true));
    ASSERT_TRUE(Equal(*rfst3_, cfst));
  }
}

TEST_F(RmEpsilonTest, PrunedRmEpsilon) {
  VectorFst<Arc> nfst1;
  VectorFst<Arc> nfst2;

  VectorFst<Arc> vfst(*rfst1_);

  RmEpsilon(&vfst, false, Weight(1.0), 10);
  ASSERT_TRUE(Verify(vfst));
  ASSERT_TRUE(vfst.Properties(kNoEpsilons, true));
  ASSERT_TRUE(Equal(*rfst4_, vfst));

  RmEpsilon(&nfst1, false, Weight(1.0), 10);
  ASSERT_TRUE(Verify(nfst1));
  ASSERT_TRUE(Equal(nfst1, nfst2));
}

}  // namespace
}  // namespace fst
