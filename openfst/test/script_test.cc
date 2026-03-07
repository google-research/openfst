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
// Tests of the scripting interface.

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "openfst/compat/init.h"
#include "gtest/gtest.h"
#include "absl/flags/flag.h"
#include "absl/strings/numbers.h"
#include "absl/strings/string_view.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/determinize.h"
#include "openfst/lib/encode.h"
#include "openfst/lib/properties.h"
#include "openfst/lib/symbol-table.h"
#include "openfst/lib/util.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/lib/weight.h"
#include "openfst/script/arc-class.h"
#include "openfst/script/arciterator-class.h"
#include "openfst/script/decode.h"
#include "openfst/script/encode.h"
#include "openfst/script/encodemapper-class.h"
#include "openfst/script/equal.h"
#include "openfst/script/fst-class.h"
#include "openfst/script/stateiterator-class.h"
#include "openfst/script/weight-class.h"

namespace fst {
namespace {

using ::fst::script::ArcClass;
using ::fst::script::ArcIteratorClass;
using ::fst::script::EncodeMapperClass;
using ::fst::script::FstClass;
using ::fst::script::MutableArcIteratorClass;
using ::fst::script::MutableFstClass;
using ::fst::script::StateIteratorClass;
using ::fst::script::VectorFstClass;
using ::fst::script::WeightClass;

class ClassTest : public testing::Test {
 protected:
  static constexpr char kDummyName[] = "nonexistent";
  static constexpr char kArcError[] = "Unknown arc type: nonexistent";
  static constexpr char kWeightError[] = "Unknown weight type: nonexistent";

  static constexpr std::array<absl::string_view, 3> kArcTypes = {
      {"standard", "log", "log64"}};
  static constexpr std::array<absl::string_view, 3> kWeightTypes = {
      {"tropical", "log", "log64"}};
  static constexpr std::array<uint32_t, 3> kEncodeFlags = {
      {kEncodeLabels, kEncodeWeights, kEncodeLabels | kEncodeWeights}};

