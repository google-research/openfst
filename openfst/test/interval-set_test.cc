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
// Unit test for the IntervalSet class.

#include "openfst/lib/interval-set.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <ostream>
#include <random>
#include <set>
#include <vector>

#include "gtest/gtest.h"
#include "absl/flags/flag.h"
#include "absl/log/log.h"
#include "openfst/compat/seed_sequences.h"
#include "absl/random/random.h"

ABSL_FLAG(int32_t, repeat, 1000, "number of test repetitions");

namespace fst {
namespace {

using T = int;

class IntervalSetTest : public testing::Test {
 public:
  using Interval = IntervalSet<T>::Interval;

 protected:
  void TearDown() override {
    iset1_.MutableIntervals()->clear();
    iset2_.MutableIntervals()->clear();
    iset3_.MutableIntervals()->clear();
    iset4_.MutableIntervals()->clear();
    iset5_.MutableIntervals()->clear();
    iset6_.MutableIntervals()->clear();
    iset7_.MutableIntervals()->clear();
    iset8_.MutableIntervals()->clear();
  }

  bool SetEqual(const std::set<T>& set1, const IntervalSet<T>& iset2) const {
    bool ret = true;
    if (iset2.Count() != set1.size()) {
      ret = false;
    } else {
      for (std::set<T>::const_iterator it = set1.begin(); it != set1.end();
           ++it) {
        if (!iset2.Member(*it)) {
          ret = false;
          break;
        }
      }
    }
    if (!ret) {
      std::cerr << "set " << &set1 << " size: " << set1.size() << std::endl;
      std::cerr << "iset " << &iset2 << " size: " << iset2.Count() << std::endl;
      PrintSet(set1);
      PrintIntervalSet(iset2);
    }
    return ret;
  }

  void PrintSet(const std::set<T>& s) const {
    std::cerr << "set " << &s << ": ";
    for (auto it = s.begin(); it != s.end(); ++it) std::cerr << *it << " ";
    std::cerr << std::endl;
  }

  void PrintIntervalSet(const IntervalSet<T>& s) const {
    std::cerr << "iset " << &s << " " << s << std::endl;
  }

  absl::BitGen rand_{fst::MakeTaggedSeedSeq("INTERVAL_SET_TEST")};
  IntervalSet<T> iset0_ = IntervalSet<T>{};
  IntervalSet<T> iset1_ = {{0, 2}, {4, 7}, {9, 10}};
  IntervalSet<T> iset2_ = {{1, 2}, {3, 5}, {6, 10}};
  IntervalSet<T> iset3_ = {{0, 2}, {3, 10}};
  IntervalSet<T> iset4_ = {{1, 2}, {4, 5}, {6, 7}, {9, 10}};
  IntervalSet<T> iset5_ = {{0, 11}};
  IntervalSet<T> iset6_ = {{2, 4}, {7, 9}, {10, 11}};
  IntervalSet<T> iset7_ = {{0, 3}, {4, 6}, {9, 10}};
  IntervalSet<T> iset8_ = {{0, 10}};

