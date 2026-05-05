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
// Regression test for FST weights.

#include "openfst/lib/weight.h"

#include <cstddef>
#include <cstdint>
#include <utility>

#include "gtest/gtest.h"
#include "absl/flags/flag.h"
#include "absl/log/flags.h"
#include "openfst/lib/expectation-weight.h"
#include "openfst/lib/float-weight.h"
#include "openfst/lib/lexicographic-weight.h"
#include "openfst/lib/pair-weight.h"
#include "openfst/lib/power-weight.h"
#include "openfst/lib/product-weight.h"
#include "openfst/lib/set-weight.h"
#include "openfst/lib/signed-log-weight.h"
#include "openfst/lib/sparse-power-weight.h"
#include "openfst/lib/string-weight.h"
#include "openfst/lib/union-weight.h"
#include "openfst/test/weight-tester.h"

ABSL_FLAG(uint64_t, seed, 403, "random seed");
ABSL_FLAG(int32_t, repeat, 10000, "number of test repetitions");

namespace fst {

// If this test fails, it is possible that x == x will not
// hold for FloatWeight, breaking NaturalLess and probably more.
// To trigger these failures, use g++ -O -m32 -mno-sse.
template <class T>
bool FloatEqualityIsReflexive(T m) {
  // The idea here is that x is spilled to memory, but
  // y remains in an 80-bit register with extra precision,
  // causing it to compare unequal to x.
  volatile T x = 1.111;
  x *= m;

  T y = 1.111;
  y *= m;

  return x == y;
}

template <class Weight>
void TestAdder(int n) {
  Weight sum = Weight::Zero();
  Adder<Weight> adder;
  for (int i = 0; i < n; ++i) {
    sum = Plus(sum, Weight::One());
    adder.Add(Weight::One());
  }
  EXPECT_TRUE(ApproxEqual(sum, adder.Sum()));
}

template <class Weight>
void TestSignedAdder(int n) {
  Weight sum = Weight::Zero();
  Adder<Weight> adder;
  const Weight minus_one = Minus(Weight::Zero(), Weight::One());
  for (int i = 0; i < n; ++i) {
    if (i < n / 4 || i > 3 * n / 4) {
      sum = Plus(sum, Weight::One());
      adder.Add(Weight::One());
    } else {
      sum = Minus(sum, Weight::One());
      adder.Add(minus_one);
    }
  }
  EXPECT_TRUE(ApproxEqual(sum, adder.Sum()));
}

template <typename Weight1, typename Weight2>
void TestWeightConversion(Weight1 w1) {
  // Tests round-trp conversion.
  const WeightConvert<Weight2, Weight1> to_w1_;
  const WeightConvert<Weight1, Weight2> to_w2_;
  Weight2 w2 = to_w2_(w1);
  Weight1 nw1 = to_w1_(w2);
  EXPECT_EQ(w1, nw1);
}

template <typename FromWeight, typename ToWeight>
void TestWeightCopy(FromWeight w) {
  // Test copy constructor.
  const ToWeight to_copied(w);
  const FromWeight roundtrip_copied(to_copied);
  EXPECT_EQ(w, roundtrip_copied);

  // Test copy assign.
  ToWeight to_copy_assigned;
  to_copy_assigned = w;
  EXPECT_EQ(to_copied, to_copy_assigned);

  FromWeight roundtrip_copy_assigned;
  roundtrip_copy_assigned = to_copy_assigned;
  EXPECT_EQ(w, roundtrip_copy_assigned);
}

template <typename FromWeight, typename ToWeight>
void TestWeightMove(FromWeight w) {
  // Assume FromWeight -> FromWeight copy works.
  const FromWeight orig(w);
  ToWeight to_moved(std::move(w));
  const FromWeight roundtrip_moved(std::move(to_moved));
  EXPECT_EQ(orig, roundtrip_moved);

  // Test move assign.
  w = orig;
  ToWeight to_move_assigned;
  to_move_assigned = std::move(w);
  FromWeight roundtrip_move_assigned;
  roundtrip_move_assigned = std::move(to_move_assigned);
  EXPECT_EQ(orig, roundtrip_move_assigned);
}

template <class Weight>
void TestWeightConstructorsFromScalar() {
  // Only test a few of the operations; assumes they are implemented with the
  // same pattern.
  EXPECT_EQ(Weight(2.0f), Weight(2.0f));
  EXPECT_EQ(Weight(2.0f), Weight(2.0));
  EXPECT_EQ(2.0f, Weight(2.0f).Value());
  EXPECT_EQ(2.0, Weight(2.0).Value());
}

template <class Weight>
void TestZeroAnnihilates() {
  EXPECT_EQ(Weight::Zero(), Times(Weight::Zero(), Weight(3.0f)));
  EXPECT_EQ(Weight::Zero(), Times(Weight::Zero(), Weight(3.0)));
  EXPECT_EQ(Weight::Zero(), Times(Weight(3.0), Weight::Zero()));
}

template <class Weight>
void TestZeroIsAdditiveIdentity() {
  EXPECT_EQ(Weight(3.0), Plus(Weight::Zero(), Weight(3.0f)));
  EXPECT_EQ(Weight(3.0), Plus(Weight::Zero(), Weight(3.0)));
  EXPECT_EQ(Weight(3.0), Plus(Weight(3.0), Weight::Zero()));
}

void TestPowerWeightGetSetValue() {
  PowerWeight<LogWeight, 3> w;
  // LogWeight has unspecified initial value, so don't check it.
  w.SetValue(0, LogWeight(2));
  w.SetValue(1, LogWeight(3));
  EXPECT_EQ(LogWeight(2), w.Value(0));
  EXPECT_EQ(LogWeight(3), w.Value(1));
}

void TestSparsePowerWeightGetSetValue() {
  const LogWeight default_value(17);
  SparsePowerWeight<LogWeight> w;
  w.SetDefaultValue(default_value);

  // All gets should be the default.
  EXPECT_EQ(default_value, w.Value(0));
  EXPECT_EQ(default_value, w.Value(100));

  // First set should fill first_.
  w.SetValue(10, LogWeight(10));
  EXPECT_EQ(LogWeight(10), w.Value(10));
  w.SetValue(10, LogWeight(20));
  EXPECT_EQ(LogWeight(20), w.Value(10));

  // Add a smaller index.
  w.SetValue(5, LogWeight(5));
  EXPECT_EQ(LogWeight(5), w.Value(5));
  EXPECT_EQ(LogWeight(20), w.Value(10));

  // Add some larger indices.
  w.SetValue(30, LogWeight(30));
  EXPECT_EQ(LogWeight(5), w.Value(5));
  EXPECT_EQ(LogWeight(20), w.Value(10));
  EXPECT_EQ(LogWeight(30), w.Value(30));

  w.SetValue(29, LogWeight(29));
  EXPECT_EQ(LogWeight(5), w.Value(5));
  EXPECT_EQ(LogWeight(20), w.Value(10));
  EXPECT_EQ(LogWeight(29), w.Value(29));
  EXPECT_EQ(LogWeight(30), w.Value(30));

  w.SetValue(31, LogWeight(31));
  EXPECT_EQ(LogWeight(5), w.Value(5));
  EXPECT_EQ(LogWeight(20), w.Value(10));
  EXPECT_EQ(LogWeight(29), w.Value(29));
  EXPECT_EQ(LogWeight(30), w.Value(30));
  EXPECT_EQ(LogWeight(31), w.Value(31));

  // Replace a value.
  w.SetValue(30, LogWeight(60));
  EXPECT_EQ(LogWeight(60), w.Value(30));

  // Replace a value with the default.
  EXPECT_EQ(5, w.Size());
  w.SetValue(30, default_value);
  EXPECT_EQ(default_value, w.Value(30));
  EXPECT_EQ(4, w.Size());

  // Replace lowest index by the default value.
  w.SetValue(5, default_value);
  EXPECT_EQ(default_value, w.Value(5));
  EXPECT_EQ(3, w.Size());

  // Clear out everything.
  w.SetValue(31, default_value);
  w.SetValue(29, default_value);
  w.SetValue(10, default_value);
  EXPECT_EQ(0, w.Size());

  EXPECT_EQ(default_value, w.Value(5));
  EXPECT_EQ(default_value, w.Value(10));
  EXPECT_EQ(default_value, w.Value(29));
  EXPECT_EQ(default_value, w.Value(30));
  EXPECT_EQ(default_value, w.Value(31));
}

void TestFloatEqualityIsReflexive() {
  // Use a volatile test_value to avoid excessive inlining / optimization
  // breaking what we're trying to test.
  volatile double test_value = 1.1;
  EXPECT_TRUE(FloatEqualityIsReflexive(static_cast<float>(test_value)));
  EXPECT_TRUE(FloatEqualityIsReflexive(test_value));
}

struct UnionWeightOptions {
  // These are used, but getting a `-Wunused-local-typedef` false-positive.
  // Suppress with `[[maybe_unused]]`.
  using Compare [[maybe_unused]] = NaturalLess<TropicalWeight>;
  using ReverseOptions [[maybe_unused]] = UnionWeightOptions;

