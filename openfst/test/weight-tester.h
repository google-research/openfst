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
// Utility class for regression testing of FST weights.

#ifndef OPENFST_TEST_WEIGHT_TESTER_H_
#define OPENFST_TEST_WEIGHT_TESTER_H_

#include <cstdint>
#include <sstream>
#include <utility>

#include "gtest/gtest.h"
#include "absl/flags/declare.h"
#include "absl/flags/flag.h"
#include "absl/log/log.h"
#include "openfst/lib/weight.h"

// These flags must be defined in the *test.cc including this file.
ABSL_DECLARE_FLAG(uint64_t, seed);
ABSL_DECLARE_FLAG(int32_t, repeat);

namespace fst {

// Traits class to control test behavior.  This may be specialized for weights
// that require different implementations.
template <class Weight>
struct WeightTestTraits {
  static WeightGenerate<Weight> Generator(uint64_t seed) {
    return WeightGenerate<Weight>(seed);
  }
  static bool IoRequiresParens() { return false; }
};

template <class Weight>
class WeightTest : public ::testing::Test {
 public:
  using WeightGenerator = WeightGenerate<Weight>;

  WeightTest()
      : seed_(absl::GetFlag(FLAGS_seed)),
        generate_(WeightTestTraits<Weight>::Generator(seed_)) {}

 protected:
  const uint64_t seed_;
  WeightGenerator generate_;
};

TYPED_TEST_SUITE_P(WeightTest);

// Tests (Plus, Times, Zero, One) defines a commutative semiring.
TYPED_TEST_P(WeightTest, Semiring) {
  using Weight = TypeParam;
  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    const Weight w1(this->generate_());
    const Weight w2(this->generate_());
    const Weight w3(this->generate_());

    VLOG(1) << "weight type = " << Weight::Type();
    VLOG(1) << "w1 = " << w1;
    VLOG(1) << "w2 = " << w2;
    VLOG(1) << "w3 = " << w3;

    // Checks that the operations are closed.
    EXPECT_TRUE(Plus(w1, w2).Member());
    EXPECT_TRUE(Times(w1, w2).Member());

    // Checks that the operations are associative.
    EXPECT_TRUE(ApproxEqual(Plus(w1, Plus(w2, w3)), Plus(Plus(w1, w2), w3)));
    EXPECT_TRUE(
        ApproxEqual(Times(w1, Times(w2, w3)), Times(Times(w1, w2), w3)));

    // Checks the identity elements.
    EXPECT_EQ(Plus(w1, Weight::Zero()), w1);
    EXPECT_EQ(Plus(Weight::Zero(), w1), w1);
    EXPECT_EQ(Times(w1, Weight::One()), w1);
    EXPECT_EQ(Times(Weight::One(), w1), w1);

    // Check the no weight element.
    EXPECT_FALSE(Weight::NoWeight().Member());
    EXPECT_FALSE(Plus(w1, Weight::NoWeight()).Member());
    EXPECT_FALSE(Plus(Weight::NoWeight(), w1).Member());
    EXPECT_FALSE(Times(w1, Weight::NoWeight()).Member());
    EXPECT_FALSE(Times(Weight::NoWeight(), w1).Member());

    // Checks that the operations commute.
    EXPECT_TRUE(ApproxEqual(Plus(w1, w2), Plus(w2, w1)));

    if (Weight::Properties() & kCommutative) {
      EXPECT_TRUE(ApproxEqual(Times(w1, w2), Times(w2, w1)));
    }

    // Checks Zero() is the annihilator.
    EXPECT_EQ(Times(w1, Weight::Zero()), Weight::Zero());
    EXPECT_EQ(Times(Weight::Zero(), w1), Weight::Zero());

    // Check Power(w, 0) is Weight::One()
    EXPECT_EQ(Power(w1, 0), Weight::One());

    // Check Power(w, 1) is w
    EXPECT_EQ(Power(w1, 1), w1);

    // Check Power(w, 2) is Times(w, w)
    EXPECT_EQ(Power(w1, 2), Times(w1, w1));

    // Check Power(w, 3) is Times(w, Times(w, w))
    EXPECT_EQ(Power(w1, 3), Times(w1, Times(w1, w1)));

    // Checks distributivity.
    if (Weight::Properties() & kLeftSemiring) {
      EXPECT_TRUE(ApproxEqual(Times(w1, Plus(w2, w3)),
                              Plus(Times(w1, w2), Times(w1, w3))));
    }
    if (Weight::Properties() & kRightSemiring) {
      EXPECT_TRUE(ApproxEqual(Times(Plus(w1, w2), w3),
                              Plus(Times(w1, w3), Times(w2, w3))));
    }

    if (Weight::Properties() & kIdempotent) {
      EXPECT_EQ(Plus(w1, w1), w1);
    }

    if (Weight::Properties() & kPath) {
      EXPECT_TRUE(Plus(w1, w2) == w1 || Plus(w1, w2) == w2);
    }

    // Ensure weights form a left or right semiring.
    EXPECT_TRUE(Weight::Properties() & (kLeftSemiring | kRightSemiring));

    // Check when Times() is commutative that it is marked as a semiring.
    if (Weight::Properties() & kCommutative) {
      EXPECT_TRUE(Weight::Properties() & kSemiring);
    }
  }
}

