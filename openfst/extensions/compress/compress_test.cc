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
// Unit test for Compress and Decompress functions.

#include "openfst/extensions/compress/compress.h"

#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "absl/flags/flag.h"
#include "absl/log/flags.h"
#include "openfst/extensions/compress/elias.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/isomorphic.h"
#include "openfst/lib/mutable-fst.h"
#include "openfst/lib/test-properties.h"
#include "openfst/lib/vector-fst.h"

using ::fst::Elias;
using ::fst::LempelZiv;
using ::fst::StdArc;
using ::fst::StdMutableFst;
using ::fst::StdVectorFst;
using ::fst::internal::ExpandLZCode;

namespace fst {
namespace {

class CompressTest : public testing::Test {
 protected:
  void SetUp() override {
    const std::string unweighted_fstname = JoinPath(
        std::string("."),
        "/openfst/extensions/compress/testdata",
        "unweight.fst");
    unweight_fst_.reset(StdVectorFst::Read(unweighted_fstname));
  }

  std::unique_ptr<StdMutableFst> unweight_fst_;
};

// Testing if the output after decode and encode is isomorphic to
// the original unweighted fst
TEST_F(CompressTest, UnWeightedDecodeAndEncode) {
  StdVectorFst input_fst(*unweight_fst_);
  StdVectorFst output_fst;
  const std::string unweighted_output =
      JoinPath(::testing::TempDir(), "unweight_output.fstz");
  ASSERT_TRUE(Compress(input_fst, unweighted_output));
  ASSERT_TRUE(Decompress(unweighted_output, &output_fst));
  EXPECT_TRUE(Isomorphic(input_fst, output_fst));
}

// Tests LempelZiv compression and ExpandLZCode.
TEST_F(CompressTest, LempelZivOverCharacters) {
  std::vector<char> input = {'a', 'b', 'a', 'b', 'a', 'b', 'b'};
  LempelZiv<int, char, std::less<char>, std::equal_to<char>> lempel_char;
  std::vector<std::pair<int, char>> code;
  std::vector<char> output;

  // Testing LZ compression.
  lempel_char.BatchEncode(input, &code);
  ASSERT_TRUE(lempel_char.BatchDecode(code, &output));
  EXPECT_TRUE(input == output);

  // Testing ExpandLZCode.
  std::vector<std::vector<char>> expanded_code;
  ASSERT_TRUE(ExpandLZCode(code, &expanded_code));
  std::vector<std::vector<char>> expected_expanded_code;
  expected_expanded_code.push_back(std::vector<char>({'a'}));
  expected_expanded_code.push_back(std::vector<char>({'b'}));
  expected_expanded_code.push_back(std::vector<char>({'a', 'b'}));
  expected_expanded_code.push_back(std::vector<char>({'a', 'b', 'b'}));
  EXPECT_TRUE(expanded_code == expected_expanded_code);
}

// Tests Compress on an empty FST.
TEST_F(CompressTest, EmptyFst) {
  StdVectorFst input_fst, output_fst;
  const std::string output_fstname =
      JoinPath(::testing::TempDir(), "empty_output.fstz");
  ASSERT_TRUE(Compress(input_fst, output_fstname));
  ASSERT_TRUE(Decompress(output_fstname, &output_fst));
  EXPECT_TRUE(Isomorphic(input_fst, output_fst));
}

// Tests Elias encoding.
TEST_F(CompressTest, EliasEncode) {
  std::vector<bool> code;
  Elias<int>::GammaEncode(1, &code);
  EXPECT_EQ(code, std::vector<bool>({true}));

  code.clear();
  Elias<int>::GammaEncode(2, &code);
  EXPECT_EQ(code, std::vector<bool>({false, true, false}));

  code.clear();
  Elias<int>::DeltaEncode(0, &code);
  EXPECT_EQ(code, std::vector<bool>({true}));

  code.clear();
  Elias<int>::DeltaEncode(1, &code);
  EXPECT_EQ(code, std::vector<bool>({false, true, false, false}));

  // Tests appending.
  code = {true};
  Elias<int>::DeltaEncode(0, &code);
  std::vector<bool> expected = {true, true};
  EXPECT_EQ(code, expected);
}

// Tests Elias BatchDecode and its iterator exhaustion branches.
TEST_F(CompressTest, EliasBatchDecode) {
  std::vector<bool> input;
  std::vector<int> output;

  // Valid input.
  Elias<int>::DeltaEncode(0, &input);
  Elias<int>::DeltaEncode(1, &input);
  Elias<int>::BatchDecode(input, &output);
  ASSERT_EQ(output.size(), 2);
  EXPECT_EQ(output[0], 0);
  EXPECT_EQ(output[1], 1);

  // Iterator exhaustion branches.

  // 1. Truncated during lead_zeros (Gamma part).
  input = {0, 0};
  output.clear();
  Elias<int>::BatchDecode(input, &output);
  EXPECT_TRUE(output.empty());

  // 2. Truncated right after first 1 of Gamma.
  input = {0, 1};
  output.clear();
  Elias<int>::BatchDecode(input, &output);
  EXPECT_TRUE(output.empty());

  // 3. Truncated during reading Gamma value.
  input = {0, 1, 0};
  output.clear();
  Elias<int>::BatchDecode(input, &output);
  EXPECT_TRUE(output.empty());

  // 4. Truncated during reading Delta remainder.
  // Elias Delta of 4 is {0, 1, 1, 0, 1}
  input = {0, 1, 1, 0};
  output.clear();
  Elias<int>::BatchDecode(input, &output);
  EXPECT_TRUE(output.empty());
}

// Tests Elias BatchDecode when it ends exactly at the end of the input vector.
TEST_F(CompressTest, BatchDecodeExactEnd) {
  std::vector<bool> input;
  std::vector<int> output;

  // Elias Delta of 0 is {true}
  input = {true};
  output.clear();
  Elias<int>::BatchDecode(input, &output);
  ASSERT_EQ(output.size(), 1);
  EXPECT_EQ(output[0], 0);

  // Elias Delta of 2 is {false, true, false, true}
  input = {false, true, false, true};
  output.clear();
  Elias<int>::BatchDecode(input, &output);
  ASSERT_EQ(output.size(), 1);
  EXPECT_EQ(output[0], 2);
}

}  // namespace
}  // namespace fst

int main(int argc, char** argv) {
  absl::SetFlag(&FLAGS_fst_verify_properties, true);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