  struct Merge {
    TropicalWeight operator()(const TropicalWeight& w1,
                              const TropicalWeight& w2) const {
      return w1;
    }
  };
};

using LeftStringWeight = StringWeight<int>;
using RightStringWeight = StringWeight<int, STRING_RIGHT>;
using IUSetWeight = SetWeight<int, SET_INTERSECT_UNION>;
using UISetWeight = SetWeight<int, SET_UNION_INTERSECT>;
using BoolSetWeight = SetWeight<int, SET_BOOLEAN>;
using TropicalGallicWeight = GallicWeight<int, TropicalWeight>;
using TropicalGenGallicWeight = GallicWeight<int, TropicalWeight, GALLIC>;
using TropicalProductWeight = ProductWeight<TropicalWeight, TropicalWeight>;
using TropicalLexicographicWeight =
    LexicographicWeight<TropicalWeight, TropicalWeight>;
using TropicalCubeWeight = PowerWeight<TropicalWeight, 3>;
using FirstNestedProductWeight =
    ProductWeight<TropicalProductWeight, TropicalWeight>;
using SecondNestedProductWeight =
    ProductWeight<TropicalWeight, TropicalProductWeight>;
using NestedProductCubeWeight = PowerWeight<FirstNestedProductWeight, 3>;
using SparseNestedProductCubeWeight =
    SparsePowerWeight<FirstNestedProductWeight, size_t>;
using LogSparsePowerWeight = SparsePowerWeight<LogWeight, size_t>;
using LogLogExpectationWeight = ExpectationWeight<LogWeight, LogWeight>;
using RealRealExpectationWeight = ExpectationWeight<RealWeight, RealWeight>;
using LogLogSparseExpectationWeight =
    ExpectationWeight<LogWeight, LogSparsePowerWeight>;
using TropicalUnionWeight = UnionWeight<TropicalWeight, UnionWeightOptions>;

// Trait specializations are only needed if one of the functions needs to
// be changed.
template <>
struct WeightTestTraits<TropicalGenGallicWeight> {
  static WeightGenerate<TropicalGenGallicWeight> Generator(uint64_t seed) {
    // TODO: Document why allow_zero is false.
    return WeightGenerate<TropicalGenGallicWeight>(seed, /*allow_zero=*/false);
  }
  static bool IoRequiresParens() { return true; }
};

template <>
struct WeightTestTraits<FirstNestedProductWeight> {
  static WeightGenerate<FirstNestedProductWeight> Generator(uint64_t seed) {
    return WeightGenerate<FirstNestedProductWeight>(seed);
  }
  static bool IoRequiresParens() { return true; }
};

template <>
struct WeightTestTraits<NestedProductCubeWeight> {
  static WeightGenerate<NestedProductCubeWeight> Generator(uint64_t seed) {
    return WeightGenerate<NestedProductCubeWeight>(seed);
  }
  static bool IoRequiresParens() { return true; }
};

template <>
struct WeightTestTraits<SparseNestedProductCubeWeight> {
  static WeightGenerate<SparseNestedProductCubeWeight> Generator(
      uint64_t seed) {
    return WeightGenerate<SparseNestedProductCubeWeight>(seed);
  }
  static bool IoRequiresParens() { return true; }
};

template <>
struct WeightTestTraits<RealRealExpectationWeight> {
  static WeightGenerate<RealRealExpectationWeight> Generator(uint64_t seed) {
    return WeightGenerate<RealRealExpectationWeight>(seed);
  }
  static bool IoRequiresParens() { return true; }
};

using BaseWeights =
    ::testing::Types<TropicalWeightTpl<float>, TropicalWeightTpl<double>,
                     LogWeightTpl<float>, LogWeightTpl<double>,
                     RealWeightTpl<float>, RealWeightTpl<double>,
                     MinMaxWeightTpl<float>, MinMaxWeightTpl<double>,
                     SignedLogWeightTpl<float>, SignedLogWeightTpl<double>>;

using SpecialWeights = ::testing::Types<
    LeftStringWeight, RightStringWeight, IUSetWeight, UISetWeight,
    BoolSetWeight, TropicalGallicWeight, TropicalGenGallicWeight,
    TropicalProductWeight, TropicalLexicographicWeight, TropicalCubeWeight,
    FirstNestedProductWeight, SecondNestedProductWeight,
    NestedProductCubeWeight, SparseNestedProductCubeWeight,
    LogSparsePowerWeight, LogLogExpectationWeight, RealRealExpectationWeight,
    LogLogSparseExpectationWeight, TropicalUnionWeight>;

// The extra "," is needed to avoid an empty "..." param in C++17.
INSTANTIATE_TYPED_TEST_SUITE_P(Base, WeightTest, BaseWeights, );
INSTANTIATE_TYPED_TEST_SUITE_P(Special, WeightTest, SpecialWeights, );

TEST(WeightMiscTest, TypeNames) {
  // Makes sure type names for templated weights are consistent.
  EXPECT_EQ(TropicalWeight::Type(), "tropical");
  EXPECT_NE(TropicalWeightTpl<double>::Type(),
            TropicalWeightTpl<float>::Type());
  EXPECT_EQ(LogWeight::Type(), "log");
  EXPECT_NE(LogWeightTpl<double>::Type(), LogWeightTpl<float>::Type());
  EXPECT_EQ(RealWeight::Type(), "real");
  EXPECT_NE(RealWeightTpl<double>::Type(), RealWeightTpl<float>::Type());
  TropicalWeightTpl<double> w(2.0L);
  TropicalWeight tw(2.0F);
  EXPECT_EQ(w.Value(), tw.Value());
}

// It could be cleaner to split this up, but this test case is very fast.
TEST(WeightMiscTest, FreeFunctions) {
  TestAdder<TropicalWeight>(1000);
  TestAdder<LogWeight>(1000);
  TestAdder<RealWeight>(1000);

  TestSignedAdder<SignedLogWeight>(1000);

  TestWeightConstructorsFromScalar<TropicalWeight>();
  TestWeightConstructorsFromScalar<LogWeight>();
  TestWeightConstructorsFromScalar<RealWeight>();
  TestWeightConstructorsFromScalar<MinMaxWeight>();

  TestZeroAnnihilates<TropicalWeight>();
  TestZeroAnnihilates<LogWeight>();
  TestZeroAnnihilates<RealWeight>();
  TestZeroAnnihilates<MinMaxWeight>();

  TestZeroIsAdditiveIdentity<TropicalWeight>();
  TestZeroIsAdditiveIdentity<LogWeight>();
  TestZeroIsAdditiveIdentity<RealWeight>();
  TestZeroIsAdditiveIdentity<MinMaxWeight>();

  TestWeightConversion<TropicalWeight, LogWeight>(TropicalWeight(2.0));

  WeightGenerate<IUSetWeight> iu_set_generate(absl::GetFlag(FLAGS_seed));
  WeightGenerate<UISetWeight> ui_set_generate(absl::GetFlag(FLAGS_seed));
  WeightGenerate<BoolSetWeight> bool_set_generate(absl::GetFlag(FLAGS_seed));

  TestWeightConversion<IUSetWeight, UISetWeight>(iu_set_generate());

  TestWeightCopy<IUSetWeight, UISetWeight>(iu_set_generate());
  TestWeightCopy<IUSetWeight, BoolSetWeight>(iu_set_generate());
  TestWeightCopy<UISetWeight, IUSetWeight>(ui_set_generate());
  TestWeightCopy<UISetWeight, BoolSetWeight>(ui_set_generate());
  TestWeightCopy<BoolSetWeight, IUSetWeight>(bool_set_generate());
  TestWeightCopy<BoolSetWeight, UISetWeight>(bool_set_generate());

  TestWeightMove<IUSetWeight, UISetWeight>(iu_set_generate());
  TestWeightMove<IUSetWeight, BoolSetWeight>(iu_set_generate());
  TestWeightMove<UISetWeight, IUSetWeight>(ui_set_generate());
  TestWeightMove<UISetWeight, BoolSetWeight>(ui_set_generate());
  TestWeightMove<BoolSetWeight, IUSetWeight>(bool_set_generate());
  TestWeightMove<BoolSetWeight, UISetWeight>(bool_set_generate());

  TestPowerWeightGetSetValue();
  TestSparsePowerWeightGetSetValue();

  TestFloatEqualityIsReflexive();
}

}  // namespace fst

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
