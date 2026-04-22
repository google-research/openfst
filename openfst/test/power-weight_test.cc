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

// Tests for PowerWeight and SparsePowerWeight.

#include "openfst/lib/power-weight.h"

#include <cstdint>
#include <limits>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "openfst/compat/seed_sequences.h"
#include "absl/random/random.h"
#include "openfst/lib/float-weight.h"
#include "openfst/lib/sparse-power-weight.h"
#include "openfst/lib/weight.h"
#include "openfst/test/power-weight-util.h"
#include "openfst/test/weight-tester.h"

namespace fst {
namespace {

using ::fst::test::CreateWeight;
using ::fst::test::ToVector;
using ::testing::ElementsAre;
using ::testing::Pair;

// Get test uses index 5, so make PowerWeight size 6.
using PowerWeightTypes =
    testing::Types<SparsePowerWeight<TropicalWeight, int32_t>,
                   PowerWeight<TropicalWeight, 6>>;

template <typename PowerWeightT>
class GeneratePowerWeightTest : public testing::Test {
 public:
  using Weight = PowerWeightT;
};
TYPED_TEST_SUITE(GeneratePowerWeightTest, PowerWeightTypes);

TYPED_TEST(GeneratePowerWeightTest, WeightTester) {
  using Weight = typename TestFixture::Weight;
  using Generator = WeightGenerate<Weight>;
  using WeightTester = WeightTester<Weight>;
  absl::BitGen bit_gen(fst::MakeTaggedSeedSeq(
      "WEIGHT_TESTER"));
  WeightTester tester((Generator()));
  tester.Test(bit_gen, 2000 /* iterations */);
}

template <typename PowerWeightT>
class GetPowerWeightComponentTest : public testing::Test {
 public:
  using Weight = PowerWeightT;
};
TYPED_TEST_SUITE(GetPowerWeightComponentTest, PowerWeightTypes);

TYPED_TEST(GetPowerWeightComponentTest, Empty) {
  using Weight = typename TestFixture::Weight;
  const auto w = CreateWeight<Weight>({});
  for (int i = 0; i < 4; ++i) EXPECT_EQ(TropicalWeight::Zero(), w.Value(i));
}

TYPED_TEST(GetPowerWeightComponentTest, Found) {
  using Weight = typename TestFixture::Weight;
  const auto w = CreateWeight<Weight>({{1, 1.23}, {4, 56.7}});
  EXPECT_FLOAT_EQ(1.23, w.Value(1).Value());
  EXPECT_FLOAT_EQ(56.7, w.Value(4).Value());
}

TYPED_TEST(GetPowerWeightComponentTest, NotFound) {
  using Weight = typename TestFixture::Weight;
  constexpr bool is_sparse =
      std::is_same_v<Weight, SparsePowerWeight<TropicalWeight, int32_t>>;
  const auto w = CreateWeight<Weight>({{1, 1.23}, {4, 56.7}});
  // Only SparsePowerWeight supports an infinite range.
  if (is_sparse) {
    EXPECT_EQ(TropicalWeight::Zero(), w.Value(-1));
  }
  EXPECT_EQ(TropicalWeight::Zero(), w.Value(0));
  EXPECT_EQ(TropicalWeight::Zero(), w.Value(2));
  EXPECT_EQ(TropicalWeight::Zero(), w.Value(3));
  EXPECT_EQ(TropicalWeight::Zero(), w.Value(5));
  if (is_sparse) {
    EXPECT_EQ(TropicalWeight::Zero(),
              w.Value(std::numeric_limits<int32_t>::max()));
  }
}

template <typename PowerWeightT>
class SetPowerWeightComponentTest : public testing::Test {
 public:
  using Weight = PowerWeightT;
};
TYPED_TEST_SUITE(SetPowerWeightComponentTest, PowerWeightTypes);

TYPED_TEST(SetPowerWeightComponentTest, Empty) {
  using Weight = typename TestFixture::Weight;
  auto w = CreateWeight<Weight>({});
  w.SetValue(3, TropicalWeight(4.5));
  EXPECT_THAT(ToVector(w), ElementsAre(Pair(3, 4.5)));
}

TYPED_TEST(SetPowerWeightComponentTest, InsertBefore) {
  using Weight = typename TestFixture::Weight;
  auto w = CreateWeight<Weight>({{2, 2.34}});
  w.SetValue(1, TropicalWeight(1.23));
  EXPECT_THAT(ToVector(w), ElementsAre(Pair(1, 1.23), Pair(2, 2.34)));
}

TYPED_TEST(SetPowerWeightComponentTest, InsertAfter) {
  using Weight = typename TestFixture::Weight;
  auto w = CreateWeight<Weight>({{2, 2.34}});
  w.SetValue(3, TropicalWeight(3.45));
  EXPECT_THAT(ToVector(w), ElementsAre(Pair(2, 2.34), Pair(3, 3.45)));
}

TYPED_TEST(SetPowerWeightComponentTest, Replace) {
  using Weight = typename TestFixture::Weight;
  auto w = CreateWeight<Weight>({{1, 1.23}, {2, 2.34}, {3, 3.45}});
  w.SetValue(2, TropicalWeight(5.67));
  EXPECT_THAT(ToVector(w),
              ElementsAre(Pair(1, 1.23), Pair(2, 5.67), Pair(3, 3.45)));
}

TYPED_TEST(SetPowerWeightComponentTest, SetInOne) {
  auto w = TestFixture::Weight::One();
  w.SetValue(1, TropicalWeight(1.23));
  EXPECT_THAT(ToVector(w, TropicalWeight::One()), ElementsAre(Pair(1, 1.23)));
}

TYPED_TEST(SetPowerWeightComponentTest, SetInZero) {
  auto w = TestFixture::Weight::Zero();
  w.SetValue(1, TropicalWeight(1.23));
  EXPECT_THAT(ToVector(w), ElementsAre(Pair(1, 1.23)));
}

TYPED_TEST(SetPowerWeightComponentTest, Remove) {
  using Weight = typename TestFixture::Weight;
  auto w = CreateWeight<Weight>({{1, 1.23}, {2, 2.34}, {3, 3.45}});
  w.SetValue(2, TropicalWeight::Zero());
  EXPECT_THAT(ToVector(w), ElementsAre(Pair(1, 1.23), Pair(3, 3.45)));
}

}  // namespace
}  // namespace fst
