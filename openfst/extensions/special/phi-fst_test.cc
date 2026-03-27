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

#include "openfst/extensions/special/phi-fst.h"

#include <memory>
#include <string>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "absl/flags/flag.h"
#include "openfst/lib/compose.h"
#include "openfst/lib/equal.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/lib/verify.h"

namespace fst {
namespace {

class PhiFstTest : public ::testing::Test {
 protected:
  void SetUp() override {
    const std::string path = JoinPath(
        std::string("."),
        "openfst/extensions/special/testdata");
    a_.reset(StdFst::Read(JoinPath(path, "a.fst")));
    b1_.reset(StdFst::Read(JoinPath(path, "b1.fst")));
    b2_.reset(StdFst::Read(JoinPath(path, "b2.fst")));
    c1_.reset(StdFst::Read(JoinPath(path, "c1.fst")));
    c2_.reset(StdFst::Read(JoinPath(path, "c2.fst")));
    c3_.reset(StdFst::Read(JoinPath(path, "c3.fst")));
  }

  bool ComposeTest(const StdFst& ifst1, const StdFst& ifst2,
                   const StdFst& ofst) {
    StdVectorFst cfst;
    Compose(ifst1, ifst2, &cfst);
    return Verify(cfst) && Equal(ofst, cfst);
  }

  std::unique_ptr<const StdFst> a_;
  std::unique_ptr<const StdFst> b1_;
  std::unique_ptr<const StdFst> b2_;
  std::unique_ptr<const StdFst> c1_;
  std::unique_ptr<const StdFst> c2_;
  std::unique_ptr<const StdFst> c3_;
};

TEST_F(PhiFstTest, StdPhiFstTest) {
  absl::SetFlag(&FLAGS_phi_fst_phi_label, 0);

  absl::SetFlag(&FLAGS_phi_fst_rewrite_mode, "auto");
  EXPECT_TRUE(ComposeTest(StdPhiFst(*b1_), *a_, *c1_));

  absl::SetFlag(&FLAGS_phi_fst_rewrite_mode, "never");
  EXPECT_TRUE(ComposeTest(*a_, StdPhiFst(*b1_), *c2_));

  absl::SetFlag(&FLAGS_phi_fst_phi_label, 4);
  absl::SetFlag(&FLAGS_phi_fst_rewrite_mode, "auto");
  EXPECT_TRUE(ComposeTest(*a_, StdPhiFst(*b2_), *c1_));

  // No phi-transitions in the FST.
  absl::SetFlag(&FLAGS_phi_fst_phi_label, 0);
  EXPECT_TRUE(ComposeTest(*a_, StdPhiFst(*b2_), *c3_));
}

TEST_F(PhiFstTest, StdInputPhiFstTest) {
  absl::SetFlag(&FLAGS_phi_fst_phi_label, 0);
  absl::SetFlag(&FLAGS_phi_fst_rewrite_mode, "auto");

  // Matches on input.
  EXPECT_TRUE(ComposeTest(*a_, StdInputPhiFst(*b1_), *c1_));

  // Matches on output.
  EXPECT_TRUE(ComposeTest(StdInputPhiFst(*b2_), *a_, *c3_));
}

TEST_F(PhiFstTest, StdOutputPhiFstTest) {
  absl::SetFlag(&FLAGS_phi_fst_phi_label, 0);
  absl::SetFlag(&FLAGS_phi_fst_rewrite_mode, "auto");

  // Matches on output.
  EXPECT_TRUE(ComposeTest(StdOutputPhiFst(*b1_), *a_, *c1_));

  // Matches on input.
  EXPECT_TRUE(ComposeTest(*a_, StdOutputPhiFst(*b2_), *c3_));
}

}  // namespace
}  // namespace fst
