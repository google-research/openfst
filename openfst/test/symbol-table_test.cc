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

#include "openfst/lib/symbol-table.h"

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "openfst/compat/file_path.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/log/log.h"
#include "absl/memory/memory.h"
#include "openfst/compat/compat_memory.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"

namespace fst {
namespace {

using ::testing::IsNull;
using ::testing::NotNull;

class SymbolTableTest : public testing::Test {
 protected:
  std::string test_tmpdir_;
  void SetUp() override {
    const std::string path = JoinPath(
        std::string("."),
        "openfst/test/testdata/symbol-table");
    test_tmpdir_ = ::testing::TempDir();
    const std::string phoneset_name = JoinPath(path, "phones.map");

    pphoneset_.reset(SymbolTable::ReadText(phoneset_name));
    ASSERT_EQ(pphoneset_->NumSymbols(), 93);
  }

  std::unique_ptr<SymbolTable> pphoneset_;
};

TEST_F(SymbolTableTest, ReadWriteIterateCountEquality) {
  SymbolTable& phoneset = *pphoneset_;

  std::string checksum1 = phoneset.LabeledCheckSum();
  // Writes out.
  phoneset.Write(JoinPath(test_tmpdir_, "phones.map"));

  // Reads back in.
  std::unique_ptr<SymbolTable> tmp(
      SymbolTable::Read(JoinPath(test_tmpdir_, "phones.map")));
  tmp->AddSymbol("uw");

  std::string checksum2 = tmp->LabeledCheckSum();
  VLOG(1) << "uw Index     : " << tmp->Find("uw");
  VLOG(1) << "String uw    : " << tmp->Find(93);

  for (SymbolTableIterator syms_iter(phoneset); !syms_iter.Done();
       syms_iter.Next()) {
    LOG(INFO) << syms_iter.Value() << " " << syms_iter.Symbol();
    EXPECT_EQ(syms_iter.Value(), tmp->Find(syms_iter.Symbol()));
    EXPECT_EQ(syms_iter.Symbol(), tmp->Find(syms_iter.Value()));
  }
  EXPECT_EQ(phoneset.NumSymbols(), tmp->NumSymbols());

  ASSERT_TRUE(checksum1 == checksum2);
  VLOG(1) << "Checksum equality succeeded";
}

TEST_F(SymbolTableTest, AddSymbol) {
  SymbolTable st_one;
  EXPECT_EQ(100, st_one.AddSymbol("A", 100));
  EXPECT_EQ(100, st_one.AddSymbol("A", 101));
  EXPECT_EQ(100, st_one.Find("A"));
  EXPECT_EQ("", st_one.Find(101));
  EXPECT_FALSE(st_one.Member(101));
  std::multimap<std::string, int64_t> check;
  for (SymbolTableIterator sti(st_one); !sti.Done(); sti.Next()) {
    check.insert(std::pair<std::string, int64_t>(sti.Symbol(), sti.Value()));
  }
  EXPECT_EQ(1, check.size());

  SymbolTable st_two;
  EXPECT_EQ(100, st_two.AddSymbol("A", 100));
  EXPECT_EQ(100, st_two.AddSymbol("B", 100));
  EXPECT_EQ(100, st_two.Find("A"));
  EXPECT_EQ(100, st_two.Find("B"));
  EXPECT_EQ("B", st_two.Find(100));
  std::multimap<std::string, int64_t> check2;
  for (SymbolTableIterator sti(st_two); !sti.Done(); sti.Next()) {
    check2.insert(std::pair<std::string, int64_t>(sti.Symbol(), sti.Value()));
  }
  EXPECT_EQ(2, check2.size());
}

TEST_F(SymbolTableTest, ManyDifferentIterationVariants) {
  SymbolTable& phoneset = *pphoneset_;

  // Uses `std::transform` to flex `SymbolTable::iterator`.
  std::vector<std::pair<int64_t, std::string>> st_vec;
  std::transform(phoneset.begin(), phoneset.end(), std::back_inserter(st_vec),
                 [](const auto& sti_item) {
                   return std::make_pair(sti_item.Label(), sti_item.Symbol());
                 });

  // Traditional SymbolTableIterator for loop.
  {
    int idx = 0;
    for (SymbolTableIterator sti(phoneset); !sti.Done(); sti.Next()) {
      LOG(INFO) << sti.Value() << " " << sti.Symbol();
      EXPECT_EQ(sti.Value(), st_vec[idx].first);
      EXPECT_EQ(sti.Symbol(), st_vec[idx].second);
      ++idx;
    }
  }

  // Iterator-style for loop.
  {
    int idx = 0;
    for (SymbolTable::iterator sti = phoneset.begin(); sti != phoneset.end();
         ++sti) {
      LOG(INFO) << sti->Label() << " " << sti->Symbol();
      EXPECT_EQ(sti->Label(), st_vec[idx].first);
      EXPECT_EQ(sti->Symbol(), st_vec[idx].second);
      ++idx;
    }
  }

  // Iterator-style for loop using the const_iterator cbegin/cend functions,
  // even though they're just aliases here to the non-const versions.
  {
    int idx = 0;
    for (SymbolTable::const_iterator sti = phoneset.cbegin();
         sti != phoneset.cend(); ++sti) {
      LOG(INFO) << sti->Label() << " " << sti->Symbol();
      EXPECT_EQ(sti->Label(), st_vec[idx].first);
      EXPECT_EQ(sti->Symbol(), st_vec[idx].second);
      ++idx;
    }
  }

  // Range-based for loop.
  {
    int idx = 0;
    for (const auto& st_item : phoneset) {
      LOG(INFO) << st_item.Label() << " " << st_item.Symbol();
      EXPECT_EQ(st_item.Label(), st_vec[idx].first);
      EXPECT_EQ(st_item.Symbol(), st_vec[idx].second);
      ++idx;
    }
  }

  // Verifies immediately-unpacked and later-unpacked versions of
  // `SymbolTable::iterator::value_type` give identical results.
  std::vector<SymbolTable::iterator::value_type> st_item_vec;
  std::copy(phoneset.begin(), phoneset.end(), std::back_inserter(st_item_vec));
  for (int idx = 0; idx < st_vec.size(); ++idx) {
    EXPECT_EQ(st_item_vec[idx].Label(), st_vec[idx].first);
    EXPECT_EQ(st_item_vec[idx].Symbol(), st_vec[idx].second);
  }

  // Tests prefix-increment with two iterators.
  {
    ASSERT_GE(phoneset.NumSymbols(), 2);
    SymbolTable::iterator syms_iter = phoneset.begin();
    SymbolTable::iterator syms_iter_2(syms_iter);
    EXPECT_EQ(syms_iter, syms_iter_2);
    EXPECT_NE(syms_iter, ++syms_iter_2);
    EXPECT_EQ(++syms_iter, syms_iter_2);
  }

  // Tests prefix-increment with one iterator.
  {
    ASSERT_GE(phoneset.NumSymbols(), 2);
    SymbolTable::iterator syms_iter = phoneset.begin();
    EXPECT_EQ(syms_iter, syms_iter);
    EXPECT_EQ(++syms_iter, syms_iter);
    EXPECT_EQ(syms_iter, syms_iter);
    EXPECT_EQ(syms_iter, ++syms_iter);
    EXPECT_EQ(syms_iter, syms_iter);
  }

  // Tests postfix-increment with one iterator.
  {
    ASSERT_GE(phoneset.NumSymbols(), 2);
    SymbolTable::iterator syms_iter = phoneset.begin();
    EXPECT_EQ(syms_iter, syms_iter);
    EXPECT_NE(syms_iter++, syms_iter);
    EXPECT_EQ(syms_iter, syms_iter);
    EXPECT_NE(syms_iter, syms_iter++);
    EXPECT_EQ(syms_iter, syms_iter);
  }

  // Tests `end()` is `NumSymbols()` increments after `begin()`.
  {
    SymbolTable::iterator syms_iter = phoneset.begin();
    for (int i = 0; i < phoneset.NumSymbols(); ++i) {
      EXPECT_NE(syms_iter, phoneset.end());
      ++syms_iter;
    }
    EXPECT_EQ(syms_iter, phoneset.end());
  }

  // Tests `SymbolTable::iterator`'s move and copy assignment.
  {
    // Uses move constructor.
    const SymbolTable::iterator syms_iter_begin(phoneset.begin());
    // Uses copy constructor.
    SymbolTable::iterator syms_iter(syms_iter_begin);
    EXPECT_EQ(syms_iter, syms_iter_begin);
    ++syms_iter;
    EXPECT_NE(syms_iter, syms_iter_begin);
    // Uses move assignment.
    syms_iter = phoneset.begin();
    EXPECT_EQ(syms_iter, syms_iter_begin);
    ++syms_iter;
    EXPECT_NE(syms_iter, syms_iter_begin);
    // Uses copy assignment.
    syms_iter = syms_iter_begin;
    EXPECT_EQ(syms_iter, syms_iter_begin);
  }
}

TEST_F(SymbolTableTest, NthKey) {
  SymbolTable symbols("dense-phones");
  for (auto i = 0; i < pphoneset_->NumSymbols(); i++) {
    EXPECT_EQ(i, symbols.NumSymbols());
    const int64_t orig_key = pphoneset_->GetNthKey(i);
    const int64_t key = symbols.AddSymbol(pphoneset_->Find(orig_key), orig_key);
    EXPECT_EQ(orig_key, key);
  }
}

TEST_F(SymbolTableTest, NegLabelsOk) {
  pphoneset_->AddSymbol("neg", -2);
  EXPECT_EQ("neg", pphoneset_->Find(-2));
  EXPECT_EQ(-2, pphoneset_->Find("neg"));
}

TEST_F(SymbolTableTest, Relabel) {
  const std::vector<std::pair<int64_t, int64_t>> relabel = {
      std::make_pair(88, 89),
      std::make_pair(90, 92),
      std::make_pair(79, 72),
  };
  std::unique_ptr<SymbolTable> new_syms(
      RelabelSymbolTable<int64_t>(pphoneset_.get(), relabel));

  // Should only include explicitly-mentioned pairs.
  EXPECT_EQ(3, new_syms->NumSymbols());
}

TEST_F(SymbolTableTest, CompatSymbols) {
  EXPECT_TRUE(CompatSymbols(nullptr, nullptr)) << "Both sides nullptr";

  EXPECT_TRUE(CompatSymbols(nullptr, pphoneset_.get())) << "Left side nullptr";
  EXPECT_TRUE(CompatSymbols(pphoneset_.get(), nullptr)) << "Right side nullptr";

  std::unique_ptr<SymbolTable> other_table =
      std::make_unique<SymbolTable>(*pphoneset_);
  EXPECT_TRUE(CompatSymbols(other_table.get(), pphoneset_.get()))
      << "Self-equivalent";

  other_table->AddSymbol("heyhowdyhey");
  EXPECT_FALSE(CompatSymbols(other_table.get(), pphoneset_.get()))
      << "Non-equivalent";
}

TEST(SymbolTableReadTextTest, Valid) {
  // Both tab and space are valid by default.
  std::istringstream strm("foo\t2\nbar 1\n");

  auto symbols =
      absl::WrapUnique(SymbolTable::ReadText(strm, /*name=*/"valid input"));
  ASSERT_THAT(symbols, NotNull());
  EXPECT_EQ(symbols->AvailableKey(), 3);
  EXPECT_EQ(symbols->Find("foo"), 2);
  EXPECT_EQ(symbols->Find("bar"), 1);
}

TEST(SymbolTableReadTextTest, ExtraWordChars) {
  std::istringstream strm("foo\t2boo\nbar 1\n");

  auto symbols = absl::WrapUnique(
      SymbolTable::ReadText(strm, /*name=*/"invalid input", "\t"));
  ASSERT_THAT(symbols, IsNull());
}

TEST(SymbolTableReadTextTest, ExtraWhitespace) {
  std::istringstream strm("foo\t2 \nbar 1\n");

  auto symbols = absl::WrapUnique(
      SymbolTable::ReadText(strm, /*name=*/"surprisingly valid input"));
  ASSERT_THAT(symbols, NotNull());
  EXPECT_EQ(symbols->AvailableKey(), 3);
  EXPECT_EQ(symbols->Find("foo"), 2);
  EXPECT_EQ(symbols->Find("bar"), 1);
}

TEST(SymbolTableReadTextTest, SpaceSymbolWithDefaultSeparatorFails) {
  std::istringstream strm("x\t0\n \t1\ny\t2\n");

  // Default separator is tab and space, so the second line is missing
  // its symbol.
  auto symbols = absl::WrapUnique(
      SymbolTable::ReadText(strm, /*name=*/"space/tab separators"));
  ASSERT_THAT(symbols, IsNull());
}

TEST(SymbolTableReadTextTest, SpaceSymbolWithTabSeparatorParses) {
  std::istringstream strm("x\t0\n \t1\ny\t2\n");

  auto symbols = absl::WrapUnique(
      SymbolTable::ReadText(strm, /*name=*/"tab separator", "\t"));
  ASSERT_THAT(symbols, NotNull());
  EXPECT_EQ(symbols->AvailableKey(), 3);
  EXPECT_EQ(symbols->Find("x"), 0);
  EXPECT_EQ(symbols->Find(" "), 1);
  EXPECT_EQ(symbols->Find("y"), 2);
}

TEST(SymbolTableReadTextTest, NegativeLabelPasses) {
  std::istringstream strm("negative_label -2\n");

  auto symbols = absl::WrapUnique(
      SymbolTable::ReadText(strm, /*name=*/"allow_negative_labels true"));
  ASSERT_THAT(symbols, NotNull());
  EXPECT_EQ(symbols->Find("negative_label"), -2);
  EXPECT_EQ(symbols->Find(-2), "negative_label");
}

class RemoveSymbolTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Dense range 0 .. 9: "a"  .. "j"
    for (int i = 0; i < 10; ++i) {
      const std::string symbol(1, 'a' + i);
      elements_.emplace_back(symbol, i);
      symbols_.AddSymbol(symbol);
    }
    // Sparse symbols 100, 110, .., 190: "aa" .. "jj"
    for (int i = 0; i < 10; ++i) {
      const std::string symbol(2, 'a' + i);
      const int key = 100 + i * 10;
      elements_.emplace_back(symbol, key);
      symbols_.AddSymbol(symbol, key);
    }
  }

