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
#include <fstream>
#include <istream>
#include <limits>
#include <memory>
#include <optional>
#include <sstream>
#include <streambuf>
#include <string>
#include <utility>
#include <vector>

#include "gtest/gtest.h"
#include "absl/flags/flag.h"
#include "absl/flags/reflection.h"
#include "absl/strings/numbers.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/const-fst.h"
#include "openfst/lib/determinize.h"
#include "openfst/lib/encode.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/properties.h"
#include "openfst/lib/symbol-table.h"
#include "openfst/lib/util.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/lib/weight.h"
#include "openfst/script/arc-class.h"
#include "openfst/script/arciterator-class.h"
#include "openfst/script/arg-packs.h"
#include "openfst/script/compile-impl.h"
#include "openfst/script/compile.h"
#include "openfst/script/decode.h"
#include "openfst/script/encode.h"
#include "openfst/script/encodemapper-class.h"
#include "openfst/script/equal.h"
#include "openfst/script/fst-class.h"
#include "openfst/script/stateiterator-class.h"
#include "openfst/script/text-io.h"
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

  absl::FlagSaver fs_;
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
  // Delete one arc, then clean out the rest.
  EXPECT_TRUE(f.DeleteArcs(s, 1));
  ASSERT_EQ(1, f.NumArcs(s));
  EXPECT_TRUE(f.DeleteArcs(s));
  ASSERT_EQ(0, f.NumArcs(s));
  // Deleting more arcs than present clears all arcs and epsilon counts.
  f.AddArc(s, ArcClass(0, 0, WeightClass::One(f.WeightType()), 0));
  ASSERT_EQ(1, f.NumArcs(s));
  ASSERT_EQ(1, f.NumInputEpsilons(s));
  ASSERT_EQ(1, f.NumOutputEpsilons(s));
  EXPECT_TRUE(f.DeleteArcs(s, 5));
  ASSERT_EQ(0, f.NumArcs(s));
  EXPECT_EQ(0, f.NumInputEpsilons(s));
  EXPECT_EQ(0, f.NumOutputEpsilons(s));
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
  std::unique_ptr<const FstClass> f(FstClass::Read(std::string(kDummyName)));
  ASSERT_EQ(nullptr, f);
}

