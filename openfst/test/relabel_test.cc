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
// Unit test for Relabel.

#include "openfst/lib/relabel.h"

#include <memory>
#include <sstream>
#include <string>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/equal.h"
#include "openfst/lib/properties.h"
#include "openfst/lib/symbol-table.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/lib/verify.h"
#include "openfst/script/compile-impl.h"

namespace fst {
namespace {

using Arc = StdArc;

class RelabelTest : public testing::Test {
 protected:
  void SetUp() override {
    const std::string path =
        JoinPath(std::string("."),
                       "openfst/test/testdata/relabel");
    const std::string relabel1_name = JoinPath(path, "r1.fst");
    const std::string relabel2_name = JoinPath(path, "r2.fst");
    const std::string relabel3_name = JoinPath(path, "r3.fst");

    // p1: base.
    pfst1_.reset(VectorFst<Arc>::Read(relabel1_name));
    // p2: p1 + relabeled input.
    pfst2_.reset(VectorFst<Arc>::Read(relabel2_name));
    // p3:  p1 + relabeled output.
    pfst3_.reset(VectorFst<Arc>::Read(relabel3_name));
  }

  std::unique_ptr<const VectorFst<Arc>> pfst1_;
  std::unique_ptr<const VectorFst<Arc>> pfst2_;
  std::unique_ptr<const VectorFst<Arc>> pfst3_;
};

TEST_F(RelabelTest, Relabel) {
  VectorFst<Arc> nfst;

  VectorFst<Arc> vfst1(*pfst1_);
  VectorFst<Arc> vfst2(*pfst1_);

  Relabel(&vfst1, pfst2_->InputSymbols(), nullptr);
  ASSERT_TRUE(Verify(vfst1));
  ASSERT_TRUE(Equal(*pfst2_, vfst1));

  Relabel(&nfst, pfst2_->InputSymbols(), nullptr);
  ASSERT_TRUE(Verify(nfst));
  ASSERT_TRUE(nfst.Properties(kAcceptor, true));

  Relabel(&vfst2, nullptr, pfst3_->OutputSymbols());
  ASSERT_TRUE(Verify(vfst2));
  ASSERT_TRUE(Equal(*pfst3_, vfst2));

  Relabel(&nfst, nullptr, pfst3_->OutputSymbols());
  ASSERT_TRUE(Verify(nfst));
  ASSERT_TRUE(nfst.Properties(kAcceptor, true));
}

TEST_F(RelabelTest, RelabelFst) {
  VectorFst<Arc> nfst;

  RelabelFst<Arc> dfst1(*pfst1_, pfst2_->InputSymbols(), nullptr);
  ASSERT_TRUE(Verify(dfst1));
  ASSERT_TRUE(Equal(*pfst2_, dfst1));

  RelabelFst<Arc> ndfst1(nfst, pfst2_->InputSymbols(), nullptr);
  ASSERT_TRUE(Verify(ndfst1));
  ASSERT_TRUE(Equal(nfst, ndfst1));

  RelabelFst<Arc> dfst2(*pfst1_, nullptr, pfst3_->OutputSymbols());
  ASSERT_TRUE(Verify(dfst2));
  ASSERT_TRUE(Equal(*pfst3_, dfst2));

  RelabelFst<Arc> ndfst2(nfst, nullptr, pfst3_->OutputSymbols());
  ASSERT_TRUE(Verify(ndfst2));
  ASSERT_TRUE(Equal(nfst, ndfst2));

  for (const bool safe : {false, true}) {
    RelabelFst<Arc> cfst(dfst1, safe);
    ASSERT_TRUE(Verify(cfst));
    ASSERT_TRUE(Equal(*pfst2_, cfst));
  }
}

// The target symbol table is missing a symbol, but that's ok because
// the FST doesn't use it.
TEST_F(RelabelTest, RelabelFstUnusedSymbols) {
  std::istringstream sym_file(
      "<epsilon> 0\n"
      "a 1\n"
      "b 2\n"
      "c 3\n"
      "d 4\n");
  std::istringstream new_syms_file(
      "<epsilon> 0\n"
      "a 1\n"
      "b 2\n"
      "c 3\n");
  std::istringstream fst_file(
      "1 2 a a\n"
      "2 3 b b\n"
      "3 4 c c\n"
      "4\n");

  std::unique_ptr<SymbolTable> syms(
      SymbolTable::ReadText(sym_file, "unused-orig"));
  FstCompiler<Arc> fst(fst_file, "", syms.get(), syms.get(), nullptr, false,
                       true, true, false);
  const VectorFst<Arc>& nfst = fst.Fst();

  std::unique_ptr<SymbolTable> newsyms(
      SymbolTable::ReadText(new_syms_file, "unused-new"));
  VectorFst<Arc> tmpfst(nfst);
  Relabel(&tmpfst, newsyms.get(), newsyms.get());
  EXPECT_TRUE(tmpfst.InputSymbols()->NumSymbols() == newsyms->NumSymbols());
  EXPECT_TRUE(tmpfst.OutputSymbols()->NumSymbols() == newsyms->NumSymbols());
}

// The target symbol table is missing a symbol, but that's ok because
// we set the unknown symbol.
TEST_F(RelabelTest, RelabelFstUnknownSymbols) {
  std::istringstream sym_file(
      "<epsilon> 0\n"
      "a 1\n"
      "b 2\n"
      "c 3\n");
  std::istringstream new_syms_file(
      "<epsilon> 0\n"
      "a 1\n"
      "b 2\n"
      "unk 3\n");
  std::istringstream fst_file(
      "1 2 a a\n"
      "2 3 b b\n"
      "3 4 c c\n"
      "4\n");

  std::unique_ptr<SymbolTable> syms(
      SymbolTable::ReadText(sym_file, "unused-orig"));
  FstCompiler<Arc> fst(fst_file, "", syms.get(), syms.get(), nullptr, false,
                       true, true, false);
  const VectorFst<Arc>& nfst = fst.Fst();

  std::unique_ptr<SymbolTable> newsyms(
      SymbolTable::ReadText(new_syms_file, "unused-new"));
  VectorFst<Arc> tmpfst(nfst);
  Relabel(&tmpfst, tmpfst.InputSymbols(), newsyms.get(), "unk", true,
          tmpfst.OutputSymbols(), newsyms.get(), "unk", true);
  EXPECT_TRUE(tmpfst.InputSymbols()->NumSymbols() == newsyms->NumSymbols());
  EXPECT_TRUE(tmpfst.OutputSymbols()->NumSymbols() == newsyms->NumSymbols());
}

TEST_F(RelabelTest, ArcIterator) {
  // Only validates that ArcIterator<RelabelFst<StdArc> is constructable.
  StdVectorFst fst;
  fst.AddState();
  RelabelFst<StdArc> relabel_fst(fst, {}, {});
  ArcIterator<RelabelFst<StdArc>> arc_iter(relabel_fst, 0);
  EXPECT_TRUE(arc_iter.Done());
}

}  // namespace
}  // namespace fst
