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

#include "openfst/extensions/pdt/replace.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "absl/strings/string_view.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/equal.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/reverse.h"
#include "openfst/lib/rmepsilon.h"
#include "openfst/lib/util.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/lib/verify.h"

namespace fst {
namespace {

using Arc = StdArc;
using Label = Arc::Label;
using StateId = Arc::StateId;
using Weight = Arc::Weight;

constexpr absl::string_view kTestDir =
    "openfst/extensions/pdt/testdata";

class ReplaceTest : public testing::Test {
 protected:
  void SetUp() override {
    const std::string replace1_name =
        JoinPath(std::string("."), kTestDir, "r1.fst");
    const std::string replace2_name =
        JoinPath(std::string("."), kTestDir, "r2.fst");
    const std::string replace3_name =
        JoinPath(std::string("."), kTestDir, "r3.fst");
    const std::string replace4_name =
        JoinPath(std::string("."), kTestDir, "r4.fst");
    const std::string replace5_name =
        JoinPath(std::string("."), kTestDir, "r5.fst");
    const std::string replace6_name =
        JoinPath(std::string("."), kTestDir, "r6.fst");
    const std::string replace7_name =
        JoinPath(std::string("."), kTestDir, "r7.fst");
    const std::string replace8_name =
        JoinPath(std::string("."), kTestDir, "r8.fst");
    const std::string replace9_name =
        JoinPath(std::string("."), kTestDir, "r9.fst");
    const std::string replace10_name =
        JoinPath(std::string("."), kTestDir, "r10.fst");
    const std::string replace11_name =
        JoinPath(std::string("."), kTestDir, "r11.fst");
    const std::string replace12_name =
        JoinPath(std::string("."), kTestDir, "r12.fst");

    const std::string parens_name =
        JoinPath(std::string("."), kTestDir, "rparen.pairs");

    rfst1_.reset(VectorFst<Arc>::Read(replace1_name));
    rfst2_.reset(VectorFst<Arc>::Read(replace2_name));
    rfst3_.reset(VectorFst<Arc>::Read(replace3_name));
    rfst4_.reset(VectorFst<Arc>::Read(replace4_name));
    rfst5_.reset(VectorFst<Arc>::Read(replace5_name));
    rfst6_.reset(VectorFst<Arc>::Read(replace6_name));
    rfst7_.reset(VectorFst<Arc>::Read(replace7_name));
    rfst8_.reset(VectorFst<Arc>::Read(replace8_name));
    rfst9_.reset(VectorFst<Arc>::Read(replace9_name));
    rfst10_.reset(VectorFst<Arc>::Read(replace10_name));
    rfst11_.reset(VectorFst<Arc>::Read(replace11_name));
    rfst12_.reset(VectorFst<Arc>::Read(replace12_name));

    ASSERT_TRUE(ReadLabelPairs(parens_name, &parens_));
  }

