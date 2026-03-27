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
// Unit test for MPDT expansion to an FST.

#include "openfst/extensions/mpdt/expand.h"

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

class ExpandTest : public testing::Test {
 protected:
  void SetUp() override {
    const std::string expand1_name = JoinPath(
        std::string("."),
        "openfst/extensions/mpdt/testdata/e1.fst");
    const std::string expand2_name = JoinPath(
        std::string("."),
        "openfst/extensions/mpdt/testdata/e2.fst");
    const std::string expand3_name = JoinPath(
        std::string("."),
        "openfst/extensions/mpdt/testdata/e3.fst");

    const std::string parens_name = JoinPath(
        std::string("."),
        "openfst/extensions/mpdt/testdata/eparen.triples");

    efst1_.reset(VectorFst<Arc>::Read(expand1_name));
    // efst2_ = Expand(efst1_)
    efst2_.reset(VectorFst<Arc>::Read(expand2_name));
    // efst3_ = Expand(efst1_) with keep_parentheses = true
    efst3_.reset(VectorFst<Arc>::Read(expand3_name));

    ASSERT_TRUE(ReadLabelTriples(parens_name, &parens_, &assignments_));
  }

  std::unique_ptr<const VectorFst<Arc>> efst1_, efst2_, efst3_, efst4_;
  std::vector<std::pair<Label, Label>> parens_;
  std::vector<Level> assignments_;
};

TEST_F(ExpandTest, MpdtExpandFst) {
  MPdtExpandFst<Arc> xfst(*efst1_, parens_, assignments_);
  ASSERT_TRUE(Verify(xfst));

  MPdtExpandFstOptions<Arc> opts;
  opts.keep_parentheses = true;
  MPdtExpandFst<Arc> yfst(*efst1_, parens_, assignments_, opts);
  ASSERT_TRUE(Verify(yfst));

  MPdtExpandFst<Arc> zfst(yfst, /*safe=*/true);
  ASSERT_TRUE(Verify(zfst));
}

TEST_F(ExpandTest, MpdtExpand) {
  VectorFst<Arc> vfst;
  Expand(*efst1_, parens_, assignments_, &vfst);
  ASSERT_TRUE(Verify(vfst));
  ASSERT_TRUE(Equal(*efst2_, vfst));

  Expand(*efst1_, parens_, assignments_, &vfst, true, true);
  ASSERT_TRUE(Verify(vfst));
  ASSERT_TRUE(Equal(*efst3_, vfst));
}

}  // namespace
}  // namespace fst
