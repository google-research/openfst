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
// Unit test for MPDT composition, minimally changed from the PDT version.

#include "openfst/extensions/mpdt/compose.h"

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
#include "openfst/extensions/pdt/compose.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/compose.h"
#include "openfst/lib/connect.h"
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

class ComposeTest : public testing::Test {
 protected:
  void SetUp() override {
    const std::string compose1_name =
        JoinPath(std::string("."), kTestDir, "c1.fst");
    const std::string compose2_name =
        JoinPath(std::string("."), kTestDir, "c2.fst");
    const std::string compose3_name =
        JoinPath(std::string("."), kTestDir, "c3.fst");

    const std::string parens_name =
        JoinPath(std::string("."), kTestDir, "eparen.triples");

    cfst1_.reset(VectorFst<Arc>::Read(compose1_name));
    cfst2_.reset(VectorFst<Arc>::Read(compose2_name));
    cfst3_.reset(VectorFst<Arc>::Read(compose3_name));

    ASSERT_TRUE(ReadLabelTriples(parens_name, &parens_, &assignments_));
  }

  absl::FlagSaver flag_saver_;
  std::unique_ptr<const VectorFst<Arc>> cfst1_;
  std::unique_ptr<const VectorFst<Arc>> cfst2_;
  std::unique_ptr<const VectorFst<Arc>> cfst3_;
  std::vector<std::pair<Label, Label>> parens_;
  std::vector<Level> assignments_;
};

TEST_F(ComposeTest, MpdtCompose) {
  VectorFst<Arc> vfst1;
  Compose(*cfst1_, parens_, assignments_, *cfst2_, &vfst1);
  ASSERT_TRUE(Verify(vfst1));
  ASSERT_TRUE(Equal(*cfst3_, vfst1));

  VectorFst<Arc> vfst2;
  Compose(*cfst2_, *cfst1_, parens_, assignments_, &vfst2);
  ASSERT_TRUE(Verify(vfst2));
  ASSERT_TRUE(Equal(*cfst3_, vfst2));
}

TEST_F(ComposeTest, FilteredMpdtCompose) {
  VectorFst<Arc> vfst1;
  MPdtComposeOptions opts(true, PdtComposeFilter::EXPAND_PAREN);

  Compose(*cfst1_, parens_, assignments_, *cfst2_, &vfst1, opts);
  ASSERT_TRUE(Verify(vfst1));
  ASSERT_TRUE(Equal(*cfst3_, vfst1));

  VectorFst<Arc> vfst2;

  Compose(*cfst2_, *cfst1_, parens_, assignments_, &vfst2, opts);
  ASSERT_TRUE(Verify(vfst2));
  ASSERT_TRUE(Equal(*cfst3_, vfst2));
}

TEST_F(ComposeTest, FilteredMpdtComposeFstSafeCopy) {
  MPdtComposeFstOptions<Arc, true> copts(*cfst1_, parens_, assignments_,
                                         *cfst2_, /*expand=*/true,
                                         /*keep_parens=*/true);
  ComposeFst<Arc> composed(*cfst1_, *cfst2_, copts);

  // Safe copy before any state expansion.
  std::unique_ptr<ComposeFst<Arc>> copy_unexpanded(composed.Copy(true));
  ASSERT_FALSE(copy_unexpanded->Properties(kError, true));
  VectorFst<Arc> vfst1(*copy_unexpanded);
  Connect(&vfst1);
  ASSERT_TRUE(Verify(vfst1));
  ASSERT_TRUE(Equal(*cfst3_, vfst1));

  // Partially expand across open-paren transitions so `stack_` has non-zero
  // stack IDs and `paren_id_` is active on the matchers, then safe-copy.
  const StateId start = composed.Start();
  ASSERT_NE(start, kNoStateId);
  for (ArcIterator<ComposeFst<Arc>> aiter(composed, start); !aiter.Done();
       aiter.Next()) {
    for (ArcIterator<ComposeFst<Arc>> aiter2(composed, aiter.Value().nextstate);
         !aiter2.Done(); aiter2.Next()) {
    }
  }
  std::unique_ptr<ComposeFst<Arc>> copy_partial(composed.Copy(true));
  ASSERT_FALSE(copy_partial->Properties(kError, true));
  VectorFst<Arc> vfst2(*copy_partial);
  Connect(&vfst2);
  ASSERT_TRUE(Verify(vfst2));
  ASSERT_TRUE(Equal(*cfst3_, vfst2));
}

TEST_F(ComposeTest, DuplicateParenthesesSetsError) {
  absl::SetFlag(&FLAGS_fst_error_fatal, false);

  struct BadCase {
    std::vector<std::pair<Label, Label>> parens;
    std::vector<Level> assignments;
  };
  for (const auto& tc : std::vector<BadCase>{{{{10, 10}}, {1}},
                                             {{{10, 11}, {10, 12}}, {1, 2}}}) {
    for (PdtComposeFilter filter :
         {PdtComposeFilter::PAREN, PdtComposeFilter::EXPAND,
          PdtComposeFilter::EXPAND_PAREN}) {
      const MPdtComposeOptions opts(true, filter);
      VectorFst<Arc> ofst1;
      Compose(*cfst1_, tc.parens, tc.assignments, *cfst2_, &ofst1, opts);
      EXPECT_TRUE(ofst1.Properties(kError, true));

      VectorFst<Arc> ofst2;
      Compose(*cfst2_, *cfst1_, tc.parens, tc.assignments, &ofst2, opts);
      EXPECT_TRUE(ofst2.Properties(kError, true));
    }
  }
}

}  // namespace
}  // namespace fst
