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
// Unit test for SymbolTable operations.

#include "openfst/lib/symbol-table-ops.h"

#include <memory>
#include <string>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "openfst/lib/relabel.h"
#include "openfst/lib/symbol-table.h"
#include "openfst/lib/vector-fst.h"

namespace fst {
namespace {

class SymbolTableOpsTest : public testing::Test {
 protected:
  void SetUp() override {
    const std::string path = JoinPath(
        std::string("."),
        "openfst/test/testdata/symbol-table-ops");
    t1_fst_name_ = JoinPath(path, "t1.fst");
    t2_fst_name_ = JoinPath(path, "t2.fst");
    t3_fst_name_ = JoinPath(path, "t3.fst");
    t1_map_name_ = JoinPath(path, "t1.map");
    t2_map_name_ = JoinPath(path, "t2.map");
  }

  std::string t1_fst_name_;
  std::string t2_fst_name_;
  std::string t3_fst_name_;
  std::string t1_map_name_;
  std::string t2_map_name_;
};

// This test the merging of two symbol tables, where conflicts are resolved
// by repositioning values taken from the right table higher.
TEST_F(SymbolTableOpsTest, MergeSymbolTableTest) {
  std::unique_ptr<const SymbolTable> left(SymbolTable::ReadText(t1_map_name_));
  std::unique_ptr<const SymbolTable> right(SymbolTable::ReadText(t2_map_name_));

  bool relabel;
  std::unique_ptr<SymbolTable> merged(
      MergeSymbolTable(*left, *right, &relabel));

  EXPECT_EQ(merged->Find("a"), 2);
  EXPECT_EQ(merged->Find("b"), 3);
  EXPECT_EQ(merged->Find("z"), 5);
  EXPECT_EQ(relabel, true);
}

TEST_F(SymbolTableOpsTest, MergeSymbolTableTestSubsets) {
  bool relabel;
  std::unique_ptr<SymbolTable> full = std::make_unique<SymbolTable>("full");
  full->AddSymbol("a");
  full->AddSymbol("b");
  full->AddSymbol("c");
  std::unique_ptr<SymbolTable> conflict = std::make_unique<SymbolTable>("left");
  conflict->AddSymbol("a");
  conflict->AddSymbol("c");
  std::unique_ptr<SymbolTable> subset = std::make_unique<SymbolTable>("subset");
  subset->AddSymbol("a");
  subset->AddSymbol("b");
  // Checks if right is a proper subset, but right needs relabeling.
  std::unique_ptr<SymbolTable> merged(
      MergeSymbolTable(*full, *conflict, &relabel));
  // We use EXPECT_TRUE here, not EXPECT_EQ, because the strings are not
  // human readable when serialized.
  EXPECT_TRUE(full->LabeledCheckSum() == merged->LabeledCheckSum());
  EXPECT_EQ(relabel, true);
  // Tests left is subset, no relabeling required.
  merged.reset(MergeSymbolTable(*subset, *full, &relabel));
  EXPECT_TRUE(full->LabeledCheckSum() == merged->LabeledCheckSum());
  EXPECT_EQ(relabel, false);
  // Checks if right is a proper subset, but no relabeling is required.
  merged.reset(MergeSymbolTable(*full, *subset, &relabel));
  EXPECT_TRUE(full->LabeledCheckSum() == merged->LabeledCheckSum());
  EXPECT_EQ(relabel, false);
  // Tests left is subset, but relabeleling required.
  merged.reset(MergeSymbolTable(*conflict, *full, &relabel));
  EXPECT_TRUE(full->LabeledCheckSum() == merged->LabeledCheckSum());
  EXPECT_EQ(relabel, true);
}

// This tests crunches the numbering of symbol table, removing holes.
TEST_F(SymbolTableOpsTest, CompactSymbolTableTest) {
  std::unique_ptr<const SymbolTable> st(SymbolTable::ReadText(t1_map_name_));

  std::unique_ptr<const SymbolTable> compact(CompactSymbolTable(*st));

  EXPECT_EQ(compact->Find("a"), 1);
  EXPECT_EQ(compact->Find("b"), 2);
  EXPECT_EQ(compact->Find("c"), 3);
}

// This tests reading a symbol table from an FST.
TEST_F(SymbolTableOpsTest, FstReadSymbolsTest) {
  std::unique_ptr<SymbolTable> st(FstReadSymbols(t3_fst_name_, true));
  EXPECT_EQ(st->Find("a"), 1);
  st.reset(FstReadSymbols(t3_fst_name_, false));
  EXPECT_EQ(st->Find("z"), 1);
}

// This tests pruning of a symbol table.
TEST_F(SymbolTableOpsTest, PruneSymbolTableTest) {
  std::unique_ptr<StdVectorFst> fst(StdVectorFst::Read(t3_fst_name_));
  std::unique_ptr<SymbolTable> left(SymbolTable::ReadText(t2_map_name_));

  Relabel(fst.get(), left.get(), left.get());
  std::unique_ptr<const SymbolTable> pruned(
      PruneSymbolTable(*fst, *fst->InputSymbols(), true));

  EXPECT_EQ(pruned->NumSymbols(), 2);
  EXPECT_EQ(pruned->Find("<epsilon>"), 0);
  EXPECT_EQ(pruned->Find("a"), 1);
  EXPECT_EQ(pruned->Find("b"), -1);

  pruned.reset(PruneSymbolTable(*fst, *fst->OutputSymbols(), false));
  EXPECT_EQ(pruned->NumSymbols(), 2);
  EXPECT_EQ(pruned->Find("<epsilon>"), 0);
  EXPECT_EQ(pruned->Find("z"), 5);
  EXPECT_EQ(pruned->Find("b"), -1);
}

}  // namespace
}  // namespace fst
