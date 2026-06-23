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

#include "openfst/test/fst_from_tuples.h"

#include <initializer_list>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/strings/string_view.h"
#include "openfst/lib/equal.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/symbol-table.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/compat/status_matchers.h"

namespace fst {
namespace {

SymbolTable CreateSymbolTable(
    std::initializer_list<absl::string_view> symbols) {
  SymbolTable symbol_table;
  for (const auto& s : symbols) symbol_table.AddSymbol(s);
  return symbol_table;
}

TEST(CreateFstFromTuplesTest, Empty) {
  const StdVectorFst fst = (CreateFstFromTuples<StdVectorFst>({}, {}));
  EXPECT_EQ(fst::kNoStateId, fst.Start());
  EXPECT_EQ(0, fst.NumStates());
}

TEST(CreateFstFromTuplesTest, OneArc) {
  const StdVectorFst fst = CreateFstFromTuples<StdVectorFst>(
      {{0, 0, 1, 2, 3.0}}, {{0, StdArc::Weight::One()}});
  EXPECT_EQ(0, fst.Start());
  EXPECT_EQ(1, fst.NumStates());
  fst::ArcIterator<StdVectorFst> aiter(fst, fst.Start());
  ASSERT_FALSE(aiter.Done());
  const StdArc& arc = aiter.Value();
  EXPECT_EQ(1, arc.ilabel);
  EXPECT_EQ(2, arc.olabel);
  EXPECT_EQ(0, arc.nextstate);
  EXPECT_EQ(StdArc::Weight(3.0), arc.weight);
  EXPECT_EQ(StdArc::Weight::One(), fst.Final(fst.Start()));
}

TEST(CreateFstFromTuplesTest, Arcs) {
  const StdVectorFst fst = CreateFstFromTuples<StdVectorFst>(
      {{5, 6, 7, 8, 10.0}, {5, 1, 2, 3, 4.0}, {0, 1, 2, 3, 4.0}},
      {{1, StdArc::Weight::One()}});
  EXPECT_EQ(5, fst.Start());
  EXPECT_EQ(7, fst.NumStates());
  EXPECT_EQ(2, fst.NumArcs(5));
  EXPECT_EQ(StdArc::Weight::One(), fst.Final(1));
}

TEST(CreateFstFromTuplesTest, AcceptorLabel) {
  const StdVectorFst fst1 = CreateFstFromTuples<StdVectorFst>(
      {{5, 6, 7, 8, 10.0}, {5, 1, 2, 2, 4.0}, {0, 1, 2, 3, 4.0}},
      {{1, StdArc::Weight::One()}});
  const StdVectorFst fst2 = CreateFstFromTuples<StdVectorFst>(
      {{5, 6, 7, 8, 10.0},
       {5, 1, AcceptorLabel<StdArc>(2), 4.0},
       {0, 1, 2, 3, 4.0}},
      {{1, StdArc::Weight::One()}});
  EXPECT_TRUE(fst::Equal(fst1, fst2));
}

TEST(CreateFstFromTuplesTest, ArcWeight) {
  using W = StdArc::Weight;
  const StdVectorFst fst = CreateFstFromTuples<StdVectorFst>(
      {{5, 6, 7, 8, W(10.0)}, {5, 1, 2, 3, W(4.0)}, {0, 1, 2, 3, W(4.0)}},
      {{1, StdArc::Weight::One()}});
  EXPECT_EQ(5, fst.Start());
  EXPECT_EQ(7, fst.NumStates());
  EXPECT_EQ(2, fst.NumArcs(5));
  EXPECT_EQ(StdArc::Weight::One(), fst.Final(1));
}

TEST(CreateFstFromTuplesTest, Final) {
  const StdVectorFst fst = CreateFstFromTuples<StdVectorFst>(
      {{10, 0, 0, 0, 0}}, {{1, 0.1}, {4, 0.4}, {7, 0.7}});
  EXPECT_EQ(11, fst.NumStates());
  for (int i = 0; i < fst.NumStates(); ++i) {
    if (i == 1 || i == 4 || i == 7)
      EXPECT_FLOAT_EQ(i / 10.0, fst.Final(i).Value());
    else
      EXPECT_EQ(StdArc::Weight::Zero(), fst.Final(i));
  }
}

TEST(CreateFstFromTuplesTest, NoArc) {
  const StdVectorFst fst =
      CreateFstFromTuples<StdVectorFst>({}, {{1, 0.1}, {4, 0.4}, {7, 0.7}});
  EXPECT_EQ(8, fst.NumStates());
  for (int i = 0; i < fst.NumStates(); ++i) {
    if (i == 1 || i == 4 || i == 7)
      EXPECT_FLOAT_EQ(i / 10.0, fst.Final(i).Value());
    else
      EXPECT_EQ(StdArc::Weight::Zero(), fst.Final(i));
  }
  EXPECT_EQ(1, fst.Start());
}

TEST(CreateFstFromTuplesTest, SymbolicConstructorOneSymbolTable) {
  const SymbolTable symbols = CreateSymbolTable({"a", "b", "c"});
  ABSL_ASSERT_OK_AND_ASSIGN(
      const StdVectorFst symbolic_fst,
      CreateFstFromTuples<StdVectorFst>(
          {{0, 1, "a", "b", 1.0}, {1, 2, "b", "c", 2.0}}, {{2, 5.0}}, symbols));
  const StdVectorFst numeric_fst = CreateFstFromTuples<StdVectorFst>(
      {{0, 1, 0, 1, 1.0}, {1, 2, 1, 2, 2.0}}, {{2, 5.0}});
  EXPECT_TRUE(fst::Equal(symbolic_fst, numeric_fst));
}

TEST(CreateFstFromTuplesTest, SymbolicConstructorSeparateInOutSymbolTables) {
  const SymbolTable isymbols = CreateSymbolTable({"a", "b", "c"});
  const SymbolTable osymbols = CreateSymbolTable({"d", "e", "f"});
  ABSL_ASSERT_OK_AND_ASSIGN(
      const StdVectorFst symbolic_fst,
      CreateFstFromTuples<StdVectorFst>(
          {{0, 1, "a", "e", 1.0}, {1, 2, "b", "f", 2.0}},
          {{2, 5.0}}, isymbols, osymbols));
  using Label = StdVectorFst::Arc::Label;
  const StdVectorFst numeric_fst = CreateFstFromTuples<StdVectorFst>(
      {{0, 1, Label(isymbols.Find("a")), Label(osymbols.Find("e")), 1.0},
       {1, 2, Label(isymbols.Find("b")), Label(osymbols.Find("f")), 2.0}},
      {{2, 5.0}});
  EXPECT_TRUE(fst::Equal(symbolic_fst, numeric_fst));
}

}  // namespace
}  // namespace fst
