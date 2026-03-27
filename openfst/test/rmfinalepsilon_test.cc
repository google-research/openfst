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
// Unit test for RmFinalEpsilon.

#include "openfst/lib/rmfinalepsilon.h"

#include <memory>
#include <string>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/connect.h"
#include "openfst/lib/equal.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/lib/verify.h"

namespace fst {
namespace {

using Arc = StdArc;

class RmFinalEpsilonTest : public testing::Test {
 protected:
  void SetUp() override {
    const std::string path = JoinPath(
        std::string("."),
        "openfst/test/testdata/rmfinalepsilon");
    const std::string rmeps1_name = JoinPath(path, "r1.fst");
    const std::string rmeps2_name = JoinPath(path, "r2.fst");

    rfst1_.reset(VectorFst<Arc>::Read(rmeps1_name));
    rfst2_.reset(VectorFst<Arc>::Read(rmeps2_name));
  }

  std::unique_ptr<const VectorFst<Arc>> rfst1_;
  std::unique_ptr<const VectorFst<Arc>> rfst2_;
};

TEST_F(RmFinalEpsilonTest, RmEpsilon) {
  VectorFst<Arc> nfst;

  VectorFst<Arc> vfst(*rfst1_);

  RmFinalEpsilon(&vfst);
  Connect(&vfst);
  ASSERT_TRUE(Verify(vfst));
  ASSERT_TRUE(Equal(*rfst2_, vfst));

  RmFinalEpsilon(&nfst);
  ASSERT_TRUE(Verify(nfst));
}

}  // namespace
}  // namespace fst
