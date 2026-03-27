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
// Unit test for MPDT reversal.

#include "openfst/extensions/mpdt/reverse.h"

#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "openfst/extensions/mpdt/read_write_utils.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/equal.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/lib/verify.h"

namespace fst {
namespace {

using Arc = StdArc;
using Label = Arc::Label;
using StateId = Arc::StateId;
using Weight = Arc::Weight;
using Level = Label;

class ReverseTest : public testing::Test {
 protected:
  void SetUp() override {
    const std::string reverse1_name = JoinPath(
        std::string("."),
        "openfst/extensions/mpdt/testdata/v1.fst");
    const std::string reverse2_name = JoinPath(
        std::string("."),
        "openfst/extensions/mpdt/testdata/v2.fst");

    const std::string parens_name = JoinPath(
        std::string("."),
        "openfst/extensions/mpdt/testdata/vparen.triples");

    rfst1_.reset(VectorFst<Arc>::Read(reverse1_name));
    // rfst2_ = PdtReverse(rfst1_)
    rfst2_.reset(VectorFst<Arc>::Read(reverse2_name));

    ASSERT_TRUE(ReadLabelTriples(parens_name, &parens_, &assignments_));
  }

  std::unique_ptr<const VectorFst<Arc>> rfst1_;
  std::unique_ptr<const VectorFst<Arc>> rfst2_;
  std::vector<std::pair<Label, Label>> parens_;
  std::vector<Level> assignments_;
};

TEST_F(ReverseTest, PdtReverse) {
  VectorFst<Arc> vfst;
  std::vector<Level> assignments(assignments_);
  Reverse(*rfst1_, parens_, &assignments, &vfst);
  ASSERT_TRUE(Verify(vfst));
  ASSERT_TRUE(Equal(*rfst2_, vfst));
  for (size_t i = 0; i < assignments.size(); ++i) {
    if (assignments[i] == 1) {
      ASSERT_EQ(assignments_[i], 2);
    } else if (assignments[i] == 2) {
      ASSERT_EQ(assignments_[i], 1);
    }
  }
}

}  // namespace
}  // namespace fst