TYPED_TEST_P(WeightTest, Division) {
  using Weight = TypeParam;
  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    const Weight w1(this->generate_());
    const Weight w2(this->generate_());
    Weight p = Times(w1, w2);
    VLOG(1) << "TestDivision: p = " << p;

    if (Weight::Properties() & kLeftSemiring) {
      const Weight d = Divide(p, w1, DIVIDE_LEFT);
      if (d.Member()) {
        EXPECT_TRUE(ApproxEqual(p, Times(w1, d)));
      }
      EXPECT_FALSE(Divide(w1, Weight::NoWeight(), DIVIDE_LEFT).Member());
      EXPECT_FALSE(Divide(Weight::NoWeight(), w1, DIVIDE_LEFT).Member());
    }

    if (Weight::Properties() & kRightSemiring) {
      const Weight d = Divide(p, w2, DIVIDE_RIGHT);
      if (d.Member()) {
        EXPECT_TRUE(ApproxEqual(p, Times(d, w2)));
      }
      EXPECT_FALSE(Divide(w1, Weight::NoWeight(), DIVIDE_RIGHT).Member());
      EXPECT_FALSE(Divide(Weight::NoWeight(), w1, DIVIDE_RIGHT).Member());
    }

    if (Weight::Properties() & kCommutative) {
      const Weight d1 = Divide(p, w1, DIVIDE_ANY);
      if (d1.Member()) {
        EXPECT_TRUE(ApproxEqual(p, Times(d1, w1)));
      }
      const Weight d2 = Divide(p, w2, DIVIDE_ANY);
      if (d2.Member()) {
        EXPECT_TRUE(ApproxEqual(p, Times(w2, d2)));
      }
    }
  }
}

TYPED_TEST_P(WeightTest, Reverse) {
  using Weight = TypeParam;
  using ReverseWeight = typename Weight::ReverseWeight;
  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    const Weight w1(this->generate_());
    const Weight w2(this->generate_());

    const ReverseWeight rw1 = w1.Reverse();
    const ReverseWeight rw2 = w2.Reverse();

    EXPECT_EQ(rw1.Reverse(), w1);
    EXPECT_EQ(Plus(w1, w2).Reverse(), Plus(rw1, rw2));
    EXPECT_EQ(Times(w1, w2).Reverse(), Times(rw2, rw1));
  }
}

TYPED_TEST_P(WeightTest, OperatorEqualsIsEquivalenceRelation) {
  using Weight = TypeParam;
  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    const Weight w1(this->generate_());
    const Weight w2(this->generate_());
    const Weight w3(this->generate_());

    // Checks reflexivity.
    EXPECT_EQ(w1, w1);

    // Checks symmetry.
    EXPECT_EQ(w1 == w2, w2 == w1);

    // Checks transitivity.
    if (w1 == w2 && w2 == w3) {
      EXPECT_EQ(w1, w3);
    }

    // Checks that two weights are either equal or not equal.
    EXPECT_NE(w1 == w2, w1 != w2);

    if (w1 == w2) {
      // Checks that equal weights have identical hashes.
      EXPECT_EQ(w1.Hash(), w2.Hash());
      // Checks that equal weights are also approximately equal.
      EXPECT_TRUE(ApproxEqual(w1, w2));
    }

    // Checks that weights which are not even approximately equal are also
    // strictly unequal.
    if (!ApproxEqual(w1, w2)) {
      EXPECT_NE(w1, w2);
    }
  }
}

TYPED_TEST_P(WeightTest, BinaryIO) {
  using Weight = TypeParam;
  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    const Weight w(this->generate_());
    std::ostringstream os;
    w.Write(os);
    os.flush();
    std::istringstream is(os.str());
    Weight v;
    v.Read(is);
    EXPECT_EQ(w, v);
  }
}

TYPED_TEST_P(WeightTest, TextIOWithParens) {
  absl::SetFlag(&FLAGS_fst_weight_parentheses, "()");

  using Weight = TypeParam;
  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    const Weight w(this->generate_());
    std::ostringstream os;
    os << w;
    std::istringstream is(os.str());
    Weight v(Weight::One());
    is >> v;
    EXPECT_TRUE(ApproxEqual(w, v));
  }
}

TYPED_TEST_P(WeightTest, TextIOWithoutParens) {
  using Weight = TypeParam;
  if (WeightTestTraits<Weight>::IoRequiresParens()) {
    GTEST_SKIP() << "Parens required for I/O";
  }

  absl::SetFlag(&FLAGS_fst_weight_parentheses, "");

  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    const Weight w(this->generate_());
    std::ostringstream os;
    os << w;
    std::istringstream is(os.str());
    Weight v(Weight::One());
    is >> v;
    EXPECT_TRUE(ApproxEqual(w, v));
  }
}

TYPED_TEST_P(WeightTest, Copy) {
  using Weight = TypeParam;
  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    const Weight w(this->generate_());
    Weight x = w;
    EXPECT_EQ(w, x);

    x = Weight(w);
    EXPECT_EQ(w, x);

    x.operator=(x);
    EXPECT_EQ(w, x);
  }
}

REGISTER_TYPED_TEST_SUITE_P(WeightTest, Semiring, Division, Reverse,
                            OperatorEqualsIsEquivalenceRelation, BinaryIO,
                            TextIOWithParens, TextIOWithoutParens, Copy);

}  // namespace fst

#endif  // OPENFST_TEST_WEIGHT_TESTER_H_
