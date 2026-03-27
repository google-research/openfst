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
// Unit test for ArcMap.

#include "openfst/lib/arc-map.h"

#include <memory>
#include <string>
#include <type_traits>
#include <utility>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/cache.h"
#include "openfst/lib/equal.h"
#include "openfst/lib/expanded-fst.h"
#include "openfst/lib/float-weight.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/string-weight.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/lib/verify.h"

namespace fst {
namespace {

// A weight converter that is not default constructible, for use with
// `WeightConvertMapper`. The resulting mapper (`WeightConvertMapper<StdArc,
// StdArc, MyNonDefaultConstructibleWeightConverter>`) is functionally identical
// to `PlusMapper<StdArc>`.
class MyNonDefaultConstructibleWeightConverter {
 public:
  constexpr explicit MyNonDefaultConstructibleWeightConverter(
      StdArc::Weight weight)
      : weight_(weight) {}
  StdArc::Weight operator()(const StdArc::Weight& weight) const {
    return Plus(weight, weight_);
  }

 private:
  StdArc::Weight weight_;
};

class ArcMapTest : public testing::Test {
 protected:
  void SetUp() override {
    const std::string path =
        JoinPath(std::string("."),
                       "openfst/test/testdata/arc-map");
    const std::string map1_name = JoinPath(path, "m1.fst");
    const std::string map2_name = JoinPath(path, "m2.fst");
    const std::string map3_name = JoinPath(path, "m3.fst");
    const std::string map4_name = JoinPath(path, "m4.fst");
    const std::string map5_name = JoinPath(path, "m5.fst");
    const std::string map13_name = JoinPath(path, "m13.fst");
    const std::string map14_name = JoinPath(path, "m14.fst");
    const std::string map15_name = JoinPath(path, "m15.fst");
    const std::string map16_name = JoinPath(path, "m16.fst");

    mfst1_.reset(VectorFst<StdArc>::Read(map1_name));
    mfst2_.reset(VectorFst<LogArc>::Read(map2_name));
    mfst3_.reset(VectorFst<GallicArc<StdArc>>::Read(map3_name));
    mfst4_.reset(VectorFst<StdArc>::Read(map4_name));
    mfst5_.reset(VectorFst<StdArc>::Read(map5_name));
    mfst13_.reset(VectorFst<StdArc>::Read(map13_name));
    mfst14_.reset(VectorFst<StdArc>::Read(map14_name));
    mfst15_.reset(VectorFst<StdArc>::Read(map15_name));
    mfst16_.reset(VectorFst<StdArc>::Read(map16_name));
  }

