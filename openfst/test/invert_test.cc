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
// Unit test for Invert.

#include "openfst/lib/invert.h"

#include <memory>
#include <string>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/equal.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/lib/verify.h"

namespace fst {
namespace {

using Arc = StdArc;

class InvertTest : public testing::Test {
 protected:
  void SetUp() override {
    const std::string path =
        JoinPath(std::string("."),
                       "openfst/test/testdata/invert");
    const std::string invert1_name = JoinPath(path, "i1.fst");
    const std::string invert2_name = JoinPath(path, "i2.fst");

    ifst1_.reset(VectorFst<Arc>::Read(invert1_name));
    // ifst2_ = ifst1_^{-1}
    ifst2_.reset(VectorFst<Arc>::Read(invert2_name));
  }

  std::unique_ptr<const VectorFst<Arc>> ifst1_;
  std::unique_ptr<const VectorFst<Arc>> ifst2_;
};

TEST_F(InvertTest, Invert) {
  VectorFst<Arc> vfst;
  Invert(*ifst1_, &vfst);
  EXPECT_TRUE(Verify(vfst));
  EXPECT_TRUE(Equal(*ifst2_, vfst));
}

TEST_F(InvertTest, InvertFst) {
  VectorFst<Arc> nfst;

  InvertFst<Arc> dfst(*ifst1_);
  EXPECT_TRUE(Verify(dfst));
  EXPECT_TRUE(Equal(*ifst2_, dfst));

  InvertFst<Arc> ndfst(nfst);
  EXPECT_TRUE(Verify(ndfst));
  EXPECT_TRUE(Equal(nfst, ndfst));

  for (const bool safe : {false, true}) {
    InvertFst<Arc> cfst(dfst, safe);
    EXPECT_TRUE(Verify(cfst));
    EXPECT_TRUE(Equal(*ifst2_, cfst));
  }
}

}  // namespace
}  // namespace fst