  void CheckSymbols(absl::Span<const int64_t> removed_keys) const {
    SymbolTableIterator iter(symbols_);
    for (const auto& e : elements_) {
      if (std::find(removed_keys.begin(), removed_keys.end(), e.second) ==
          removed_keys.end()) {
        EXPECT_EQ(e.second, symbols_.Find(e.first)) << e.first;
        EXPECT_EQ(e.first, symbols_.Find(e.second)) << e.first;
        EXPECT_FALSE(iter.Done());
        EXPECT_EQ(e.second, iter.Value());
        EXPECT_EQ(e.first, iter.Symbol());
        iter.Next();
      } else {  // removed key
        EXPECT_EQ(kNoSymbol, symbols_.Find(e.first));
        EXPECT_FALSE(symbols_.Member(e.second));
      }
    }
    EXPECT_TRUE(iter.Done());
  }

  std::vector<std::pair<std::string, int>> elements_;
  SymbolTable symbols_;
};

TEST_F(RemoveSymbolTest, RemoveNotExisting) {
  symbols_.RemoveSymbol(999);
  CheckSymbols({});
}

TEST_F(RemoveSymbolTest, AvailableKey) {
  EXPECT_EQ(191, symbols_.AvailableKey());
  symbols_.RemoveSymbol(9);
  EXPECT_EQ(191, symbols_.AvailableKey());

  symbols_.RemoveSymbol(190);
  EXPECT_EQ(190, symbols_.AvailableKey());
}