  std::unique_ptr<const VectorFst<StdArc>> mfst1_;
  std::unique_ptr<const VectorFst<LogArc>> mfst2_;
  std::unique_ptr<const VectorFst<GallicArc<StdArc>>> mfst3_;
  std::unique_ptr<const VectorFst<StdArc>> mfst4_;
  std::unique_ptr<const VectorFst<StdArc>> mfst5_;
  std::unique_ptr<const VectorFst<StdArc>> mfst13_;
  std::unique_ptr<const VectorFst<StdArc>> mfst14_;
  std::unique_ptr<const VectorFst<StdArc>> mfst15_;
  std::unique_ptr<const VectorFst<StdArc>> mfst16_;
};

// This tests the self-modifying map using the identity mapper.
// Project and Invert unit tests do more exciting mappings.
TEST_F(ArcMapTest, DestructiveArcMap) {
  VectorFst<StdArc> nfst;

  VectorFst<StdArc> vfst1(*mfst1_);
  VectorFst<StdArc> vfst2(nfst);

  IdentityArcMapper<StdArc> mapper;

  ArcMap(&vfst1, mapper);
  ASSERT_TRUE(Verify(vfst1));
  ASSERT_TRUE(Equal(*mfst1_, vfst1));

  ArcMap(&vfst2, mapper);
  ASSERT_TRUE(Verify(vfst2));

  ASSERT_TRUE(Equal(nfst, vfst2));
}

// This tests mappings from a StdFst to a Log MutableFst and back.
TEST_F(ArcMapTest, StdLogMap) {
  VectorFst<StdArc> nsfst;
  VectorFst<LogArc> nlfst;

  const StdToLogMapper s2lmapper;
  const LogToStdMapper l2smapper;

  VectorFst<LogArc> lfst1;
  ArcMap(*mfst1_, &lfst1, s2lmapper);
  ASSERT_TRUE(Verify(lfst1));
  ASSERT_TRUE(Equal(*mfst2_, lfst1));

  VectorFst<LogArc> cfst1;
  Cast(*mfst1_, &cfst1);
  ASSERT_TRUE(Verify(cfst1));
  ASSERT_TRUE(Equal(lfst1, cfst1));

  VectorFst<StdArc> sfst1;
  ArcMap(lfst1, &sfst1, l2smapper);
  ASSERT_TRUE(Verify(sfst1));
  ASSERT_TRUE(Equal(*mfst1_, sfst1));

  VectorFst<LogArc> lfst2;
  ArcMap(nsfst, &lfst2, s2lmapper);
  ASSERT_TRUE(Verify(lfst2));
  ASSERT_TRUE(Equal(lfst2, nlfst));

  VectorFst<StdArc> sfst2;
  ArcMap(lfst2, &sfst2, l2smapper);
  ASSERT_TRUE(Verify(sfst2));
  ASSERT_TRUE(Equal(nsfst, sfst2));
}

// This tests mappings from a StdFst to a Gallic MutableFst and back
// and between left and right Gallic.
TEST_F(ArcMapTest, GallicMap) {
  using LGA = GallicArc<StdArc>;
  using RGA = GallicArc<StdArc, GALLIC_RIGHT>;

  ToGallicMapper<StdArc> a2lgmapper;
  FromGallicMapper<StdArc> lg2amapper;

  VectorFst<LGA> lgfst1;
  ArcMap(*mfst1_, &lgfst1, a2lgmapper);
  ASSERT_TRUE(Verify(lgfst1));
  ASSERT_TRUE(Equal(*mfst3_, lgfst1));

  VectorFst<StdArc> sfst1;
  ArcMap(lgfst1, &sfst1, lg2amapper);
  ASSERT_TRUE(Verify(sfst1));
  ASSERT_TRUE(Equal(*mfst1_, sfst1));

  ToGallicMapper<StdArc, GALLIC_RIGHT> a2rgmapper;
  FromGallicMapper<StdArc, GALLIC_RIGHT> rg2amapper;

  VectorFst<RGA> rgfst1;
  ArcMap(*mfst1_, &rgfst1, a2rgmapper);
  ASSERT_TRUE(Verify(rgfst1));

  VectorFst<StdArc> sfst2;
  ArcMap(rgfst1, &sfst2, rg2amapper);
  ASSERT_TRUE(Verify(sfst2));
  ASSERT_TRUE(Equal(*mfst1_, sfst2));

  ReverseWeightMapper<LGA, RGA> lrgmapper;
  ReverseWeightMapper<RGA, LGA> rlgmapper;

  VectorFst<RGA> rgfst2;
  ArcMap(lgfst1, &rgfst2, lrgmapper);
  ASSERT_TRUE(Verify(rgfst2));
  ASSERT_TRUE(Equal(rgfst1, rgfst2));

  VectorFst<LGA> lgfst2;
  ArcMap(rgfst1, &lgfst2, rlgmapper);
  ASSERT_TRUE(Verify(lgfst2));
  ASSERT_TRUE(Equal(lgfst1, lgfst2));
}

TEST_F(ArcMapTest, OnTheFlyArcMapFstEmpty) {
  ArcMapFst<StdArc, LogArc, StdToLogMapper> lfst2{StdVectorFst()};
  ASSERT_TRUE(Verify(lfst2));
  ASSERT_TRUE(Equal(VectorFst<LogArc>(), lfst2));

  ArcMapFst<LogArc, StdArc, LogToStdMapper> sfst2(lfst2);
  ASSERT_TRUE(Verify(sfst2));
  ASSERT_TRUE(Equal(StdVectorFst(), sfst2));
}

TEST_F(ArcMapTest, OnTheFlyArcMapFst) {
  ArcMapFst<StdArc, LogArc, StdToLogMapper> lfst1(*mfst1_);
  ASSERT_TRUE(Verify(lfst1));
  ASSERT_TRUE(Equal(*mfst2_, lfst1));

  ArcMapFst<LogArc, StdArc, LogToStdMapper> sfst1(lfst1);
  ASSERT_TRUE(Verify(sfst1));
  ASSERT_TRUE(Equal(*mfst1_, sfst1));
}

TEST_F(ArcMapTest, OnTheFlyArcMapFstCopyConstructor) {
  ArcMapFst<StdArc, LogArc, StdToLogMapper> lfst1(*mfst1_);
  ASSERT_TRUE(Verify(lfst1));
  ASSERT_TRUE(Equal(*mfst2_, lfst1));

  for (const bool safe : {false, true}) {
    ArcMapFst<StdArc, LogArc, StdToLogMapper> cfst(lfst1, safe);
    ASSERT_TRUE(Verify(cfst));
    ASSERT_TRUE(Equal(*mfst2_, cfst));
  }
}

TEST_F(ArcMapTest, OnTheFlyCTADArcMapFstEmpty) {
  ArcMapFst lfst2{StdVectorFst(), StdToLogMapper()};
  ASSERT_TRUE(Verify(lfst2));
  ASSERT_TRUE(Equal(VectorFst<LogArc>(), lfst2));

  ArcMapFst sfst2(lfst2, LogToStdMapper());
  ASSERT_TRUE(Verify(sfst2));
  ASSERT_TRUE(Equal(StdVectorFst(), sfst2));
}

TEST_F(ArcMapTest, OnTheFlyCTADArcMapFst) {
  ArcMapFst lfst1(*mfst1_, StdToLogMapper());
  ASSERT_TRUE(Verify(lfst1));
  ASSERT_TRUE(Equal(*mfst2_, lfst1));

  ArcMapFst sfst1(lfst1, LogToStdMapper());
  ASSERT_TRUE(Verify(sfst1));
  ASSERT_TRUE(Equal(*mfst1_, sfst1));
}

static_assert(
    std::is_same_v<decltype(ArcMapFst(std::declval<Fst<StdArc>>(),
                                      StdToLogMapper())),
                   ArcMapFst<StdToLogMapper::FromArc, StdToLogMapper::ToArc,
                             StdToLogMapper>>,
    "ArcMapFst's CTAD deduction guides fail to correctly instantiate when "
    "performing a StdArc to LogArc mapping.");

static_assert(
    std::is_same_v<decltype(ArcMapFst(std::declval<Fst<LogArc>>(),
                                      LogToStdMapper())),
                   ArcMapFst<LogToStdMapper::FromArc, LogToStdMapper::ToArc,
                             LogToStdMapper>>,
    "ArcMapFst's CTAD deduction guides fail to correctly instantiate when "
    "performing a LogArc to StdArc mapping.");

// Mapper pointer constructor
static_assert(
    std::is_same_v<decltype(ArcMapFst(std::declval<Fst<StdArc>>(),
                                      static_cast<StdToLog64Mapper*>(nullptr))),
                   ArcMapFst<StdToLog64Mapper::FromArc, StdToLog64Mapper::ToArc,
                             StdToLog64Mapper>>,
    "ArcMapFst's CTAD deduction guides fail to correctly instantiate when "
    "performing a StdArc to Log64Arc mapping using the mapper pointer "
    "constructor.");

//// Now test the case where the mapper cannot support compile-time expandedness
//   preservation. Check that when fed an `Fst`, it of course cannot take the
//   optimization because of its mapper, due the mapper not being default
//   constructible.
static_assert(
    !std::is_base_of_v<ExpandedFst<StdArc>,
                       decltype(ArcMapFst(std::declval<Fst<StdArc>>(),
                                          std::declval<PlusMapper<StdArc>>()))>,
    "ArcMapFst's CTAD deduction guides fail to correctly instantiate when "
    "using PlusMapper<StdArc> mapping.");

// Mapper pointer constructor
static_assert(
    !std::is_base_of_v<
        ExpandedFst<StdArc>,
        decltype(ArcMapFst(std::declval<Fst<StdArc>>(),
                           static_cast<PlusMapper<StdArc>*>(nullptr)))>,
    "ArcMapFst's CTAD deduction guides fail to correctly instantiate when "
    "using PlusMapper<StdArc> via the mapper pointer constructor.");

//// Now test the case where the mapper cannot support compile-time expandedness
//   preservation. Check that when fed an `Fst`, it of course cannot take the
//   optimization because of its mapper, due the mapper not being default
//   constructible, for the case of `WeightConvertMapper`.
static_assert(
    !std::is_base_of_v<
        ExpandedFst<StdArc>,
        decltype(ArcMapFst(
            std::declval<Fst<StdArc>>(),
            std::declval<WeightConvertMapper<
                StdArc, StdArc, MyNonDefaultConstructibleWeightConverter>>()))>,
    "ArcMapFst's CTAD deduction guides fail to correctly instantiate when "
    "using `WeightConvertMapper<StdArc, StdArc, "
    "MyNonDefaultConstructibleWeightConverter>`.");

// Mapper pointer constructor
static_assert(
    !std::is_base_of_v<
        ExpandedFst<StdArc>,
        decltype(ArcMapFst(
            std::declval<Fst<StdArc>>(),
            static_cast<WeightConvertMapper<
                StdArc, StdArc, MyNonDefaultConstructibleWeightConverter>*>(
                nullptr)))>,
    "ArcMapFst's CTAD deduction guides fail to correctly instantiate when "
    "using `WeightConvertMapper<StdArc, StdArc, "
    "MyNonDefaultConstructibleWeightConverter>` via the mapper "
    "pointer constructor.");

//// Now test the case where the mapper cannot support compile-time expandedness
//   preservation. Check that when fed an `Fst`, it of course doesn't take the
//   optimization.
static_assert(
    !std::is_base_of_v<ExpandedFst<StdArc>,
                       decltype(ArcMapFst(std::declval<Fst<StdArc>>(),
                                          ToGallicMapper<StdArc>()))>,
    "ArcMapFst's CTAD deduction guides fail to correctly instantiate when "
    "performing a StdArc to ToGallicMapper<LogArc> mapping.");

// Mapper pointer constructor
static_assert(
    !std::is_base_of_v<
        ExpandedFst<StdArc>,
        decltype(ArcMapFst(std::declval<Fst<StdArc>>(),
                           static_cast<ToGallicMapper<StdArc>*>(nullptr)))>,
    "ArcMapFst's CTAD deduction guides fail to correctly instantiate when "
    "performing a LogArc to ToGallicMapper<LogArc> mapping via the mapper "
    "pointer constructor.");

TEST_F(ArcMapTest, OnTheFlyExpandedArcMapFstEmpty) {
  ArcMapFst<StdArc, LogArc, StdToLogMapper, DefaultCacheStore<LogArc>,
            PropagateKExpanded::kIfPossible>
      lfst2{StdVectorFst()};
  ASSERT_TRUE(Verify(lfst2));
  ASSERT_TRUE(Equal(VectorFst<LogArc>(), lfst2));
  ASSERT_EQ(lfst2.NumStates(), 0);
  static_assert(
      std::is_base_of_v<ExpandedFst<StdArc>, decltype(StdVectorFst())>,
      "StdVectorFst is not an ExpandedFst, as it should be.");

  static_assert(std::is_base_of_v<ExpandedFst<LogArc>, decltype(lfst2)>,
                "lfst2 is not an ExpandedFst, as it should be.");

  ArcMapFst<LogArc, StdArc, LogToStdMapper, DefaultCacheStore<StdArc>,
            PropagateKExpanded::kIfPossible>
      sfst2(lfst2);
  ASSERT_TRUE(Verify(sfst2));
  ASSERT_TRUE(Equal(StdVectorFst(), sfst2));
  ASSERT_EQ(sfst2.NumStates(), 0);
}

TEST_F(ArcMapTest, OnTheFlyExpandedArcMapFst) {
  ArcMapFst<StdArc, LogArc, StdToLogMapper, DefaultCacheStore<LogArc>,
            PropagateKExpanded::kIfPossible>
      lfst1(*mfst1_);
  ASSERT_TRUE(Verify(lfst1));
  ASSERT_TRUE(Equal(*mfst2_, lfst1));

  ArcMapFst<LogArc, StdArc, LogToStdMapper, DefaultCacheStore<StdArc>,
            PropagateKExpanded::kIfPossible>
      sfst1(lfst1);
  ASSERT_TRUE(Verify(sfst1));
  ASSERT_TRUE(Equal(*mfst1_, sfst1));
}

TEST_F(ArcMapTest, OnTheFlyExpandedArcMapFstCopyConstructor) {
  ArcMapFst<StdArc, LogArc, StdToLogMapper, DefaultCacheStore<LogArc>,
            PropagateKExpanded::kIfPossible>
      lfst1(*mfst1_);
  ASSERT_TRUE(Verify(lfst1));
  ASSERT_TRUE(Equal(*mfst2_, lfst1));

  for (const bool safe : {false, true}) {
    ArcMapFst<StdArc, LogArc, StdToLogMapper, DefaultCacheStore<LogArc>,
              PropagateKExpanded::kIfPossible>
        cfst(lfst1, safe);
    ASSERT_TRUE(Verify(cfst));
    ASSERT_TRUE(Equal(*mfst2_, cfst));
  }
}

TEST_F(ArcMapTest, OnTheFlyCTADExpandedArcMapFstEmpty) {
  ArcMapFst lfst2{StdVectorFst(), StdToLogMapper()};
  ASSERT_TRUE(Verify(lfst2));
  ASSERT_TRUE(Equal(VectorFst<LogArc>(), lfst2));

  ArcMapFst sfst2(lfst2, LogToStdMapper());
  ASSERT_TRUE(Verify(sfst2));
  ASSERT_TRUE(Equal(StdVectorFst(), sfst2));
}

TEST_F(ArcMapTest, OnTheFlyCTADExpandedArcMapFst) {
  ArcMapFst lfst1(*mfst1_, StdToLogMapper());
  ASSERT_TRUE(Verify(lfst1));
  ASSERT_TRUE(Equal(*mfst2_, lfst1));

  ArcMapFst sfst1(lfst1, LogToStdMapper());
  ASSERT_TRUE(Verify(sfst1));
  ASSERT_TRUE(Equal(*mfst1_, sfst1));
}

static_assert(
    std::is_same_v<
        decltype(ArcMapFst(VectorFst<StdArc>(), StdToLogMapper())),
        ArcMapFst<StdToLogMapper::FromArc, StdToLogMapper::ToArc,
                  StdToLogMapper, DefaultCacheStore<StdToLogMapper::ToArc>,
                  PropagateKExpanded::kIfPossible>>,
    "ArcMapFst's CTAD deduction guides fail to correctly instantiate when "
    "performing a StdArc to LogArc mapping.");

static_assert(
    std::is_same_v<
        decltype(ArcMapFst(VectorFst<LogArc>(), LogToStdMapper())),
        ArcMapFst<LogToStdMapper::FromArc, LogToStdMapper::ToArc,
                  LogToStdMapper, DefaultCacheStore<LogToStdMapper::ToArc>,
                  PropagateKExpanded::kIfPossible>>,
    "ArcMapFst's CTAD deduction guides fail to correctly instantiate when "
    "performing a LogArc to StdArc mapping.");

// Mapper pointer constructor
static_assert(
    std::is_same_v<
        decltype(ArcMapFst(VectorFst<StdArc>(),
                           static_cast<StdToLog64Mapper*>(nullptr))),
        ArcMapFst<StdToLog64Mapper::FromArc, StdToLog64Mapper::ToArc,
                  StdToLog64Mapper, DefaultCacheStore<StdToLog64Mapper::ToArc>,
                  PropagateKExpanded::kIfPossible>>,
    "ArcMapFst's CTAD deduction guides fail to correctly instantiate when "
    "performing a StdArc to Log64Arc mapping using the mapper pointer "
    "constructor.");

//// Now test the case where the mapper cannot support compile-time expandedness
//   preservation. Check that when fed an `ExpandedFst`, it still cannot take
//   the optimization because of its mapper, due the mapper not being default
//   constructible.
static_assert(
    !std::is_base_of_v<ExpandedFst<StdArc>,
                       decltype(ArcMapFst(VectorFst<StdArc>(),
                                          std::declval<PlusMapper<StdArc>>()))>,
    "ArcMapFst's CTAD deduction guides fail to correctly instantiate when "
    "using PlusMapper<StdArc> mapping.");

// Mapper pointer constructor
static_assert(
    !std::is_base_of_v<
        ExpandedFst<StdArc>,
        decltype(ArcMapFst(VectorFst<StdArc>(),
                           static_cast<PlusMapper<StdArc>*>(nullptr)))>,
    "ArcMapFst's CTAD deduction guides fail to correctly instantiate when "
    "using PlusMapper<StdArc> via the mapper pointer constructor.");

//// Now test the case where the mapper cannot support compile-time expandedness
//   preservation. Check that when fed an `ExpandedFst`, it still cannot take
//   the optimization because of its mapper, due the mapper not being default
//   constructible, for the case of `WeightConvertMapper`.
static_assert(
    !std::is_base_of_v<
        ExpandedFst<StdArc>,
        decltype(ArcMapFst(
            VectorFst<StdArc>(),
            std::declval<WeightConvertMapper<
                StdArc, StdArc, MyNonDefaultConstructibleWeightConverter>>()))>,
    "ArcMapFst's CTAD deduction guides fail to correctly instantiate when "
    "using `WeightConvertMapper<StdArc, StdArc, "
    "MyNonDefaultConstructibleWeightConverter>`.");

// Mapper pointer constructor
static_assert(
    !std::is_base_of_v<
        ExpandedFst<StdArc>,
        decltype(ArcMapFst(
            VectorFst<StdArc>(),
            static_cast<WeightConvertMapper<
                StdArc, StdArc, MyNonDefaultConstructibleWeightConverter>*>(
                nullptr)))>,
    "ArcMapFst's CTAD deduction guides fail to correctly instantiate when "
    "`WeightConvertMapper<StdArc, StdArc, "
    "MyNonDefaultConstructibleWeightConverter>` via the mapper "
    "pointer constructor.");

//// Now test the case where the mapper cannot support compile-time expandedness
//   preservation. Check that when fed an `ExpandedFst`, it still cannot take
//   the optimization because of its mapper, due to `Properties()` not being
//   constexpr.
static_assert(
    !std::is_base_of_v<ExpandedFst<StdArc>,
                       decltype(ArcMapFst(VectorFst<StdArc>(),
                                          ToGallicMapper<StdArc>()))>,
    "ArcMapFst's CTAD deduction guides fail to correctly instantiate when "
    "performing a StdArc to ToGallicMapper<LogArc> mapping.");

// Mapper pointer constructor
static_assert(
    !std::is_base_of_v<
        ExpandedFst<StdArc>,
        decltype(ArcMapFst(VectorFst<StdArc>(),
                           static_cast<ToGallicMapper<StdArc>*>(nullptr)))>,
    "ArcMapFst's CTAD deduction guides fail to correctly instantiate when "
    "performing a LogArc to ToGallicMapper<LogArc> mapping via the mapper "
    "pointer constructor.");

// This tests adding a superfinal state.
TEST_F(ArcMapTest, SuperFinalMap) {
  SuperFinalMapper<StdArc> sfmapper;
  VectorFst<StdArc> nfst1;
  VectorFst<StdArc> nfst2;

  VectorFst<StdArc> vfst1(*mfst1_);
  VectorFst<StdArc> vfst2(*mfst1_);

  // Destructive tests.
  ArcMap(&nfst1, sfmapper);
  ASSERT_TRUE(Verify(nfst1));
  ASSERT_TRUE(Equal(nfst1, nfst2));

  ArcMap(&vfst1, sfmapper);
  ASSERT_TRUE(Verify(vfst1));
  ASSERT_TRUE(Equal(*mfst4_, vfst1));

  // Constructive tests.
  ArcMap(nfst2, &vfst2, sfmapper);
  ASSERT_TRUE(Verify(vfst2));
  ASSERT_TRUE(Equal(vfst2, nfst2));

  ArcMap(*mfst1_, &vfst2, sfmapper);
  ASSERT_TRUE(Verify(vfst2));
  ASSERT_TRUE(Equal(*mfst4_, vfst2));

  // On-the-fly tests.
  ArcMapFst<StdArc, StdArc, SuperFinalMapper<StdArc>> sffst1(nfst2, sfmapper);
  ASSERT_TRUE(Verify(sffst1));
  ASSERT_TRUE(Equal(sffst1, nfst2));

  ArcMapFst<StdArc, StdArc, SuperFinalMapper<StdArc>> sffst2(*mfst1_, sfmapper);
  ASSERT_TRUE(Verify(sffst2));
  ASSERT_TRUE(Equal(sffst2, *mfst5_));
}

// This tests input and output epsilon mappers.
TEST_F(ArcMapTest, EpsilonMap) {
  VectorFst<StdArc> vfst;

  InputEpsilonMapper<StdArc> imapper;
  ArcMap(*mfst1_, &vfst, imapper);
  ASSERT_TRUE(Verify(vfst));
  ASSERT_TRUE(Equal(*mfst13_, vfst));

  OutputEpsilonMapper<StdArc> omapper;
  ArcMap(*mfst1_, &vfst, omapper);
  ASSERT_TRUE(Verify(vfst));
  ASSERT_TRUE(Equal(*mfst14_, vfst));
}

// This tests power mappers.
TEST_F(ArcMapTest, PowerMap) {
  VectorFst<StdArc> vfst;
  ArcMap(*mfst1_, &vfst, PowerMapper<StdArc>(.5));
  ASSERT_TRUE(Verify(vfst));
  ASSERT_TRUE(Equal(*mfst15_, vfst));

  ArcMap(*mfst1_, &vfst, PowerMapper<StdArc>(2));
  ASSERT_TRUE(Verify(vfst));
  ASSERT_TRUE(Equal(*mfst16_, vfst));
}

TEST_F(ArcMapTest, IdentityMap) {
  VectorFst<StdArc> nfst;

  VectorFst<StdArc> vfst1(*mfst1_);
  VectorFst<StdArc> vfst2(nfst);

  IdentityArcMapper<StdArc> mapper;

  ArcMap(&vfst1, mapper);
  ASSERT_TRUE(Verify(vfst1));
  ASSERT_TRUE(Equal(*mfst1_, vfst1));

  ArcMap(&vfst2, mapper);
  ASSERT_TRUE(Verify(vfst2));
  ASSERT_TRUE(Equal(nfst, vfst2));

  VectorFst<StdArc> vfst3;
  ArcMap(*mfst1_, &vfst3, mapper);
  ASSERT_TRUE(Verify(vfst3));
  ASSERT_TRUE(Equal(*mfst1_, vfst3));

  ArcMapFst<StdArc, StdArc, IdentityArcMapper<StdArc>> mfst1(*mfst1_, mapper);
  ASSERT_TRUE(Verify(mfst1));
  ASSERT_TRUE(Equal(*mfst1_, mfst1));
}

}  // namespace
}  // namespace fst
