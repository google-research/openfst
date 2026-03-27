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

// quantize_weights_test.h
//
// Unit tests for quantizing weights.

#include "openfst/extensions/compress/quantize_weights.h"

#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "absl/flags/flag.h"
#include "absl/log/flags.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/equal.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/test-properties.h"
#include "openfst/lib/vector-fst.h"

namespace fst {
namespace {

using Arc = fst::StdArc;

constexpr char kTestDataDir[] =
    "openfst/extensions/compress/testdata";

// Test class.
class QuantizeWeightsTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Input fst.
    const std::string input_fst_name = JoinPath(
        std::string("."), kTestDataDir, "quantize_weights_input.fst");
    input_fst_.reset(Fst<Arc>::Read(input_fst_name));

    // Uniform quantization.
    const std::string uniformly_quantized_fst_name = JoinPath(
        std::string("."), kTestDataDir, "uniform_quantization.fst");
    uniformly_quantized_fst_.reset(
        Fst<Arc>::Read(uniformly_quantized_fst_name));

    // Uniform quantization to the range of the input FST.
    const std::string uniformly_quantized_input_range_fst_name =
        JoinPath(std::string("."), kTestDataDir,
                       "uniform_quantization_input_range.fst");
    uniformly_quantized_input_range_fst_.reset(
        Fst<Arc>::Read(uniformly_quantized_input_range_fst_name));

    // k-median quantization.
    const std::string kmedian_quantized_fst_name = JoinPath(
        std::string("."), kTestDataDir, "kmedian_quantization.fst");
    kmedian_quantized_fst_.reset(Fst<Arc>::Read(kmedian_quantized_fst_name));

    // Clip_weights_only.
    const std::string clip_weights_only_quantized_fst_name =
        JoinPath(std::string("."), kTestDataDir, "clip_only.fst");
    clip_only_fst_.reset(Fst<Arc>::Read(clip_weights_only_quantized_fst_name));

    // quantize_zero_weight.
    const std::string quantize_zero_weight_fst_name = JoinPath(
        std::string("."), kTestDataDir, "quantize_zero_weight.fst");
    quantize_zero_weight_fst_.reset(
        Fst<Arc>::Read(quantize_zero_weight_fst_name));
  }

  std::unique_ptr<const Fst<Arc>> input_fst_;
  std::unique_ptr<const Fst<Arc>> uniformly_quantized_fst_;
  std::unique_ptr<const Fst<Arc>> uniformly_quantized_input_range_fst_;
  std::unique_ptr<const Fst<Arc>> kmedian_quantized_fst_;
  std::unique_ptr<const Fst<Arc>> clip_only_fst_;
  std::unique_ptr<const Fst<Arc>> quantize_zero_weight_fst_;
};

TEST_F(QuantizeWeightsTest, MinMaxFiniteWeightValues_EmptyFst) {
  const auto min_max = GetMinMaxFiniteWeightValues(StdVectorFst());
  EXPECT_EQ(min_max.first, std::numeric_limits<double>::infinity());
  EXPECT_EQ(min_max.second, -std::numeric_limits<double>::infinity());
}

TEST_F(QuantizeWeightsTest, MinMaxFiniteWeightValues) {
  const auto min_max = GetMinMaxFiniteWeightValues(*input_fst_);
  // These values are from fstprint.
  EXPECT_NEAR(min_max.first, -0.300104588, 0.5e-9);
  EXPECT_DOUBLE_EQ(min_max.second, 99.0);
}

// Checks uniform quantization.
TEST_F(QuantizeWeightsTest, UniformQuantizationTest) {
  std::vector<double> boundaries;
  ComputeUniformQuantization(/*num_levels=*/101, /*min_level=*/-10.0,
                             /*max_level=*/10.0, &boundaries);
  VectorFst<Arc> quantized_fst(*input_fst_);
  QuantizeWeights<StdArc>(boundaries, &quantized_fst);
  EXPECT_TRUE(Equal(quantized_fst, *uniformly_quantized_fst_));
}

// Checks uniform to range of input FST.
TEST_F(QuantizeWeightsTest, UniformQuantizationInputRangeTest) {
  const auto min_max = GetMinMaxFiniteWeightValues(*input_fst_);

  std::vector<double> boundaries;
  ComputeUniformQuantization(
      /*num_levels=*/100, /*min_level=*/min_max.first,
      /*max_level=*/min_max.second, &boundaries);
  VectorFst<Arc> quantized_fst(*input_fst_);
  QuantizeWeights<StdArc>(boundaries, &quantized_fst);
  // From the MinMaxFiniteWeightValues test, we see the min is ~-0.3,
  // and the max is 99. With 101 steps, the increment is ~1. The weights
  // are in fact near -0.3, 0.7, 2.7, etc.
  EXPECT_TRUE(Equal(quantized_fst, *uniformly_quantized_input_range_fst_));
}

// Checks log-uniform quantization.
TEST_F(QuantizeWeightsTest, KMedianQuantizationTest) {
  std::vector<double> boundaries;
  // The input FST has ~40 weights with 10 different values, so 4 clusters
  // should provide a reasonable test of k-medians.
  ComputeWeightedkMedianQuantization(*input_fst_,
                                     /*num_levels=*/4, &boundaries);
  VectorFst<Arc> quantized_fst(*input_fst_);
  QuantizeWeights<StdArc>(boundaries, &quantized_fst);
  EXPECT_TRUE(Equal(quantized_fst, *kmedian_quantized_fst_));
}

// Checks clip_weights_only.
TEST_F(QuantizeWeightsTest, ClipWeightsOnlyTest) {
  std::vector<double> boundaries;
  ComputeUniformQuantization(/*num_levels=*/101, /*min_level=*/-10.0,
                             /*max_level=*/10.0, &boundaries);
  VectorFst<Arc> quantized_fst(*input_fst_);
  QuantizeWeights<StdArc>(boundaries, &quantized_fst,
                          /*quantize_zero_weight=*/false,
                          /*clip_weights_only=*/true);
  EXPECT_TRUE(Equal(quantized_fst, *clip_only_fst_));
}

// Checks uniform quantization with quantize_zero_weight.
TEST_F(QuantizeWeightsTest, QuantizeWeightsZeroTest) {
  std::vector<double> boundaries;
  ComputeUniformQuantization(/*num_levels=*/101, /*min_level=*/-10.0,
                             /*max_level=*/10.0, &boundaries);
  VectorFst<Arc> quantized_fst(*input_fst_);
  QuantizeWeights<StdArc>(boundaries, &quantized_fst,
                          /*quantize_zero_weight=*/true,
                          /*clip_weights_only=*/false);
  EXPECT_TRUE(Equal(quantized_fst, *quantize_zero_weight_fst_));
}

}  // namespace
}  // namespace fst

int main(int argc, char** argv) {
  absl::SetFlag(&FLAGS_fst_verify_properties, true);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
