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
// Unit test for RandGen.

#include "openfst/lib/randgen.h"

#include <cstdint>
#include <limits>
#include <memory>
#include <string>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "openfst/compat/seed_sequences.h"
#include "absl/random/random.h"
#include "openfst/compat/seed_sequences.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/equal.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/lib/verify.h"

namespace fst {
namespace {

using Arc = StdArc;

class RandGenTest : public testing::Test {
 protected:
  void SetUp() override {
    const std::string path =
        JoinPath(std::string("."),
                       "openfst/test/testdata/randgen");
    const std::string randgen1_name = JoinPath(path, "r1.fst");
    const std::string randgen2_name = JoinPath(path, "r2.fst");
    const std::string randgen3_name = JoinPath(path, "r3.fst");
    const std::string randgen4_name = JoinPath(path, "r4.fst");

    rfst1_.reset(VectorFst<Arc>::Read(randgen1_name));
    rfst2_.reset(VectorFst<Arc>::Read(randgen2_name));
    rfst3_.reset(VectorFst<Arc>::Read(randgen3_name));
    rfst4_.reset(VectorFst<Arc>::Read(randgen4_name));
  }

  std::unique_ptr<const VectorFst<Arc>> rfst1_;
  std::unique_ptr<const VectorFst<Arc>> rfst2_;
  std::unique_ptr<const VectorFst<Arc>> rfst3_;
  std::unique_ptr<const VectorFst<Arc>> rfst4_;
};

TEST_F(RandGenTest, UniformRandGenEmpty) {
  VectorFst<Arc> empty_fst;

  VectorFst<Arc> path;
  absl::BitGen bit_gen(
      fst::MakeTaggedSeedSeq("EMPTY"));
  RandGen(empty_fst, &path, bit_gen);

  ASSERT_TRUE(Verify(path));
  ASSERT_TRUE(Equal(empty_fst, path));
}

TEST_F(RandGenTest, UniformRandGenNonEmpty) {
  // Note: The seed here must match the seed used in the corresponding
  // Python test (pywrapfst_test.py) and shell test (randgen-main_test.sh).
  std::mt19937_64 bit_gen(2);
  const UniformArcSelector<Arc> uniform_selector;
  const RandGenOptions<UniformArcSelector<Arc>> opts(uniform_selector);

  VectorFst<Arc> path;
  RandGen(*rfst1_, &path, opts, bit_gen);

  ASSERT_TRUE(Verify(path));
  ASSERT_TRUE(Equal(*rfst2_, path));
}

TEST_F(RandGenTest, LogRandGen) {
  VectorFst<Arc> path;

  std::mt19937_64 bit_gen;
  const LogProbArcSelector<Arc> std_selector;
  const RandGenOptions<LogProbArcSelector<Arc>> opts(std_selector);
  RandGen(*rfst1_, &path, opts, bit_gen);
  ASSERT_TRUE(Verify(path));
  ASSERT_TRUE(Equal(*rfst3_, path));
}

TEST_F(RandGenTest, FastLogRandGen) {
  VectorFst<Arc> path;

  std::mt19937_64 bit_gen;
  const FastLogProbArcSelector<Arc> fastlog_selector;
  const RandGenOptions<FastLogProbArcSelector<Arc>> opts(fastlog_selector);
  RandGen(*rfst1_, &path, opts, bit_gen);
  ASSERT_TRUE(Verify(path));
  ASSERT_TRUE(Equal(*rfst3_, path));
}

TEST_F(RandGenTest, WeightedRandGen) {
  VectorFst<Arc> path;

  std::mt19937_64 bit_gen;
  const LogProbArcSelector<Arc> std_selector;
  const RandGenOptions<LogProbArcSelector<Arc>> opts(
      std_selector,
      /*max_length=*/std::numeric_limits<int32_t>::max(),
      /*npath=*/2, /*weighted=*/true);
  RandGen(*rfst1_, &path, opts, bit_gen);
  ASSERT_TRUE(Verify(path));
  ASSERT_TRUE(Equal(*rfst4_, path));
}

template <typename Selector>
class RandGenLogTest : public testing::Test {};

using RandGenLogTestTypes =
    testing::Types<LogProbArcSelector<Arc>, FastLogProbArcSelector<Arc>>;
TYPED_TEST_SUITE(RandGenLogTest, RandGenLogTestTypes);

TYPED_TEST(RandGenLogTest, HighWeights) {
  auto create_fst = [](float weight) {
    VectorFst<Arc> fst;
    while (fst.NumStates() < 3) fst.AddState();
    fst.SetStart(0);
    fst.SetFinal(2);
    fst.AddArc(0, Arc(1, 1, weight, 1));
    fst.AddArc(1, Arc(2, 2, weight, 2));
    return fst;
  };

  VectorFst<Arc> path;
  absl::BitGen bit_gen(fst::MakeTaggedSeedSeq(
      "HIGH_WEIGHTS"));
  const TypeParam fastlog_selector;
  const RandGenOptions<TypeParam> opts(fastlog_selector);
  RandGen(create_fst(1000.0f), &path, opts, bit_gen);
  ASSERT_TRUE(Verify(path));
  ASSERT_TRUE(Equal(create_fst(0.0f), path));
}

}  // namespace
}  // namespace fst
