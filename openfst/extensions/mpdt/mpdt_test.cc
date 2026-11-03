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
// Unit test for multiple pushdown transducers.

#include "openfst/extensions/mpdt/mpdt.h"

#include <utility>
#include <vector>

#include "gtest/gtest.h"
#include "absl/flags/flag.h"
#include "absl/flags/reflection.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/util.h"

namespace fst {
namespace internal {
namespace {

class MPdtTest : public testing::Test {
 protected:
  void SetUp() override {
    // Parens for the first level.
    paren_set_.push_back(std::pair<int, int>(100, 103));
    assignments_.push_back(1);
    paren_set_.push_back(std::pair<int, int>(101, 104));
    assignments_.push_back(1);
    // Parens for the second level.
    paren_set_.push_back(std::pair<int, int>(102, 105));
    assignments_.push_back(2);
  }

  std::vector<std::pair<int, int>> paren_set_;
  std::vector<int> assignments_;
  absl::FlagSaver flag_saver_;
};

TEST_F(MPdtTest, Test0) {
  MPdtStack<StdArc::StateId, StdArc::Label, 2, MPdtType::READ_RESTRICT> stack(
      paren_set_, assignments_);
  int s2 = stack.Find(0, 102);  // Writes onto second stack: legal.
  ASSERT_NE(s2, -1);
  ASSERT_NE(stack.Find(s2, 105), -1);  // Reads from second stack: legal.
}

TEST_F(MPdtTest, Test1) {
  MPdtStack<StdArc::StateId, StdArc::Label, 2, MPdtType::READ_RESTRICT> stack(
      paren_set_, assignments_);
  int s1 = stack.Find(0, 101);         // Writes onto first stack: legal.
  ASSERT_NE(s1, -1);                   //
  int s2 = stack.Find(s1, 102);        // Writes onto second stack: legal.
  ASSERT_NE(s2, -1);                   //
  ASSERT_EQ(stack.Find(s2, 105), -1);  // Reads from second stack: illegal.
  int s3 = stack.Find(s2, 104);        // Reads from first stack: legal.
  ASSERT_NE(s3, -1);                   //
  ASSERT_NE(stack.Find(s3, 105), -1);  // Reads from second stack: now legal.
}

TEST_F(MPdtTest, Test2) {
  MPdtStack<StdArc::StateId, StdArc::Label, 2, MPdtType::WRITE_RESTRICT> stack(
      paren_set_, assignments_);
  int s1 = stack.Find(0, 100);         // Writes onto first stack: legal.
  ASSERT_NE(s1, -1);                   //
  int s2 = stack.Find(s1, 102);        // Writes onto second stack: illegal.
  ASSERT_EQ(s2, -1);                   //
  int s3 = stack.Find(s1, 103);        // Reads from first stack: legal.
  ASSERT_NE(s3, -1);                   //
  ASSERT_NE(stack.Find(s3, 102), -1);  // Writes onto second stack: now legal.
}

// Also tests Top().
TEST_F(MPdtTest, Test3) {
  MPdtStack<StdArc::StateId, StdArc::Label, 2, MPdtType::NO_RESTRICT> stack(
      paren_set_, assignments_);
  int s1 = stack.Find(0, 101);  // Writes onto first stack: legal.
  ASSERT_EQ(stack.Top(s1), 1);
  ASSERT_NE(s1, -1);
  int s2 = stack.Find(s1, 102);  // Writes onto second stack: legal.
  ASSERT_EQ(stack.Top(s2), 1);
  ASSERT_NE(s2, -1);
  int s3 = stack.Find(s2, 105);
  ASSERT_NE(s3, -1);                   // Reads from second stack: legal.
  ASSERT_NE(stack.Find(s3, 104), -1);  // Reads from first stack: legal.
}

TEST_F(MPdtTest, DuplicateParenthesesSetsError) {
  absl::SetFlag(&FLAGS_fst_error_fatal, false);

  struct BadCase {
    std::vector<std::pair<int, int>> parens;
    std::vector<int> assignments;
  };
  for (const auto& tc :
       std::vector<BadCase>{{{{100, 100}}, {1}},
                            {{{100, 101}, {100, 102}}, {1, 2}},
                            {{{100, 101}, {102, 101}}, {1, 2}},
                            {{{100, 101}, {101, 102}}, {1, 2}}}) {
    MPdtStack<StdArc::StateId, StdArc::Label, 2, MPdtType::READ_RESTRICT> stack(
        tc.parens, tc.assignments);
    EXPECT_TRUE(stack.Error());
    EXPECT_EQ(stack.Find(0, 100), -1);
    EXPECT_EQ(stack.Top(0), -1);
  }
}

}  // namespace
}  // namespace internal
}  // namespace fst
