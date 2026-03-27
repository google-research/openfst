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
// Unit test for EpsNormalize.

#include "openfst/lib/epsnormalize.h"

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
using StateId = Arc::StateId;
using Weight = Arc::Weight;
using Label = Arc::Label;

class EpsNormalizeTest : public testing::Test {
 protected:
  void SetUp() override {
    const std::string path = JoinPath(
        std::string("."),
        "openfst/test/testdata/epsnormalize");
    const std::string epsnorm1_name = JoinPath(path, "e1.fst");
    const std::string epsnorm2_name = JoinPath(path, "e2.fst");
    const std::string epsnorm3_name = JoinPath(path, "e3.fst");
    const std::string epsnorm4_name = JoinPath(path, "e4.fst");
    const std::string epsnorm5_name = JoinPath(path, "e5.fst");
    const std::string epsnorm6_name = JoinPath(path, "e6.fst");

    efst1_.reset(VectorFst<Arc>::Read(epsnorm1_name));
    efst2_.reset(VectorFst<Arc>::Read(epsnorm2_name));
    efst3_.reset(VectorFst<Arc>::Read(epsnorm3_name));
    efst4_.reset(VectorFst<Arc>::Read(epsnorm4_name));
    efst5_.reset(VectorFst<Arc>::Read(epsnorm5_name));
    efst6_.reset(VectorFst<Arc>::Read(epsnorm6_name));
  }

  std::unique_ptr<const VectorFst<Arc>> efst1_;
  std::unique_ptr<const VectorFst<Arc>> efst2_;
  std::unique_ptr<const VectorFst<Arc>> efst3_;
  std::unique_ptr<const VectorFst<Arc>> efst4_;
  std::unique_ptr<const VectorFst<Arc>> efst5_;
  std::unique_ptr<const VectorFst<Arc>> efst6_;
};

TEST_F(EpsNormalizeTest, EpsNormalizeInput) {
  VectorFst<Arc> nfst;
  VectorFst<Arc> vfst;

  EpsNormalize(*efst1_, &vfst, EPS_NORM_INPUT);
  ASSERT_TRUE(Verify(vfst));
  ASSERT_TRUE(Equal(*efst2_, vfst));

  EpsNormalize(nfst, &vfst, EPS_NORM_INPUT);
  ASSERT_TRUE(Verify(vfst));
  ASSERT_TRUE(Equal(nfst, vfst));
}

TEST_F(EpsNormalizeTest, EpsNormalizeOutput) {
  VectorFst<Arc> nfst;
  VectorFst<Arc> vfst;

  EpsNormalize(*efst3_, &vfst, EPS_NORM_OUTPUT);
  ASSERT_TRUE(Verify(vfst));
  ASSERT_TRUE(Equal(*efst4_, vfst));

  EpsNormalize(nfst, &vfst, EPS_NORM_OUTPUT);
  ASSERT_TRUE(Verify(vfst));
  ASSERT_TRUE(Equal(nfst, vfst));
}

TEST_F(EpsNormalizeTest, EpsNormalizeNonFunctional) {
  VectorFst<Arc> vfst;

  EpsNormalize(*efst5_, &vfst, EPS_NORM_INPUT);
  ASSERT_TRUE(Verify(vfst));
  ASSERT_TRUE(Equal(*efst6_, vfst));
}

}  // namespace
}  // namespace fst
