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
// Unit test for PDT composition.

#include "openfst/extensions/pdt/compose.h"

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

class ComposeTest : public testing::Test {
 protected:
  void SetUp() override {
    const std::string compose1_name = JoinPath(
        std::string("."),
        "openfst/extensions/pdt/testdata/c1.fst");
    const std::string compose2_name = JoinPath(
        std::string("."),
        "openfst/extensions/pdt/testdata/c2.fst");
    const std::string compose3_name = JoinPath(
        std::string("."),
        "openfst/extensions/pdt/testdata/c3.fst");

    const std::string parens_name = JoinPath(
        std::string("."),
        "openfst/extensions/pdt/testdata/cparen.pairs");

    cfst1_.reset(VectorFst<Arc>::Read(compose1_name));
    cfst2_.reset(VectorFst<Arc>::Read(compose2_name));
    // cfst3_ = PdtCompose(cfst1_, cfst2_)
    cfst3_.reset(VectorFst<Arc>::Read(compose3_name));

    ASSERT_TRUE(ReadLabelPairs(parens_name, &parens_));
  }

  std::unique_ptr<const VectorFst<Arc>> cfst1_;
  std::unique_ptr<const VectorFst<Arc>> cfst2_;
  std::unique_ptr<const VectorFst<Arc>> cfst3_;
  std::vector<std::pair<Label, Label>> parens_;
};

TEST_F(ComposeTest, PdtCompose) {
  VectorFst<Arc> vfst1;
  Compose(*cfst1_, parens_, *cfst2_, &vfst1);
  ASSERT_TRUE(Verify(vfst1));
  ASSERT_TRUE(Equal(*cfst3_, vfst1));

  VectorFst<Arc> vfst2;
  Compose(*cfst2_, *cfst1_, parens_, &vfst2);
  ASSERT_TRUE(Verify(vfst2));
  ASSERT_TRUE(Equal(*cfst3_, vfst2));
}

TEST_F(ComposeTest, FilteredPdtCompose) {
  VectorFst<Arc> vfst1;
  PdtComposeOptions opts(true, PdtComposeFilter::EXPAND_PAREN);

  Compose(*cfst1_, parens_, *cfst2_, &vfst1, opts);
  ASSERT_TRUE(Verify(vfst1));
  ASSERT_TRUE(Equal(*cfst3_, vfst1));

  VectorFst<Arc> vfst2;

  Compose(*cfst2_, *cfst1_, parens_, &vfst2, opts);
  ASSERT_TRUE(Verify(vfst2));
  ASSERT_TRUE(Equal(*cfst3_, vfst2));
}

}  // namespace
}  // namespace fst
