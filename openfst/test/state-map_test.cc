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
// Unit test for StateMap.

#include "openfst/lib/state-map.h"

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

class StateMapTest : public testing::Test {
 protected:
  void SetUp() override {
    const std::string path =
        JoinPath(std::string("."),
                       "openfst/test/testdata/state-map");
    const std::string a1_name = JoinPath(path, "a1.fst");
    const std::string a2_name = JoinPath(path, "a2.fst");
    const std::string b1_name = JoinPath(path, "b1.fst");
    const std::string b2_name = JoinPath(path, "b2.fst");

    a1_.reset(VectorFst<Arc>::Read(a1_name));
    a2_.reset(VectorFst<Arc>::Read(a2_name));
    b1_.reset(VectorFst<Arc>::Read(b1_name));
    b2_.reset(VectorFst<Arc>::Read(b2_name));
  }

  std::unique_ptr<const VectorFst<Arc>> a1_;
  std::unique_ptr<const VectorFst<Arc>> a2_;
  std::unique_ptr<const VectorFst<Arc>> b1_;
  std::unique_ptr<const VectorFst<Arc>> b2_;
};

TEST_F(StateMapTest, IdentityTest) {
  IdentityStateMapper<Arc> mapper(*a1_);
  VectorFst<Arc> a1;
  StateMap(*a1_, &a1, mapper);
  EXPECT_TRUE(Verify(a1));
  EXPECT_TRUE(Equal(a1, *a1_));
}

TEST_F(StateMapTest, ArcSumTest) {
  ArcSumMapper<Arc> mapper(*a1_);

  // On-the-fly.
  {
    StateMapFst<Arc, Arc, ArcSumMapper<Arc>> a1fst(*a1_, mapper);
    EXPECT_TRUE(Verify(a1fst));
    EXPECT_TRUE(Equal(a1fst, *a2_));
  }

  // Constructive.
  {
    VectorFst<Arc> a1;
    StateMap(*a1_, &a1, mapper);
    EXPECT_TRUE(Verify(a1));
    EXPECT_TRUE(Equal(a1, *a2_));
  }

  // Destructive.
  {
    VectorFst<Arc> a1(*a1_);
    StateMap(&a1, mapper);
    EXPECT_TRUE(Verify(a1));
    EXPECT_TRUE(Equal(a1, *a2_));
  }
}

TEST_F(StateMapTest, ArcUniqueTest) {
  ArcSumMapper<Arc> mapper(*b1_);
  VectorFst<Arc> b1(*b1_);
  StateMap(&b1, mapper);
  EXPECT_TRUE(Verify(b1));
  EXPECT_TRUE(Equal(b1, *b2_));
}

}  // namespace
}  // namespace fst