  static constexpr T kNumInsertions = 10;
  static constexpr T kNumElements = 20;
};

TEST_F(IntervalSetTest, UnionTest) {
  IntervalSet<T> iset0 = iset0_;
  iset0.Union(iset0_);
  iset0.Normalize();
  ASSERT_TRUE(iset0 == iset0_);

  IntervalSet<T> iset1 = iset1_;
  iset1.Union(iset1_);
  iset1.Normalize();
  ASSERT_TRUE(iset1 == iset1_);

  IntervalSet<T> iset2 = iset1_;
  iset2.Union(iset0_);
  iset2.Union(iset2_);
  iset2.Normalize();
  ASSERT_TRUE(iset2 == iset3_);
}

TEST_F(IntervalSetTest, IntersectTest) {
  IntervalSet<T> iset0;
  iset0_.Intersect(iset1_, &iset0);
  ASSERT_TRUE(iset0 == iset0_);

  IntervalSet<T> iset1;
  iset1_.Intersect(iset1_, &iset1);
  ASSERT_TRUE(iset1 == iset1_);

  IntervalSet<T> iset2;
  iset1_.Intersect(iset2_, &iset2);
  ASSERT_TRUE(iset2 == iset4_);
}

TEST_F(IntervalSetTest, ComplementTest) {
  IntervalSet<T> iset0;
  iset0_.Complement(11, &iset0);
  ASSERT_TRUE(iset0 == iset5_);

  IntervalSet<T> iset1;
  iset1_.Complement(11, &iset1);
  ASSERT_TRUE(iset1 == iset6_);
}

TEST_F(IntervalSetTest, DifferenceTest) {
  IntervalSet<T> iset0;
  iset0_.Difference(iset0_, &iset0);
  ASSERT_TRUE(iset0 == iset0_);

  IntervalSet<T> iset1;
  iset5_.Difference(iset1_, &iset1);
  ASSERT_TRUE(iset1 == iset6_);

  IntervalSet<T> iset2;
  IntervalSet<T> iset3;
  iset1_.Intersect(iset2_, &iset2);
  iset1_.Difference(iset2_, &iset3);
  iset2.Union(iset3);
  iset2.Normalize();
  ASSERT_TRUE(iset2 == iset1_);
}

TEST_F(IntervalSetTest, OverlapTest) {
  ASSERT_FALSE(iset0_.Overlaps(iset0_));
  ASSERT_TRUE(iset1_.Overlaps(iset1_));
  ASSERT_TRUE(iset1_.Overlaps(iset2_));
  ASSERT_TRUE(iset1_.Overlaps(iset4_));
  ASSERT_TRUE(iset1_.Overlaps(iset5_));
  ASSERT_TRUE(iset1_.Overlaps(iset7_));
}

TEST_F(IntervalSetTest, StrictlyOverlapTest) {
  ASSERT_FALSE(iset0_.StrictlyOverlaps(iset0_));
  ASSERT_FALSE(iset1_.StrictlyOverlaps(iset1_));
  ASSERT_TRUE(iset1_.StrictlyOverlaps(iset2_));
  ASSERT_FALSE(iset1_.StrictlyOverlaps(iset4_));
  ASSERT_FALSE(iset1_.StrictlyOverlaps(iset5_));
  ASSERT_TRUE(iset1_.StrictlyOverlaps(iset7_));
}

TEST_F(IntervalSetTest, ContainsTest) {
  ASSERT_TRUE(iset0_.Contains(iset0_));
  ASSERT_TRUE(iset1_.Contains(iset1_));
  ASSERT_TRUE(iset3_.Contains(iset1_));
  ASSERT_TRUE(iset3_.Contains(iset2_));
  ASSERT_FALSE(iset3_.Contains(iset5_));
  ASSERT_FALSE(iset7_.Contains(iset6_));
  ASSERT_TRUE(iset5_.Contains(iset8_));
  ASSERT_FALSE(iset8_.Contains(iset5_));
}

TEST_F(IntervalSetTest, RandomTest) {
  for (T i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) {
    std::set<T> set1;
    std::set<T> set2;
    IntervalSet<T> iset1;
    IntervalSet<T> iset2;

    const int num_insertions =
        absl::Uniform(absl::IntervalClosedClosed, rand_, 1, kNumInsertions);
    const int num_elements =
        absl::Uniform(absl::IntervalClosedClosed, rand_, 1, kNumElements);
    for (T j = 0; j < num_insertions; ++j) {
      if (absl::Bernoulli(rand_, .5)) {
        T k1 = absl::Uniform(rand_, 0, num_elements);
        set1.insert(k1);
        iset1.MutableIntervals()->push_back(Interval(k1, k1 + 1));
      }
      if (absl::Bernoulli(rand_, .5)) {
        T k2 = absl::Uniform(rand_, 0, num_elements);
        set2.insert(k2);
        iset2.MutableIntervals()->push_back(Interval(k2, k2 + 1));
      }
    }
    iset1.Normalize();
    iset2.Normalize();
    //    PrintIntervalSet(iset1);
    //    PrintIntervalSet(iset2);

    ASSERT_TRUE(SetEqual(set1, iset1));
    ASSERT_TRUE(SetEqual(set2, iset2));

    std::set<T> set3;
    std::set_union(set1.begin(), set1.end(), set2.begin(), set2.end(),
                   std::inserter(set3, set3.end()));
    IntervalSet<T> iset3;
    iset3.Union(iset1);
    iset3.Union(iset2);
    iset3.Normalize();
    ASSERT_TRUE(SetEqual(set3, iset3));

    std::set<T> set4;
    std::set_intersection(set1.begin(), set1.end(), set2.begin(), set2.end(),
                          std::inserter(set4, set4.end()));
    IntervalSet<T> iset4;
    iset1.Intersect(iset2, &iset4);
    ASSERT_TRUE(SetEqual(set4, iset4));

    std::set<T> set5;
    std::set_difference(set1.begin(), set1.end(), set2.begin(), set2.end(),
                        std::inserter(set5, set5.end()));
    IntervalSet<T> iset5;
    iset1.Difference(iset2, &iset5);
    ASSERT_TRUE(SetEqual(set5, iset5));

    std::set<T> set6;
    std::set_difference(set2.begin(), set2.end(), set1.begin(), set1.end(),
                        std::inserter(set6, set6.end()));
    IntervalSet<T> iset6;
    iset2.Difference(iset1, &iset6);
    ASSERT_TRUE(SetEqual(set6, iset6));

    bool overlaps = !set4.empty();
    ASSERT_EQ(overlaps, iset1.Overlaps(iset2));
    if (!iset1.Empty()) {
      ASSERT_TRUE(iset3.Overlaps(iset1));
    }
    if (!iset2.Empty()) {
      ASSERT_TRUE(iset3.Overlaps(iset2));
    }

    bool strictly_overlaps = !set4.empty() && !set5.empty() && !set6.empty();
    ASSERT_EQ(strictly_overlaps, iset1.StrictlyOverlaps(iset2));

    bool contains = set2.empty() || (!set4.empty() && set6.empty());
    ASSERT_EQ(contains, iset1.Contains(iset2));
    ASSERT_TRUE(iset3.Contains(iset1));
    ASSERT_TRUE(iset3.Contains(iset2));
    ASSERT_TRUE(iset1.Contains(iset4));
    ASSERT_TRUE(iset2.Contains(iset4));
    ASSERT_TRUE(iset1.Contains(iset5));
  }
}

}  // namespace
}  // namespace fst
