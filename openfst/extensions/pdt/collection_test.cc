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
// Unit test for the Collection class.

#include "openfst/extensions/pdt/collection.h"

#include <vector>

#include "gtest/gtest.h"

namespace fst {
namespace {

using I = int;
using T = int;

class CollectionTest : public testing::Test {
 public:
 protected:
  void SetUp() override {
    set1_.push_back(0);
    set1_.push_back(1);
    set1_.push_back(4);
    set1_.push_back(5);
    set1_.push_back(6);
    set1_.push_back(9);

    set2_.push_back(0);
    set2_.push_back(1);
    set2_.push_back(5);
    set2_.push_back(4);
    set2_.push_back(6);
    set2_.push_back(9);

    set3_.push_back(0);
    set3_.push_back(1);
    set3_.push_back(1);
    set3_.push_back(4);
    set3_.push_back(5);
    set3_.push_back(6);
    set3_.push_back(9);
  }

  void TearDown() override {
    set1_.clear();
    set2_.clear();
    set3_.clear();
  }

  std::vector<T> set1_;  // (0, 1, 4, 5, 6, 9).
  std::vector<T> set2_;  // (0, 1, 5, 4, 6, 9).
  std::vector<T> set3_;  // (0, 1, 1, 4, 5, 6, 9).
};

// Basic test.
TEST_F(CollectionTest, Find1Test) {
  Collection<I, T> collection;
  const auto id = collection.FindId(set1_);
  std::vector<T> set;
  auto set_iter = collection.FindSet(id);
  for (; !set_iter.Done(); set_iter.Next()) set.push_back(set_iter.Element());
  ASSERT_EQ(set, set1_);
}

// Sets are ordered.
TEST_F(CollectionTest, Find2Test) {
  Collection<I, T> collection;
  const auto id1 = collection.FindId(set1_);
  const auto no_id = Collection<I, T>::NodeTable::kNoId;
  ASSERT_NE(collection.FindId(set1_, false), no_id);
  ASSERT_EQ(collection.FindId(set2_, false), no_id);
  const auto id2 = collection.FindId(set2_);
  ASSERT_NE(id1, id2);
  std::vector<T> set;
  auto set_iter = collection.FindSet(id2);
  for (; !set_iter.Done(); set_iter.Next()) set.push_back(set_iter.Element());
  ASSERT_EQ(set, set2_);
}

// Sets are actually multisets; repetitions allowed.
TEST_F(CollectionTest, Find3Test) {
  Collection<I, T> collection;
  const auto id1 = collection.FindId(set1_);
  const auto id3 = collection.FindId(set3_);
  ASSERT_NE(id1, id3);
  std::vector<T> set;
  Collection<I, T>::SetIterator set_iter = collection.FindSet(id3);
  for (; !set_iter.Done(); set_iter.Next()) set.push_back(set_iter.Element());
  ASSERT_EQ(set, set3_);
}

}  // namespace
}  // namespace fst
