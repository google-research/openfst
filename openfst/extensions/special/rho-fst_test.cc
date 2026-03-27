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

#include "openfst/extensions/special/rho-fst.h"

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

class RhoFstTest : public ::testing::Test {
 protected:
  void SetUp() override {
    const std::string path = JoinPath(
        std::string("."),
        "openfst/extensions/special/testdata");
    d1_.reset(StdFst::Read(JoinPath(path, "d1.fst")));
    d2_.reset(StdFst::Read(JoinPath(path, "d2.fst")));
    e_.reset(StdFst::Read(JoinPath(path, "e.fst")));
    f1_.reset(StdFst::Read(JoinPath(path, "f1.fst")));
    f2_.reset(StdFst::Read(JoinPath(path, "f2.fst")));
  }

  bool ComposeTest(const StdFst& ifst1, const StdFst& ifst2,
                   const StdFst& ofst) {
    StdVectorFst cfst;
    Compose(ifst1, ifst2, &cfst);
    return Verify(cfst) && Equal(ofst, cfst);
  }

  // A string matching e.
  std::unique_ptr<const StdFst> d1_;
  // A string not matching e.
  std::unique_ptr<const StdFst> d2_;
  // A context-free rewrite rule unioned with rho-star (where rho = 2)....
  std::unique_ptr<const StdFst> e_;
  // Application of e to d1.
  std::unique_ptr<const StdFst> f1_;
  // Application of e to d2 where labels matching rho are not passed through..
  std::unique_ptr<const StdFst> f2_;
};

TEST_F(RhoFstTest, StdRhoFstTest) {
  absl::SetFlag(&FLAGS_rho_fst_rho_label, 2);

  absl::SetFlag(&FLAGS_rho_fst_rewrite_mode, "auto");
  EXPECT_TRUE(ComposeTest(*d1_, StdRhoFst(*e_), *f1_));
  EXPECT_TRUE(ComposeTest(*d2_, StdRhoFst(*e_), *f2_));

  absl::SetFlag(&FLAGS_rho_fst_rewrite_mode, "never");
  EXPECT_TRUE(ComposeTest(*d1_, StdRhoFst(*e_), *f1_));
  EXPECT_TRUE(ComposeTest(*d2_, StdRhoFst(*e_), *f2_));

  absl::SetFlag(&FLAGS_rho_fst_rewrite_mode, "always");
  EXPECT_TRUE(ComposeTest(*d1_, StdRhoFst(*e_), *f1_));
  EXPECT_TRUE(ComposeTest(*d2_, StdRhoFst(*e_), *d2_));
}

TEST_F(RhoFstTest, StdInputRhoFstTest) {
  absl::SetFlag(&FLAGS_rho_fst_rho_label, 2);

  absl::SetFlag(&FLAGS_rho_fst_rewrite_mode, "never");
  EXPECT_TRUE(ComposeTest(*d1_, StdInputRhoFst(*e_), *f1_));
  EXPECT_TRUE(ComposeTest(*d2_, StdInputRhoFst(*e_), *f2_));

  absl::SetFlag(&FLAGS_rho_fst_rewrite_mode, "always");
  EXPECT_TRUE(ComposeTest(*d1_, StdInputRhoFst(*e_), *f1_));
  EXPECT_TRUE(ComposeTest(*d2_, StdInputRhoFst(*e_), *d2_));
}

}  // namespace
}  // namespace fst
