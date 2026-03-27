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
// Unit test for Reweight.

#include "openfst/lib/reweight.h"

#include <memory>
#include <string>
#include <vector>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/equal.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/lib/verify.h"

namespace fst {
namespace {

using Arc = StdArc;
using Weight = Arc::Weight;

class ReweightTest : public testing::Test {
 protected:
  void SetUp() override {
    const std::string path =
        JoinPath(std::string("."),
                       "openfst/test/testdata/reweight");
    const std::string reweight1_name = JoinPath(path, "r1.fst");
    const std::string reweight2_name = JoinPath(path, "r2.fst");
    const std::string reweight3_name = JoinPath(path, "r3.fst");

    rfst1_.reset(VectorFst<Arc>::Read(reweight1_name));
    rfst2_.reset(VectorFst<Arc>::Read(reweight2_name));
    rfst3_.reset(VectorFst<Arc>::Read(reweight3_name));
  }

  std::unique_ptr<const VectorFst<Arc>> rfst1_;
  std::unique_ptr<const VectorFst<Arc>> rfst2_;
  std::unique_ptr<const VectorFst<Arc>> rfst3_;
};

TEST_F(ReweightTest, Reweight) {
  VectorFst<Arc> nfst1;
  VectorFst<Arc> nfst2;

  VectorFst<Arc> vfst1(*rfst1_);
  VectorFst<Arc> vfst2(*rfst1_);

  std::vector<Weight> potential;
  potential.push_back(Weight(2.0));
  potential.push_back(Weight(3.0));
  potential.push_back(Weight(-1.0));

  Reweight(&vfst1, potential, REWEIGHT_TO_INITIAL);
  ASSERT_TRUE(Verify(vfst1));
  ASSERT_TRUE(Equal(*rfst2_, vfst1));

  Reweight(&vfst2, potential, REWEIGHT_TO_FINAL);
  ASSERT_TRUE(Verify(vfst2));
  ASSERT_TRUE(Equal(*rfst3_, vfst2));

  Reweight(&nfst1, potential, REWEIGHT_TO_INITIAL);
  ASSERT_TRUE(Verify(nfst1));

  Reweight(&nfst2, potential, REWEIGHT_TO_FINAL);
  ASSERT_TRUE(Verify(nfst2));
}

}  // namespace
}  // namespace fst