// Tests reading a non-mutable FST with convert=true into a MutableFstClass.
TEST_F(ClassTest, MutableFstClassReadConvert) {
  StdVectorFst vfst;
  const auto s0 = vfst.AddState();
  const auto s1 = vfst.AddState();
  vfst.SetStart(s0);
  vfst.AddArc(s0, StdArc(1, 2, StdArc::Weight(0.5f), s1));
  vfst.SetFinal(s1, StdArc::Weight(1.5f));

  const std::string const_path =
      absl::StrCat(::testing::TempDir(), "/const.fst");
  const StdConstFst cfst(vfst);
  ASSERT_TRUE(cfst.Write(const_path));

  std::unique_ptr<MutableFstClass> converted =
      MutableFstClass::Read(const_path, /*convert=*/true);
  ASSERT_NE(converted, nullptr);
  EXPECT_EQ(converted->FstType(), "vector");
  EXPECT_TRUE(Equal(*converted, FstClass(vfst)));
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
  {
    const absl::FlagSaver fs;
    absl::SetFlag(&FLAGS_fst_error_fatal, false);
    f.ReserveStates(-1);
    f.ReserveStates(std::numeric_limits<int64_t>::min());
  }
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

  // Tests null-impl WeightClass behavior (default-constructed, unknown type, or
  // type-mismatched arithmetic in non-fatal mode).
  WeightClass empty;
  EXPECT_EQ(empty.GetWeight<StdArc::Weight>(), nullptr);
  {
    std::ostringstream ostrm;
    ostrm << empty;
    EXPECT_EQ(ostrm.str(), "none");
  }
  absl::SetFlag(&FLAGS_fst_error_fatal, false);
  for (const WeightClass& invalid :
       {WeightClass(kDummyName, "1"),
        Plus(WeightClass::One("tropical"), WeightClass::One("log")),
        Times(WeightClass::One("tropical"), WeightClass::One("log")),
        Divide(WeightClass::One("tropical"), WeightClass::One("log"))}) {
    EXPECT_EQ(invalid.GetWeight<StdArc::Weight>(), nullptr);
    std::ostringstream ostrm;
    ostrm << invalid;
    EXPECT_EQ(ostrm.str(), "none");
  }
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

  // Death test for unknown arc type in fatal mode.
  ASSERT_DEATH(EncodeMapperClass(kDummyName, kEncodeLabels, ENCODE), kArcError);

  // Tests null-impl EncodeMapperClass behavior (default-constructed or unknown
  // arc type in non-fatal mode).
  absl::SetFlag(&FLAGS_fst_error_fatal, false);
  for (EncodeMapperClass invalid :
       {EncodeMapperClass(),
        EncodeMapperClass(kDummyName, kEncodeLabels, ENCODE)}) {
    EXPECT_EQ(invalid.ArcType(), "none");
    EXPECT_EQ(invalid.WeightType(), "none");
    EXPECT_EQ(invalid.Flags(), 0);
    EXPECT_EQ(invalid.Properties(0), kError);
    EXPECT_EQ(invalid.Type(), ENCODE);
    EXPECT_EQ(invalid.InputSymbols(), nullptr);
    EXPECT_EQ(invalid.OutputSymbols(), nullptr);
    SymbolTable syms;
    invalid.SetInputSymbols(&syms);
    invalid.SetOutputSymbols(&syms);
    EXPECT_EQ(invalid.InputSymbols(), nullptr);
    EXPECT_EQ(invalid.OutputSymbols(), nullptr);
    EXPECT_EQ(invalid.GetEncodeMapper<StdArc>(), nullptr);
    const EncodeMapperClass& const_invalid = invalid;
    EXPECT_EQ(const_invalid.GetEncodeMapper<StdArc>(), nullptr);
    std::ostringstream ostrm;
    EXPECT_FALSE(invalid.Write(ostrm, "mem"));
    const ArcClass mapped =
        invalid(ArcClass(1, 2, WeightClass::One("tropical"), 3));
    EXPECT_EQ(mapped.ilabel, kNoLabel);
    EXPECT_EQ(mapped.olabel, kNoLabel);
    EXPECT_EQ(mapped.weight.Type(), "none");
    EXPECT_EQ(mapped.weight.ToString(), "none");
    EXPECT_EQ(mapped.nextstate, 3);
    Encode(&vfstc, &invalid);
    Decode(&vfstc, invalid);
  }

  // Tests reading an EncodeMapperClass from a non-zero stream offset and from
  // a non-seekable stream (such as a pipe or stdin).
  Encode(&vfstc, &encoder);
  std::ostringstream serialized;
  ASSERT_TRUE(encoder.Write(serialized, "mem"));
  const std::string mapper_bytes = serialized.str();

  {
    const std::string kPrefix = "PREFIX_HEADER";
    std::istringstream istrm(absl::StrCat(kPrefix, mapper_bytes));
    std::string prefix(kPrefix.size(), '\0');
    ASSERT_TRUE(istrm.read(prefix.data(), prefix.size()));
    std::unique_ptr<EncodeMapperClass> read_mapper =
        EncodeMapperClass::Read(istrm, "offset_stream");
    ASSERT_NE(read_mapper, nullptr);
    EXPECT_EQ(read_mapper->ArcType(), encoder.ArcType());
    EXPECT_EQ(read_mapper->Flags(), encoder.Flags());
    VectorFstClass decoded(vfstc);
    Decode(&decoded, *read_mapper);
    EXPECT_TRUE(Equal(vfstc_copy, decoded));
  }

  {
    class NonSeekableInBuf : public std::streambuf {
     public:
      explicit NonSeekableInBuf(std::string data) : data_(std::move(data)) {
        setg(data_.data(), data_.data(), data_.data() + data_.size());
      }

     private:
      std::string data_;
    };
    NonSeekableInBuf buf(mapper_bytes);
    std::istream non_seekable_strm(&buf);
    std::unique_ptr<EncodeMapperClass> read_mapper =
        EncodeMapperClass::Read(non_seekable_strm, "pipe_stream");
    ASSERT_NE(read_mapper, nullptr);
    VectorFstClass decoded(vfstc);
    Decode(&decoded, *read_mapper);
    EXPECT_TRUE(Equal(vfstc_copy, decoded));
  }
}

