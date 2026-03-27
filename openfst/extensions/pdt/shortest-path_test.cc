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
// Unit test for ShortestPath.

#include "openfst/extensions/pdt/shortest-path.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/equal.h"
#include "openfst/lib/util.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/lib/verify.h"

namespace fst {
namespace {

using Arc = StdArc;
using Label = Arc::Label;
using StateId = Arc::StateId;
using Weight = Arc::Weight;

class ShortestPathTest : public testing::Test {
 protected:
  void SetUp() override {
    const std::string shortest_path1_name = JoinPath(
        std::string("."),
        "openfst/extensions/pdt/testdata/sp1.fst");
    const std::string shortest_path2_name = JoinPath(
        std::string("."),
        "openfst/extensions/pdt/testdata/sp2.fst");
    const std::string shortest_path3_name = JoinPath(
        std::string("."),
        "openfst/extensions/pdt/testdata/sp3.fst");
    const std::string shortest_path4_name = JoinPath(
        std::string("."),
        "openfst/extensions/pdt/testdata/sp4.fst");

    const std::string parens_name = JoinPath(
        std::string("."),
        "openfst/extensions/pdt/testdata/spparen.pairs");

    spfst1_.reset(VectorFst<Arc>::Read(shortest_path1_name));
    spfst2_.reset(VectorFst<Arc>::Read(shortest_path2_name));
    spfst3_.reset(VectorFst<Arc>::Read(shortest_path3_name));
    spfst4_.reset(VectorFst<Arc>::Read(shortest_path4_name));

    ASSERT_TRUE(ReadLabelPairs(parens_name, &parens_));
  }

  std::unique_ptr<const VectorFst<Arc>> spfst1_;
  std::unique_ptr<const VectorFst<Arc>> spfst2_;
  std::unique_ptr<const VectorFst<Arc>> spfst3_;
  std::unique_ptr<const VectorFst<Arc>> spfst4_;
  std::vector<std::pair<Label, Label>> parens_;
};

// PDT with single-state paren sources
TEST_F(ShortestPathTest, SingleShortestPath) {
  VectorFst<Arc> path;
  ShortestPath(*spfst1_, parens_, &path);
  ASSERT_TRUE(Verify(path));
  ASSERT_TRUE(Equal(*spfst2_, path));
}

// PDT with multi-state paren sources
TEST_F(ShortestPathTest, MultiShortestPath) {
  VectorFst<Arc> path;
  ShortestPath(*spfst3_, parens_, &path);
  ASSERT_TRUE(Verify(path));
  ASSERT_TRUE(Equal(*spfst4_, path));
}

}  // namespace
}  // namespace fst
