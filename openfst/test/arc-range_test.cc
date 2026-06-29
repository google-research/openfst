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

#include "openfst/lib/arc-range.h"

#include <algorithm>
#include <iterator>
#include <type_traits>
#include <typeinfo>
#include <vector>

#include "gtest/gtest.h"
#include "benchmark/benchmark.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/arcsort.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/relabel.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/test/fst_from_tuples.h"

namespace fst {
namespace {

template <class F>
class ArcRangeTest : public ::testing::Test {
 protected:
  using TypedArcRange = ArcRange<F>;
};

using ArcRangeTestTypes = ::testing::Types<StdFst, StdVectorFst>;
TYPED_TEST_SUITE(ArcRangeTest, ArcRangeTestTypes);

TYPED_TEST(ArcRangeTest, EmptyRange) {
  StdVectorFst fst = CreateFstFromTuples({}, {{0, StdArc::Weight::One()}});
  typename TestFixture::TypedArcRange range(fst, 0);
  EXPECT_TRUE(range.begin() == range.end());
  EXPECT_FALSE(range.begin() != range.end());
  EXPECT_EQ(0, (range.end() - range.begin()));
}

TYPED_TEST(ArcRangeTest, IteratorTraits) {
  using Traits = std::iterator_traits<ArcRange<StdFst>::Iterator>;
  EXPECT_TRUE((std::is_same<Traits::value_type, StdArc>::value));
  EXPECT_TRUE((std::is_same<Traits::pointer, const StdArc*>::value));
  EXPECT_TRUE((std::is_same<Traits::reference, const StdArc&>::value));
  EXPECT_TRUE((std::is_same<Traits::iterator_category,
                            std::random_access_iterator_tag>::value));
}

TYPED_TEST(ArcRangeTest, IteratorCompare) {
  StdVectorFst fst = CreateFstFromTuples({}, {{0, StdArc::Weight::One()}});
  for (int i = 0; i < 10; ++i) {
    fst.AddArc(0, StdArc(i, i + 1, StdArc::Weight(i + 2), i + 3));
  }
  typename TestFixture::TypedArcRange range(fst, 0);

  EXPECT_FALSE(range.begin() == range.end());
  EXPECT_TRUE(range.begin() != range.end());
  EXPECT_TRUE(range.begin() < range.end());
  EXPECT_TRUE(range.begin() <= range.end());
  EXPECT_FALSE(range.begin() > range.end());
  EXPECT_FALSE(range.begin() >= range.end());

  EXPECT_EQ(10, std::distance(range.begin(), range.end()));
  EXPECT_EQ(10, range.end() - range.begin());

  EXPECT_TRUE((range.begin() + 10) == range.end());
  EXPECT_TRUE(range.begin() == (range.end() - 10));
}

TYPED_TEST(ArcRangeTest, IteratorPlus) {
  StdVectorFst fst = CreateFstFromTuples({}, {{0, StdArc::Weight::One()}});
  for (int i = 0; i < 10; ++i) {
    fst.AddArc(0, StdArc(i, i + 1, StdArc::Weight(i + 2), i + 3));
  }
  typename TestFixture::TypedArcRange range(fst, 0);

  int l = 0;
  for (auto iter = range.begin(); iter != range.end(); ++iter, ++l) {
    EXPECT_EQ(l, iter->ilabel);
  }
  EXPECT_EQ(10, l);

  l = 0;
  for (auto iter = range.begin(); iter != range.end(); iter++, ++l) {
    EXPECT_EQ(l, iter->ilabel);
  }
  EXPECT_EQ(10, l);

  l = 0;
  for (auto iter = range.begin(); iter != range.end(); iter += 2, l += 2) {
    EXPECT_EQ(l, iter->ilabel);
  }
  EXPECT_EQ(10, l);

  for (int i = 0; i < 10; ++i) {
    auto iter = range.begin() + i;
    EXPECT_EQ(i, iter->ilabel);
  }
}

TYPED_TEST(ArcRangeTest, IteratorMinus) {
  StdVectorFst fst = CreateFstFromTuples({}, {{0, StdArc::Weight::One()}});
  for (int i = 0; i < 10; ++i) {
    fst.AddArc(0, StdArc(i, i + 1, StdArc::Weight(i + 2), i + 3));
  }
  typename TestFixture::TypedArcRange range(fst, 0);

  int l = 9;
  for (auto iter = range.end() - 1; iter >= range.begin(); --iter, --l) {
    EXPECT_EQ(l, iter->ilabel);
  }
  EXPECT_EQ(-1, l);

  l = 9;
  for (auto iter = range.end() - 1; iter >= range.begin(); iter--, --l) {
    EXPECT_EQ(l, iter->ilabel);
  }
  EXPECT_EQ(-1, l);

  l = 9;
  for (auto iter = range.end() - 1; iter >= range.begin(); iter -= 2, l -= 2) {
    EXPECT_EQ(l, iter->ilabel);
  }
  EXPECT_EQ(-1, l);

  for (int i = 1; i <= 10; ++i) {
    auto iter = range.end() - i;
    EXPECT_EQ(10 - i, iter->ilabel);
  }
}

TYPED_TEST(ArcRangeTest, IteratorAccess) {
  StdVectorFst fst = CreateFstFromTuples({}, {{0, StdArc::Weight::One()}});
  for (int i = 0; i < 10; ++i) {
    fst.AddArc(0, StdArc(i, i + 1, StdArc::Weight(i + 2), i + 3));
  }
  typename TestFixture::TypedArcRange range(fst, 0);

  for (int i = 0; i < 10; ++i) {
    auto iter = (range.begin() + i);
    EXPECT_EQ(i, iter->ilabel);
    const StdArc& a = *iter;
    EXPECT_EQ(i, a.ilabel);
    for (int j = 0; j < 10 - i; ++j) {
      EXPECT_EQ(i + j, iter[j].ilabel);
    }
  }
}

TYPED_TEST(ArcRangeTest, Range) {
  const StdVectorFst fst = CreateFstFromTuples(
      {{0, 1, 2, 3, 4.5}, {1, 2, 3, 4, 5.6}, {1, 3, 4, 5, 6.7}},
      {{1, StdArc::Weight::One()}});

  std::vector<StdArc> arcs;
  for (const StdArc& arc : typename TestFixture::TypedArcRange(fst, 1)) {
    arcs.push_back(arc);
  }
  ASSERT_EQ(2, arcs.size());
  EXPECT_EQ(2, arcs.front().nextstate);
  EXPECT_EQ(3, arcs.front().ilabel);
  EXPECT_EQ(4, arcs.front().olabel);
  EXPECT_FLOAT_EQ(5.6, arcs.front().weight.Value());

  EXPECT_EQ(3, arcs.back().nextstate);
  EXPECT_EQ(4, arcs.back().ilabel);
  EXPECT_EQ(5, arcs.back().olabel);
  EXPECT_FLOAT_EQ(6.7, arcs.back().weight.Value());
}

TYPED_TEST(ArcRangeTest, Find) {
  StdVectorFst fst = CreateFstFromTuples({}, {{0, StdArc::Weight::One()}});
  for (int i = 0; i < 10; ++i) {
    fst.AddArc(0, StdArc(i, i + 1, StdArc::Weight(i + 2), i + 3));
  }
  typename TestFixture::TypedArcRange range(fst, 0);

  auto i = std::find_if(range.begin(), range.end(),
                        [](const StdArc& a) { return a.ilabel == 4; });
  EXPECT_EQ(4, std::distance(range.begin(), i));
}

TEST(ArcRangeSelectorTest, StdFst) {
  StdVectorFst fst = CreateFstFromTuples({}, {{0, StdArc::Weight::One()}});
  const StdFst& std_fst = fst;
  auto range = GetArcs(std_fst, 0);
  auto iter = range.begin();
  EXPECT_TRUE(
      (std::is_same<decltype(iter), internal::ArcRangeIterator<
                                        fst::ArcIterator<StdFst>>>::value))
      << typeid(iter).name();
}

TEST(ArcRangeSelectorTest, VectorFst) {
  // Check that the pointer iterator is used for VectorFst.
  StdVectorFst fst = CreateFstFromTuples({}, {{0, StdArc::Weight::One()}});
  auto range = GetArcs(fst, 0);
  EXPECT_TRUE((std::is_same<decltype(range.begin()), const StdArc*>::value));
}

TEST(GetArcsTest, CacheArcIterator) {
  StdVectorFst fst = CreateFstFromTuples({}, {{0, StdArc::Weight::One()}});
  fst::RelabelFst<StdArc> relabel(fst, {}, {});
  auto range = GetArcs(relabel, 0);
  EXPECT_TRUE((
      std::is_same<ArcRange<fst::RelabelFst<StdArc>>, decltype(range)>::value));
  EXPECT_TRUE((std::is_same<decltype(range.begin()), const StdArc*>::value));
}

TEST(GetArcsTest, ArcSortFst) {
  using Compare = ILabelCompare<StdArc>;
  using F = ArcSortFst<StdArc, Compare>;
  F fst(CreateFstFromTuples<StdVectorFst>(
            {}, {{0, StdArc::Weight::One()}}),
        Compare());
  auto range = GetArcs(fst, 0);
  EXPECT_TRUE((std::is_same<ArcRange<F>, decltype(range)>::value));
}

StdVectorFst CreateBenchmarkFst(int num_arcs) {
  StdVectorFst fst;
  fst.AddState();
  fst.ReserveArcs(0, num_arcs);
  for (int i = 0; i < num_arcs; ++i) {
    fst.AddArc(0, {0, 0, 0, 0});
  }
  return fst;
}

void BM_ArcIteratorVectorFst(benchmark::State& state) {
  const StdVectorFst fst = CreateBenchmarkFst(state.range(0));
  while (state.KeepRunning()) {
    for (fst::ArcIterator<StdVectorFst> aiter(fst, 0); !aiter.Done();
         aiter.Next()) {
      benchmark::DoNotOptimize(aiter.Value().nextstate);
    }
  }
}

void BM_ArcIteratorFst(benchmark::State& state) {
  const StdVectorFst fst = CreateBenchmarkFst(state.range(0));
  while (state.KeepRunning()) {
    for (fst::ArcIterator<StdFst> aiter(fst, 0); !aiter.Done(); aiter.Next()) {
      benchmark::DoNotOptimize(aiter.Value().nextstate);
    }
  }
}

void BM_ArcIteratorRelabelFst(benchmark::State& state) {
  const StdVectorFst fst = CreateBenchmarkFst(state.range(0));
  const fst::RelabelFst<StdArc> relabel_fst(fst, {}, {});
  while (state.KeepRunning()) {
    for (fst::ArcIterator<fst::RelabelFst<StdArc>> aiter(relabel_fst, 0);
         !aiter.Done(); aiter.Next()) {
      benchmark::DoNotOptimize(aiter.Value().nextstate);
    }
  }
}

void BM_ArcRangeVectorFst(benchmark::State& state) {
  const StdVectorFst fst = CreateBenchmarkFst(state.range(0));
  while (state.KeepRunning()) {
    for (const auto& arc : ArcRange<StdVectorFst>(fst, 0)) {
      benchmark::DoNotOptimize(arc.nextstate);
    }
  }
}

void BM_ArcRangeFst(benchmark::State& state) {
  const StdVectorFst fst = CreateBenchmarkFst(state.range(0));
  while (state.KeepRunning()) {
    for (const auto& arc : ArcRange<StdFst>(fst, 0)) {
      benchmark::DoNotOptimize(arc.nextstate);
    }
  }
}

void BM_ArcRangeRelabelFst(benchmark::State& state) {
  const StdVectorFst fst = CreateBenchmarkFst(state.range(0));
  const fst::RelabelFst<StdArc> relabel_fst(fst, {}, {});
  while (state.KeepRunning()) {
    for (const auto& arc : ArcRange<fst::RelabelFst<StdArc>>(relabel_fst, 0)) {
      benchmark::DoNotOptimize(arc.nextstate);
    }
  }
}

BENCHMARK(BM_ArcIteratorVectorFst)->Range(1, 1 << 20);
BENCHMARK(BM_ArcIteratorFst)->Range(1, 1 << 20);
BENCHMARK(BM_ArcIteratorRelabelFst)->Range(1, 1 << 20);
BENCHMARK(BM_ArcRangeVectorFst)->Range(1, 1 << 20);
BENCHMARK(BM_ArcRangeFst)->Range(1, 1 << 20);
BENCHMARK(BM_ArcRangeRelabelFst)->Range(1, 1 << 20);

}  // namespace
}  // namespace fst