// Applying an `ArcClass` whose weight type does not match the FST or encoder
// is an error rather than a dereference of a null weight.
TEST_F(ClassTest, ArcClassWeightTypeMismatch) {
  absl::SetFlag(&FLAGS_fst_error_fatal, false);
  StdVectorFst vfst;
  MutableFstClass f(vfst);
  const auto s = f.AddState();
  f.AddArc(s, ArcClass(1, 1, WeightClass::One(f.WeightType()), s));
  const ArcClass log_arc(2, 2, WeightClass::One("log"), s);

  // `SetValue` leaves the arc unchanged.
  MutableArcIteratorClass aiter(&f, s);
  aiter.SetValue(log_arc);
  const ArcClass arc = aiter.Value();
  EXPECT_EQ(arc.ilabel, 1);
  EXPECT_EQ(arc.olabel, 1);
  EXPECT_EQ(arc.weight, WeightClass::One(f.WeightType()));

  // The encoder returns the same error arc as `EncodeMapper` on failure.
  EncodeMapperClass encoder(f.ArcType(), kEncodeLabels, ENCODE);
  const ArcClass encoded = encoder(log_arc);
  EXPECT_EQ(encoded.ilabel, kNoLabel);
  EXPECT_EQ(encoded.olabel, kNoLabel);
  EXPECT_EQ(encoded.weight.ToString(), "BadNumber");
  EXPECT_EQ(encoded.nextstate, s);
}

TEST_F(ClassTest, WithReturnValueDefaultInitializesScalarRetval) {
  const int arg = 42;
  const script::WithReturnValue<bool, int> bool_args(arg);
  EXPECT_FALSE(bool_args.retval);

  // When `Apply` fails to find an operation for an unregistered arc type in
  // non-fatal mode, `Equal` must return `false` rather than an uninitialized
  // `bool`.
  absl::SetFlag(&FLAGS_fst_error_fatal, false);
  const VectorFst<GallicArc<StdArc>> unregistered_fst;
  const FstClass unregistered_fstc(unregistered_fst);
  EXPECT_FALSE(Equal(unregistered_fstc, unregistered_fstc));
}

TEST(CompileTest, FstCompilerCompilesTextWithWeights) {
  const std::string text =
      "0 1 1 2 0.5\n"
      "1 2 3 4 1.5\n"
      "2 2.5\n";
  std::istringstream istrm(text);
  FstCompiler<StdArc> compiler(istrm, "test", nullptr, nullptr, nullptr,
                               /*accep=*/false, /*ikeep=*/false,
                               /*okeep=*/false,
                               /*nkeep=*/false);
  const auto& fst = compiler.Fst();
  EXPECT_FALSE(fst.Properties(kError, true));
  EXPECT_EQ(fst.NumStates(), 3);
  EXPECT_EQ(fst.Start(), 0);
  EXPECT_FLOAT_EQ(fst.Final(2).Value(), 2.5f);
}

TEST(CompileTest, FstCompilerBadWeightSetsError) {
  const absl::FlagSaver fs;
  absl::SetFlag(&FLAGS_fst_error_fatal, false);
  const std::string text = "0 1 1 2 not_a_weight\n";
  std::istringstream istrm(text);
  FstCompiler<StdArc> compiler(istrm, "test", nullptr, nullptr, nullptr,
                               /*accep=*/false, /*ikeep=*/false,
                               /*okeep=*/false,
                               /*nkeep=*/false);
  const auto& fst = compiler.Fst();
  EXPECT_TRUE(fst.Properties(kError, true));
}