  std::unique_ptr<const VectorFst<Arc>> rfst1_;
  std::unique_ptr<const VectorFst<Arc>> rfst2_;
  std::unique_ptr<const VectorFst<Arc>> rfst3_;
  std::unique_ptr<const VectorFst<Arc>> rfst4_;
  std::unique_ptr<const VectorFst<Arc>> rfst5_;
  std::unique_ptr<const VectorFst<Arc>> rfst6_;
  std::unique_ptr<const VectorFst<Arc>> rfst7_;
  std::unique_ptr<const VectorFst<Arc>> rfst8_;
  std::unique_ptr<const VectorFst<Arc>> rfst9_;
  std::unique_ptr<const VectorFst<Arc>> rfst10_;
  std::unique_ptr<const VectorFst<Arc>> rfst11_;
  std::unique_ptr<const VectorFst<Arc>> rfst12_;
  std::vector<std::pair<Label, Label>> parens_;
};

TEST_F(ReplaceTest, PdtReplaceLP) {
  // Default (left parser).
  VectorFst<Arc> vfst;
  std::vector<std::pair<Label, Label>> parens;
  std::vector<std::pair<Label, const Fst<Arc>*>> fst_array;
  fst_array.emplace_back(3, rfst1_.get());
  fst_array.emplace_back(4, rfst2_.get());
  fst_array.emplace_back(5, rfst3_.get());
  fst_array.emplace_back(6, rfst4_.get());

  Replace(fst_array, &vfst, &parens, 3); /* PdtParserType::LEFT */
  ASSERT_TRUE(Verify(vfst));
  ASSERT_TRUE(Equal(*rfst5_, vfst));
  ASSERT_TRUE(parens == parens_);
}

TEST_F(ReplaceTest, PdtReplaceSR) {
  // Left strongly-regular parser.
  VectorFst<Arc> vfst;
  std::vector<std::pair<Label, Label>> parens;

  {  // No strongly regular components.
    std::vector<std::pair<Label, const Fst<Arc>*>> fst_array;
    fst_array.emplace_back(3, rfst1_.get());
    fst_array.emplace_back(4, rfst2_.get());
    fst_array.emplace_back(5, rfst3_.get());
    fst_array.emplace_back(6, rfst4_.get());
    PdtReplaceOptions<Arc> opts(3, PdtParserType::LEFT_SR);
    Replace(fst_array, &vfst, &parens, opts);
    ASSERT_TRUE(Verify(vfst));
    ASSERT_TRUE(Equal(*rfst5_, vfst));
    ASSERT_TRUE(parens == parens_);
  }

  {  // Left-linear input.
    std::vector<std::pair<Label, const Fst<Arc>*>> fst_array;
    fst_array.emplace_back(4, rfst7_.get());
    fst_array.emplace_back(5, rfst8_.get());
    PdtReplaceOptions<Arc> opts(4, PdtParserType::LEFT_SR);
    Replace(fst_array, &vfst, &parens, opts);
    ASSERT_TRUE(Verify(vfst));
    ASSERT_TRUE(Equal(*rfst9_, vfst));
    ASSERT_TRUE(parens.empty());
  }

  {  // Right-linear input.
    VectorFst<Arc> rfst7;
    VectorFst<Arc> rfst8;
    Reverse(*rfst7_, &rfst7);
    RmEpsilon(&rfst7);
    Reverse(*rfst8_, &rfst8);
    RmEpsilon(&rfst8);
    std::vector<std::pair<Label, const Fst<Arc>*>> fst_array;
    fst_array.emplace_back(4, &rfst7);
    fst_array.emplace_back(5, &rfst8);
    PdtReplaceOptions<Arc> opts(4, PdtParserType::LEFT_SR);
    Replace(fst_array, &vfst, &parens, opts);
    ASSERT_TRUE(Verify(vfst));
    ASSERT_TRUE(Equal(*rfst10_, vfst));
    ASSERT_TRUE(parens.empty());
  }

  {  // Left-linear SCC input.
    std::vector<std::pair<Label, const Fst<Arc>*>> fst_array;
    fst_array.emplace_back(6, rfst6_.get());
    fst_array.emplace_back(4, rfst7_.get());
    fst_array.emplace_back(5, rfst8_.get());
    PdtReplaceOptions<Arc> opts(6, PdtParserType::LEFT_SR);
    Replace(fst_array, &vfst, &parens, opts);
    ASSERT_TRUE(Verify(vfst));
    ASSERT_TRUE(Equal(*rfst11_, vfst));
    ASSERT_EQ(parens.size(), 2);
  }

  {  // Right-linear SCC input.
    VectorFst<Arc> rfst6;
    VectorFst<Arc> rfst7;
    VectorFst<Arc> rfst8;
    Reverse(*rfst6_, &rfst6);
    RmEpsilon(&rfst6);
    Reverse(*rfst7_, &rfst7);
    RmEpsilon(&rfst7);
    Reverse(*rfst8_, &rfst8);
    RmEpsilon(&rfst8);
    std::vector<std::pair<Label, const Fst<Arc>*>> fst_array;
    fst_array.emplace_back(6, &rfst6);
    fst_array.emplace_back(4, &rfst7);
    fst_array.emplace_back(5, &rfst8);
    PdtReplaceOptions<Arc> opts(6, PdtParserType::LEFT_SR);
    Replace(fst_array, &vfst, &parens, opts);
    ASSERT_TRUE(Verify(vfst));
    ASSERT_TRUE(Equal(*rfst12_, vfst));
    ASSERT_EQ(parens.size(), 2);
  }
}

}  // namespace
}  // namespace fst
