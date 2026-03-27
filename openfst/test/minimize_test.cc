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
// Unit test for Minimize.

#include "openfst/lib/minimize.h"

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

class MinimizeTest : public testing::Test {
 protected:
  void SetUp() override {
    const std::string path =
        JoinPath(std::string("."),
                       "openfst/test/testdata/minimize");
    const std::string m1_name = JoinPath(path, "m1.fst");
    const std::string acyclic_min_name =
        JoinPath(path, "acyclic_min.fst");
    const std::string m2_name = JoinPath(path, "m2.fst");
    const std::string cyclic_min_name = JoinPath(path, "cyclic_min.fst");
    const std::string m3_name = JoinPath(path, "m3.fst");
    const std::string weighted_acyclic_min_name =
        JoinPath(path, "weighted_acyclic_min.fst");
    const std::string m4_name = JoinPath(path, "m4.fst");
    const std::string weighted_cyclic_min_name =
        JoinPath(path, "weighted_cyclic_min.fst");
    const std::string m5_name = JoinPath(path, "m5.fst");
    const std::string transducer_acyclic_min_name =
        JoinPath(path, "transducer_acyclic_min.fst");
    const std::string transducer_acyclic_min1_name =
        JoinPath(path, "transducer_acyclic_min1.fst");
    const std::string transducer_acyclic_min2_name =
        JoinPath(path, "transducer_acyclic_min2.fst");
    const std::string m6_name = JoinPath(path, "m6.fst");
    const std::string transducer_cyclic_min_name =
        JoinPath(path, "transducer_cyclic_min.fst");

    m1_.reset(VectorFst<Arc>::Read(m1_name));
    acyclic_min_.reset(VectorFst<Arc>::Read(acyclic_min_name));
    m2_.reset(VectorFst<Arc>::Read(m2_name));
    cyclic_min_.reset(VectorFst<Arc>::Read(cyclic_min_name));
    m3_.reset(VectorFst<Arc>::Read(m3_name));
    weighted_acyclic_min_.reset(
        VectorFst<Arc>::Read(weighted_acyclic_min_name));
    m4_.reset(VectorFst<Arc>::Read(m4_name));
    weighted_cyclic_min_.reset(VectorFst<Arc>::Read(weighted_cyclic_min_name));
    m5_.reset(VectorFst<Arc>::Read(m5_name));
    transducer_acyclic_min_.reset(
        VectorFst<Arc>::Read(transducer_acyclic_min_name));
    transducer_acyclic_min1_.reset(
        VectorFst<Arc>::Read(transducer_acyclic_min1_name));
    transducer_acyclic_min2_.reset(
        VectorFst<Arc>::Read(transducer_acyclic_min2_name));
    m6_.reset(VectorFst<Arc>::Read(m6_name));
    transducer_cyclic_min_.reset(
        VectorFst<Arc>::Read(transducer_cyclic_min_name));
  }

  std::unique_ptr<VectorFst<Arc>> m1_;
  std::unique_ptr<VectorFst<Arc>> m2_;
  std::unique_ptr<VectorFst<Arc>> m3_;
  std::unique_ptr<VectorFst<Arc>> m4_;
  std::unique_ptr<const VectorFst<Arc>> m5_;
  std::unique_ptr<VectorFst<Arc>> m6_;
  std::unique_ptr<const VectorFst<Arc>> acyclic_min_;
  std::unique_ptr<const VectorFst<Arc>> cyclic_min_;
  std::unique_ptr<const VectorFst<Arc>> weighted_acyclic_min_;
  std::unique_ptr<const VectorFst<Arc>> weighted_cyclic_min_;
  std::unique_ptr<const VectorFst<Arc>> transducer_acyclic_min_;
  std::unique_ptr<const VectorFst<Arc>> transducer_acyclic_min1_;
  std::unique_ptr<const VectorFst<Arc>> transducer_acyclic_min2_;
  std::unique_ptr<const VectorFst<Arc>> transducer_cyclic_min_;
};

TEST_F(MinimizeTest, AcyclicMinimizeTest) {
  Minimize(m1_.get());
  EXPECT_TRUE(Verify(*m1_));
  EXPECT_TRUE(Equal(*m1_, *acyclic_min_));
}

TEST_F(MinimizeTest, CyclicMinimizeTest) {
  Minimize(m2_.get());
  EXPECT_TRUE(Verify(*m2_));
  EXPECT_TRUE(Equal(*m2_, *cyclic_min_));
}

TEST_F(MinimizeTest, WeightedAcyclicMinimizeTest) {
  Minimize(m3_.get());
  EXPECT_TRUE(Verify(*m3_));
  EXPECT_TRUE(Equal(*m3_, *weighted_acyclic_min_));
}

TEST_F(MinimizeTest, WeightedCyclicMinimizeTest) {
  Minimize(m4_.get());
  EXPECT_TRUE(Verify(*m4_));
  EXPECT_TRUE(Equal(*m4_, *weighted_cyclic_min_));
}

TEST_F(MinimizeTest, TransducerAcyclicMinimizeTest) {
  VectorFst<Arc> vfst1(*m5_);
  Minimize(&vfst1);
  EXPECT_TRUE(Verify(vfst1));
  EXPECT_TRUE(Equal(vfst1, *transducer_acyclic_min_));
  vfst1 = *m5_;
  VectorFst<Arc> vfst2;
  Minimize(&vfst1, &vfst2);
  EXPECT_TRUE(Verify(vfst1));
  EXPECT_TRUE(Equal(vfst1, *transducer_acyclic_min1_));
  EXPECT_TRUE(Equal(vfst2, *transducer_acyclic_min2_));
}

TEST_F(MinimizeTest, TransducerCyclicMinimizeTest) {
  Minimize(m6_.get());
  EXPECT_TRUE(Verify(*m6_));
  EXPECT_TRUE(Equal(*m6_, *transducer_cyclic_min_));
}

}  // namespace
}  // namespace fst
