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
// Unit test for parenthesis utitlities

#include "openfst/extensions/pdt/paren.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "absl/strings/string_view.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/util.h"
#include "openfst/lib/vector-fst.h"

namespace fst {
namespace internal {
namespace {

using Arc = StdArc;
using Label = StdArc::Label;
using StateId = StdArc::StateId;
using Weight = StdArc::Weight;

constexpr absl::string_view kTestDir =
    "openfst/extensions/pdt/testdata";

class ParenTest : public testing::Test {
 protected:
  void SetUp() override {
    const std::string paren_fst1_name =
        JoinPath(std::string("."), kTestDir, "p1.fst");
    const std::string parens_name =
        JoinPath(std::string("."), kTestDir, "pparen.pairs");

    pfst1_.reset(VectorFst<Arc>::Read(paren_fst1_name));

    ASSERT_TRUE(ReadLabelPairs(parens_name, &parens_));
  }

  bool ParenReach(const PdtParenReachable<Arc>& reachable, StateId s,
                  int paren_id) {
    for (auto paren_iter = reachable.FindParens(s); !paren_iter.Done();
         paren_iter.Next()) {
      if (paren_iter.Value() == paren_id) return true;
    }
    return false;
  }

  std::unique_ptr<VectorFst<Arc>> pfst1_;
  std::vector<std::pair<Label, Label>> parens_;
};

TEST_F(ParenTest, ParenReachTest) {
  PdtParenReachable<Arc> reachable(*pfst1_, parens_, true);
  ASSERT_FALSE(ParenReach(reachable, 0, 0));
  ASSERT_FALSE(ParenReach(reachable, 0, 1));
  ASSERT_FALSE(ParenReach(reachable, 1, 0));
  ASSERT_FALSE(ParenReach(reachable, 1, 1));

  ASSERT_TRUE(ParenReach(reachable, 2, 0));
  ASSERT_TRUE(ParenReach(reachable, 2, 1));
  ASSERT_TRUE(ParenReach(reachable, 3, 0));
  ASSERT_TRUE(ParenReach(reachable, 3, 1));
  ASSERT_TRUE(ParenReach(reachable, 4, 0));
  ASSERT_TRUE(ParenReach(reachable, 4, 1));
  ASSERT_TRUE(ParenReach(reachable, 5, 0));
  ASSERT_TRUE(ParenReach(reachable, 5, 1));

  ASSERT_FALSE(ParenReach(reachable, 6, 0));
  ASSERT_FALSE(ParenReach(reachable, 6, 1));

  ASSERT_FALSE(ParenReach(reachable, 7, 0));
  ASSERT_TRUE(ParenReach(reachable, 7, 1));

  ASSERT_FALSE(ParenReach(reachable, 8, 0));
  ASSERT_FALSE(ParenReach(reachable, 8, 1));
  ASSERT_FALSE(ParenReach(reachable, 9, 0));
  ASSERT_FALSE(ParenReach(reachable, 9, 1));
  ASSERT_FALSE(ParenReach(reachable, 10, 0));
  ASSERT_FALSE(ParenReach(reachable, 10, 1));
}

}  // namespace
}  // namespace internal
}  // namespace fst