TEST(CompileTest, FstCompilerBadIntegersSetError) {
  const absl::FlagSaver fs;
  absl::SetFlag(&FLAGS_fst_error_fatal, false);
  for (const bool nkeep : {false, true}) {
    for (const char* text :
         {"0 1 x 2\n", "0 1 1 x\n", "x 1 1 2\n", "0 x 1 2\n", "-1 0 1 2\n"}) {
      // Without nkeep, negative state IDs are renumbered densely.
      if (!nkeep && text[0] == '-') continue;
      std::istringstream istrm(text);
      FstCompiler<StdArc> compiler(istrm, "test", nullptr, nullptr, nullptr,
                                   /*accep=*/false, /*ikeep=*/false,
                                   /*okeep=*/false, nkeep);
      EXPECT_TRUE(compiler.Fst().Properties(kError, true))
          << "text = " << text << ", nkeep = " << nkeep;
    }
  }
}

TEST(CompileTest, ScriptCompileHandlesNullAndWriteFailure) {
  const absl::FlagSaver fs;
  absl::SetFlag(&FLAGS_fst_error_fatal, false);
  const std::string kTextFst = "0 1 1 2\n1\n";
  const std::string out_path =
      absl::StrCat(::testing::TempDir(), "/compiled.fst");
  const std::string bad_out_path =
      absl::StrCat(::testing::TempDir(), "/nonexistent_dir/compiled.fst");

  {
    std::istringstream istrm(kTextFst);
    EXPECT_FALSE(script::Compile(istrm, "test", out_path, "vector",
                                 "nonexistent", nullptr, nullptr, nullptr,
                                 /*accep=*/false, /*ikeep=*/false,
                                 /*okeep=*/false, /*nkeep=*/false));
  }
  {
    std::istringstream istrm(kTextFst);
    EXPECT_FALSE(script::Compile(istrm, "test", out_path, "nonexistent",
                                 "standard", nullptr, nullptr, nullptr,
                                 /*accep=*/false, /*ikeep=*/false,
                                 /*okeep=*/false, /*nkeep=*/false));
  }
  {
    std::istringstream istrm(kTextFst);
    EXPECT_FALSE(script::Compile(istrm, "test", bad_out_path, "vector",
                                 "standard", nullptr, nullptr, nullptr,
                                 /*accep=*/false, /*ikeep=*/false,
                                 /*okeep=*/false, /*nkeep=*/false));
  }
  {
    std::istringstream istrm(kTextFst);
    EXPECT_TRUE(script::Compile(istrm, "test", out_path, "vector", "standard",
                                nullptr, nullptr, nullptr,
                                /*accep=*/false, /*ikeep=*/false,
                                /*okeep=*/false, /*nkeep=*/false));
    std::unique_ptr<FstClass> fst(FstClass::Read(out_path));
    ASSERT_NE(fst, nullptr);
    EXPECT_EQ(fst->NumStatesIfKnown(), 2);
  }
}

TEST(TextIoTest, ReadPotentialsOutOfOrderRepeatedAndInvalidStateIds) {
  const std::string path = ::testing::TempDir() + "/potentials.txt";
  {
    std::ofstream ostrm(path);
    ASSERT_TRUE(ostrm.is_open());
    ostrm << "2 1.5\n"
          << "0 0.5\n"
          << "2 2.5\n";
  }
  std::vector<WeightClass> potentials;
  ASSERT_TRUE(script::ReadPotentials("tropical", path, &potentials));
  ASSERT_EQ(potentials.size(), 3);
  EXPECT_EQ(potentials[0], WeightClass("tropical", "0.5"));
  EXPECT_EQ(potentials[1], WeightClass::Zero("tropical"));
  EXPECT_EQ(potentials[2], WeightClass("tropical", "2.5"));

  const absl::FlagSaver fs;
  absl::SetFlag(&FLAGS_fst_error_fatal, false);
  for (const char* bad_line : {"-1 0.5\n", "not_an_int 0.5\n"}) {
    {
      std::ofstream ostrm(path);
      ASSERT_TRUE(ostrm.is_open());
      ostrm << bad_line;
    }
    EXPECT_FALSE(script::ReadPotentials("tropical", path, &potentials))
        << "bad_line = " << bad_line;
  }
}

}  // namespace
}  // namespace fst
