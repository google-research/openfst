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

#include "openfst/compat/init.h"
#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "absl/flags/flag.h"
#include "absl/log/flags.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/isomorphic.h"
#include "openfst/lib/mutable-fst.h"
#include "openfst/lib/test-properties.h"
#include "openfst/lib/vector-fst.h"

using ::fst::Isomorphic;
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

}  // namespace
}  // namespace fst

int main(int argc, char** argv) {
  absl::SetFlag(&FLAGS_fst_verify_properties, true);
  fst::InitOpenFst(argv[0], &argc, &argv, true);
  return RUN_ALL_TESTS();
}