TEST_F(RemoveSymbolTest, RemoveAndAdd) {
  symbols_.RemoveSymbol(9);
  EXPECT_EQ(191, symbols_.AddSymbol("xx"));
  EXPECT_EQ(9, symbols_.AddSymbol("yy", 9));
}

TEST_F(RemoveSymbolTest, RemoveDoesNotChangeOtherSymbolIds) {
  ASSERT_EQ("e", symbols_.Find(4));
  ASSERT_EQ("f", symbols_.Find(5));
  symbols_.RemoveSymbol(4);
  EXPECT_EQ("", symbols_.Find(4));
  EXPECT_EQ("f", symbols_.Find(5));
}

TEST_F(RemoveSymbolTest, OnlyDense) {
  elements_.resize(10);
  symbols_ = SymbolTable();
  for (const auto& e : elements_) symbols_.AddSymbol(e.first);
  symbols_.RemoveSymbol(4);
  CheckSymbols({4});
  symbols_.RemoveSymbol(0);
  CheckSymbols({0, 4});
  symbols_.RemoveSymbol(9);
  CheckSymbols({0, 4, 9});
}

TEST_F(RemoveSymbolTest, OnlySparse) {
  elements_.erase(elements_.begin());
  symbols_ = SymbolTable();
  for (const auto& e : elements_) symbols_.AddSymbol(e.first, e.second);
  symbols_.RemoveSymbol(140);
  CheckSymbols({140});
  symbols_.RemoveSymbol(1);
  CheckSymbols({1, 140});
  symbols_.RemoveSymbol(190);
  CheckSymbols({1, 140, 190});
}

class RemoveOneSymbolTest : public RemoveSymbolTest,
                            public ::testing::WithParamInterface<int> {
 protected:
  int RemovedKey() const { return elements_[this->GetParam()].second; }
};

TEST_P(RemoveOneSymbolTest, RemoveKeyAndCheck) {
  symbols_.RemoveSymbol(RemovedKey());
  CheckSymbols({RemovedKey()});
}

INSTANTIATE_TEST_SUITE_P(RemoveEachKey, RemoveOneSymbolTest,
                         ::testing::Range(0, 20));
}  // namespace
}  // namespace fst
