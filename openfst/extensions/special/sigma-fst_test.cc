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

#include "openfst/extensions/special/sigma-fst.h"

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

class SigmaFstTest : public ::testing::Test {
 protected:
  void SetUp() override {
    const std::string path = JoinPath(
        std::string("."),
        "openfst/extensions/special/testdata");
    g_.reset(StdFst::Read(JoinPath(path, "g.fst")));
    h_.reset(StdFst::Read(JoinPath(path, "h.fst")));
    i1_.reset(StdFst::Read(JoinPath(path, "i1.fst")));
    i2_.reset(StdFst::Read(JoinPath(path, "i2.fst")));
  }

  bool ComposeTest(const StdFst& ifst1, const StdFst& ifst2,
                   const StdFst& ofst) {
    StdVectorFst cfst;
    Compose(ifst1, ifst2, &cfst);
    return Verify(cfst) && Equal(ofst, cfst);
  }

  // A string.
  std::unique_ptr<const StdFst> g_;
  // A context-free rewrite rule "contained" by sigma-star (where sigma = 3).
  std::unique_ptr<const StdFst> h_;
  // Application of h to g where labels matching sigma are passed through.
  std::unique_ptr<const StdFst> i1_;
  // Application of h to g where labels matching sigma are not passed through.
  std::unique_ptr<const StdFst> i2_;
};

TEST_F(SigmaFstTest, StdSigmaFstTest) {
  absl::SetFlag(&FLAGS_sigma_fst_sigma_label, 3);

  absl::SetFlag(&FLAGS_sigma_fst_rewrite_mode, "auto");
  EXPECT_TRUE(ComposeTest(*g_, StdSigmaFst(*h_), *i1_));

  absl::SetFlag(&FLAGS_sigma_fst_rewrite_mode, "never");
  EXPECT_TRUE(ComposeTest(*g_, StdSigmaFst(*h_), *i1_));

  absl::SetFlag(&FLAGS_sigma_fst_rewrite_mode, "always");
  EXPECT_TRUE(ComposeTest(*g_, StdSigmaFst(*h_), *i2_));
}

TEST_F(SigmaFstTest, StdInputSigmaFstTest) {
  absl::SetFlag(&FLAGS_sigma_fst_sigma_label, 3);

  absl::SetFlag(&FLAGS_sigma_fst_rewrite_mode, "never");
  EXPECT_TRUE(ComposeTest(*g_, StdInputSigmaFst(*h_), *i1_));

  absl::SetFlag(&FLAGS_sigma_fst_rewrite_mode, "always");
  EXPECT_TRUE(ComposeTest(*g_, StdInputSigmaFst(*h_), *i2_));
}

}  // namespace
}  // namespace fst