  void SetUp() override {
    // Makes FSTERROR death tests actually fatal.
    absl::SetFlag(&FLAGS_fst_error_fatal, true);
  }
};

// Test the number-of-states accessors,
TEST_F(ClassTest, NumStatesMethods) {
  StdVectorFst vfst;
  FstClass f(vfst);
  auto ns = f.NumStatesIfKnown();
  EXPECT_TRUE(ns.has_value());
  EXPECT_EQ(ns, 0);
  MutableFstClass mf(vfst);
  EXPECT_EQ(mf.NumStates(), 0);
  StdDeterminizeFst dfst(vfst);
  FstClass df(dfst);
  EXPECT_EQ(df.NumStatesIfKnown(), std::nullopt);
}

// Tests arc mutation of MutableFstClass.
TEST_F(ClassTest, ArcMutation) {
  StdVectorFst vfst;
  MutableFstClass f(vfst);
  // Makes the "flower" FST accepting /[abc]*/.
  auto s = f.AddState();
  f.SetStart(s);
  f.SetFinal(s, WeightClass::One(f.WeightType()));
  f.ReserveArcs(s, 2);
  // Constructs ArcClass from actual arc.
  StdVectorFst::Arc arc(49, 49, StdVectorFst::Weight::One(), 0);
  f.AddArc(s, ArcClass(arc));
  // Constructs ArcClass directly.
  f.AddArc(s, ArcClass(50, 50, WeightClass::One(f.WeightType()), 0));
  ASSERT_EQ(2, f.NumArcs(s));
  // Then, clean it out.
  f.DeleteArcs(s);
  ASSERT_EQ(0, f.NumArcs(s));
}

// Tests construction and copying of FstClass.
TEST_F(ClassTest, FstClassCopy) {
  StdVectorFst vfst;
  FstClass f(vfst);
  FstClass c(f);
  ASSERT_TRUE(Equal(f, c));
}

// Tests construction and copying of MutableFstClass.
TEST_F(ClassTest, MutableFstClassCopy) {
  StdVectorFst vfst;
  MutableFstClass f(vfst);
  MutableFstClass c(f);
  ASSERT_TRUE(Equal(f, c));
}

// Tests that a read from a nonexistent test returns NULL.
TEST_F(ClassTest, ReactsCorrectlyToBadFilename) {
  std::unique_ptr<FstClass> f(FstClass::Read(std::string(kDummyName)));
  ASSERT_EQ(nullptr, f);
}

// Tests arc and state access and mutation of MutableFstClass.
TEST_F(ClassTest, ArcAndStateAccessAndMutation) {
  StdVectorFst vfst;
  MutableFstClass f(vfst);
  // Adds start state.
  auto s = f.AddState();
  f.SetStart(s);
  ASSERT_EQ(s, f.Start());
  // Marks it as final.
  f.SetFinal(s, WeightClass::One(f.WeightType()));
  ASSERT_EQ("0", f.Final(s).ToString());
  // Tests arc iteration.
  for (auto i = 0; i < 3; ++i)
    f.AddArc(s, ArcClass(i, i, WeightClass::One(f.WeightType()), s));
  ArcIteratorClass aiter1(f, s);
  for (auto i = 0; i < f.NumArcs(s); ++i) {
    auto arc = aiter1.Value();
    ASSERT_EQ(i, arc.ilabel);
    ASSERT_EQ(i, arc.olabel);
    aiter1.Next();
  }
  MutableArcIteratorClass aiter2(&f, s);
  ArcClass arc = aiter2.Value();
  char ch = 'a';
  arc.ilabel = ch;
  aiter2.SetValue(arc);
  ASSERT_EQ(ch, aiter2.Value().ilabel);
  // Tests simple state deletion.
  f.ReserveStates(4);
  ASSERT_EQ(1, f.NumStates());
  f.DeleteStates();
  ASSERT_EQ(0, f.NumStates());
  // Tests state iteration.
  f.AddState();  // Won't be deleted later.
  std::vector<int64_t> dstates;
  for (auto i = 0; i < 3; ++i) dstates.push_back(f.AddState());
  ASSERT_EQ(4, f.NumStates());
  StateIteratorClass siter(f);
  for (auto i = 0; i < f.NumStates(); ++i) {
    ASSERT_EQ(i, siter.Value());
    siter.Next();
  }
  ASSERT_TRUE(siter.Done());
  // Tests state deletion with a list.
  f.DeleteStates(dstates);
  ASSERT_EQ(1, f.NumStates());
}

// Tests symbol table access and mutation of MutableFstClass.
TEST_F(ClassTest, SymbolTableAccessAndMutation) {
  StdVectorFst vfst;
  MutableFstClass f(vfst);
  // Constructs symbol table and assigns it to MutableFstClass.
  std::unique_ptr<SymbolTable> syms = std::make_unique<SymbolTable>();
  syms->AddSymbol("a", 49);
  syms->AddSymbol("b", 50);
  f.SetInputSymbols(syms.get());
  f.SetOutputSymbols(syms.get());
  // Modifies symbol tables through the MutableFstClass.
  f.MutableInputSymbols()->AddSymbol("c", 51);
  f.MutableOutputSymbols()->AddSymbol("c", 51);
  ASSERT_EQ("c", f.InputSymbols()->Find(51));
  ASSERT_EQ("c", f.OutputSymbols()->Find(51));
  ASSERT_EQ(51, f.InputSymbols()->Find("c"));
  ASSERT_EQ(51, f.OutputSymbols()->Find("c"));
}

// Tests construction and copying of VectorFstClass.
TEST_F(ClassTest, VectorFstClassConstructAndCopy) {
  // Construction from concrete FST.
  StdVectorFst vfst;
  VectorFstClass f(vfst);
  VectorFstClass c(f);
  ASSERT_TRUE(Equal(f, c));
  // Construction from arc type string.
  for (const auto& arc_type : kArcTypes) {
    VectorFstClass vfc(arc_type);
    ASSERT_EQ(arc_type, vfc.ArcType());
    ASSERT_EQ(false, vfc.Properties(kError, true) && kError);
  }
  // Construct from a nonexistent arc type.
  ASSERT_DEATH(VectorFstClass vfc(kDummyName), kArcError);
}

// Tests WeightClass construction and comparison operations.
TEST_F(ClassTest, WeightClassOperations) {
  WeightClass w1;
  WeightClass w2;
  for (const auto& weight_type : kWeightTypes) {
    // Tests self-comparison of "normal" weights.
    w1 = WeightClass(weight_type, "1");
    w2 = WeightClass(weight_type, "1");
    ASSERT_EQ(w1, w2);
    // Tests comparison of different initializations of special weight types.
    w1 = WeightClass::Zero(weight_type);
    w2 = WeightClass(weight_type, "Infinity");
    ASSERT_EQ(w1, w2);
    w1 = WeightClass::One(weight_type);
    w2 = WeightClass(weight_type, "0");
    ASSERT_EQ(w1, w2);
    // Tests NoWeight.
    w1 = WeightClass::NoWeight(weight_type);
    w2 = WeightClass::NoWeight(weight_type);
    ASSERT_EQ(w1.ToString(), w2.ToString());
  }
  // Death tests.
  ASSERT_DEATH(WeightClass(kDummyName, "1"), kWeightError);
  ASSERT_DEATH(WeightClass::Zero(kDummyName), kWeightError);
  ASSERT_DEATH(WeightClass::One(kDummyName), kWeightError);
  ASSERT_DEATH(WeightClass::NoWeight(kDummyName), kWeightError);
  // Arithmetic tests.
  w1 = WeightClass::One("tropical");
  w2 = WeightClass("tropical", "2");
  ASSERT_EQ(w1, Plus(w1, w2));
  ASSERT_EQ(w2, Times(w1, w2));
  w1 = WeightClass("log", "0.69314718");  // -log(.5)
  w2 = WeightClass(w1);
  float one, one_sum;
  ASSERT_TRUE(absl::SimpleAtof(WeightClass::One("log").ToString(), &one));
  ASSERT_TRUE(absl::SimpleAtof(Plus(w1, w2).ToString(), &one_sum));
  // Same logic as ApproxEqual.
  ASSERT_TRUE(one <= one_sum + kDelta && one_sum <= one + kDelta);
}

TEST_F(ClassTest, EncodeMapperClassOperations) {
  // Tests using EncodeMapperClass to encode individual arcs.
  for (const auto& arc_type : kArcTypes) {
    for (const auto encode_flags : kEncodeFlags) {
      EncodeMapperClass encoder(arc_type, encode_flags, ENCODE);
      ArcClass arc('a', 'b', WeightClass(encoder.WeightType(), "2"), 1);
      ArcClass unused_earc = encoder(arc);
    }
  }
  // Tests roundtrip use of an EncodeMapperClass encoder.
  StdVectorFst vfst;  // Will accept /baa+/ with exponential decay on the plus.
  auto src = vfst.AddState();
  auto dst = vfst.AddState();
  vfst.SetStart(src);
  vfst.AddArc(src, StdArc('b', 'b', StdArc::Weight::One(), dst));
  src = dst;
  dst = vfst.AddState();
  vfst.AddArc(src, StdArc('a', 'a', StdArc::Weight::One(), dst));
  src = dst;
  dst = vfst.AddState();
  vfst.AddArc(src, StdArc('a', 'a', StdArc::Weight::One(), dst));
  vfst.SetFinal(dst, StdArc::Weight::One());
  vfst.AddArc(dst, StdArc('a', 'a', StdArc::Weight(.001), dst));
  VectorFstClass vfstc(vfst);
  VectorFstClass vfstc_copy(vfstc);  // Keeps around an unencoded copy.
  EncodeMapperClass encoder(vfstc.ArcType(), kEncodeLabels | kEncodeWeights,
                            ENCODE);
  Encode(&vfstc, &encoder);
  Decode(&vfstc, encoder);
  ASSERT_TRUE(Equal(vfstc_copy, vfstc));
}

}  // namespace
}  // namespace fst
