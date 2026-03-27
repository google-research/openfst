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
// Unit test for Topsort.

#include "openfst/lib/topsort.h"

#include <memory>
#include <string>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/equal.h"
#include "openfst/lib/properties.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/lib/verify.h"

namespace fst {
namespace {

using Arc = StdArc;

class TopsortTest : public testing::Test {
 protected:
  void SetUp() override {
    const std::string path =
        JoinPath(std::string("."),
                       "openfst/test/testdata/topsort");
    const std::string topsort1_name = JoinPath(path, "t1.fst");
    const std::string topsort2_name = JoinPath(path, "t2.fst");
    const std::string topsort3_name = JoinPath(path, "t3.fst");

    // Acyclic and not top-sorted.
    tfst1_.reset(VectorFst<Arc>::Read(topsort1_name));
    // tfst2_ = TopSort(tfst1_).
    tfst2_.reset(VectorFst<Arc>::Read(topsort2_name));
    // Cyclic machine.
    tfst3_.reset(VectorFst<Arc>::Read(topsort3_name));
  }

  std::unique_ptr<const VectorFst<Arc>> tfst1_;
  std::unique_ptr<const VectorFst<Arc>> tfst2_;
  std::unique_ptr<const VectorFst<Arc>> tfst3_;
};

TEST_F(TopsortTest, Topsort) {
  VectorFst<Arc> nfst;

  VectorFst<Arc> vfst1(*tfst1_);
  VectorFst<Arc> vfst2(*tfst3_);

  bool is_acyclic = TopSort(&vfst1);
  ASSERT_TRUE(Verify(vfst1));
  ASSERT_TRUE(is_acyclic);
  ASSERT_TRUE(vfst1.Properties(kTopSorted, true));
  ASSERT_TRUE(vfst1.Properties(kAcyclic, true));
  ASSERT_TRUE(vfst1.Properties(kInitialAcyclic, true));
  ASSERT_TRUE(Equal(*tfst2_, vfst1));

  is_acyclic = TopSort(&vfst2);
  ASSERT_TRUE(Verify(vfst2));
  ASSERT_TRUE(vfst2.Properties(kNotTopSorted, true));
  ASSERT_TRUE(vfst2.Properties(kCyclic, true));
  ASSERT_FALSE(is_acyclic);

  is_acyclic = TopSort(&nfst);
  ASSERT_TRUE(Verify(nfst));
  ASSERT_TRUE(nfst.Properties(kTopSorted, true));
  ASSERT_TRUE(nfst.Properties(kAcyclic, true));
  ASSERT_TRUE(nfst.Properties(kInitialAcyclic, true));
  ASSERT_TRUE(nfst.Properties(kTopSorted, true));
  ASSERT_TRUE(nfst.Properties(kAcyclic, true));
  ASSERT_TRUE(nfst.Properties(kInitialAcyclic, true));
  ASSERT_TRUE(is_acyclic);
}

}  // namespace
}  // namespace fst
