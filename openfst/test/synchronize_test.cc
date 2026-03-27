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
// Unit test for Synchronize.

#include "openfst/lib/synchronize.h"

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

class SynchronizeTest : public testing::Test {
 protected:
  void SetUp() override {
    const std::string path =
        JoinPath(std::string("."),
                       "openfst/test/testdata/synchronize");
    const std::string synchronize1_name = JoinPath(path, "s1.fst");
    const std::string synchronize2_name = JoinPath(path, "s2.fst");

    sfst1_.reset(VectorFst<Arc>::Read(synchronize1_name));
    sfst2_.reset(VectorFst<Arc>::Read(synchronize2_name));
  }

  std::unique_ptr<const VectorFst<Arc>> sfst1_;
  std::unique_ptr<const VectorFst<Arc>> sfst2_;
};

TEST_F(SynchronizeTest, SynchronizeFst) {
  VectorFst<Arc> nfst;

  const SynchronizeFst<Arc> dfst(*sfst1_);
  ASSERT_TRUE(Verify(dfst));
  ASSERT_TRUE(Equal(dfst, *sfst2_));

  const SynchronizeFst<Arc> ndfst(nfst);
  ASSERT_TRUE(Verify(ndfst));

  for (const bool safe : {false, true}) {
    const SynchronizeFst<Arc> cfst(dfst, safe);
    ASSERT_TRUE(Verify(cfst));
    ASSERT_TRUE(Equal(cfst, *sfst2_));
  }
}

}  // namespace
}  // namespace fst
