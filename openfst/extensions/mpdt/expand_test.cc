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

#include <fstream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "absl/flags/flag.h"
#include "absl/flags/reflection.h"
#include "absl/strings/string_view.h"
#include "openfst/extensions/mpdt/read_write_utils.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/equal.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/properties.h"
#include "openfst/lib/util.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/lib/verify.h"

namespace fst {
namespace {

using Arc = StdArc;
using Label = Arc::Label;
using StateId = Arc::StateId;
using Weight = Arc::Weight;
using Level = Label;

constexpr absl::string_view kTestDir =
    "openfst/extensions/mpdt/testdata";

class ExpandTest : public testing::Test {
 protected:
  void SetUp() override {
    const std::string expand1_name =
        JoinPath(std::string("."), kTestDir, "e1.fst");
    const std::string expand2_name =
        JoinPath(std::string("."), kTestDir, "e2.fst");
    const std::string expand3_name =
        JoinPath(std::string("."), kTestDir, "e3.fst");

    const std::string parens_name =
        JoinPath(std::string("."), kTestDir, "eparen.triples");

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
  absl::FlagSaver fs_;
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

TEST_F(ExpandTest, ReadLabelTriplesClearsOutputsAndRejectsMalformedLines) {
  const std::string parens_name =
      JoinPath(std::string("."), kTestDir, "eparen.triples");
  const auto expected_parens = parens_;
  const auto expected_assignments = assignments_;

  // Re-reading into already populated vectors must clear both `parens_` and
  // `assignments_`.
  ASSERT_TRUE(ReadLabelTriples(parens_name, &parens_, &assignments_));
  EXPECT_EQ(parens_, expected_parens);
  EXPECT_EQ(assignments_, expected_assignments);

  const std::string bad_path =
      JoinPath(::testing::TempDir(), "bad.triples");
  absl::SetFlag(&FLAGS_fst_error_fatal, false);
  for (const char* bad_contents : {"1 2 1\n3 4\n", "1 2 1\nbad 4 1\n",
                                   "1 2 1\n3 bad 1\n", "1 2 1\n3 4 bad\n"}) {
    {
      std::ofstream ostrm(bad_path);
      ASSERT_TRUE(ostrm.is_open());
      ostrm << bad_contents;
    }
    parens_ = expected_parens;
    assignments_ = expected_assignments;
    EXPECT_FALSE(ReadLabelTriples(bad_path, &parens_, &assignments_))
        << "bad_contents = " << bad_contents;
    EXPECT_TRUE(parens_.empty()) << "bad_contents = " << bad_contents;
    EXPECT_TRUE(assignments_.empty()) << "bad_contents = " << bad_contents;
  }
}

TEST_F(ExpandTest, DuplicateParenthesesSetsError) {
  absl::SetFlag(&FLAGS_fst_error_fatal, false);

  struct BadCase {
    std::vector<std::pair<Label, Label>> parens;
    std::vector<Level> assignments;
  };
  for (const auto& tc : std::vector<BadCase>{{{{10, 10}}, {1}},
                                             {{{10, 11}, {10, 12}}, {1, 2}}}) {
    MPdtExpandFst<Arc> xfst(*efst1_, tc.parens, tc.assignments);
    EXPECT_TRUE(xfst.Properties(kError, true));
    EXPECT_EQ(xfst.Start(), kNoStateId);

    VectorFst<Arc> ofst;
    Expand(*efst1_, tc.parens, tc.assignments, &ofst);
    EXPECT_TRUE(ofst.Properties(kError, true));
  }
}

}  // namespace
}  // namespace fst
