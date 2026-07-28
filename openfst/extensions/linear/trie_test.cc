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

#include "openfst/extensions/linear/trie.h"

#include <functional>
#include <sstream>

#include "gtest/gtest.h"
#include "absl/container/flat_hash_map.h"
#include "absl/strings/string_view.h"

using ::testing::Test;

namespace fst {

constexpr absl::string_view kTestKeys[] = {"hello", "health"};

template <class TrieTopology>
class TrieTopologyTester {
 public:
  static void EmptyTrie() {
    TrieTopology trie;
    EXPECT_EQ(0, trie.Root());
    EXPECT_EQ(1, trie.NumNodes());

    TrieTopology copy(trie);
    EXPECT_EQ(0, copy.Root());
    EXPECT_EQ(1, copy.NumNodes());

    EXPECT_TRUE(trie == copy);
    EXPECT_FALSE(trie != copy);

    trie.Insert(trie.Root(), 1);
    copy.Insert(copy.Root(), 2);

    EXPECT_NE(kNoTrieNodeId, trie.Find(trie.Root(), 1));
    EXPECT_EQ(kNoTrieNodeId, trie.Find(trie.Root(), 2));

    EXPECT_EQ(kNoTrieNodeId, copy.Find(copy.Root(), 1));
    EXPECT_NE(kNoTrieNodeId, copy.Find(copy.Root(), 2));

    EXPECT_FALSE(trie == copy);
    EXPECT_TRUE(trie != copy);

    trie.swap(copy);

    EXPECT_EQ(kNoTrieNodeId, trie.Find(trie.Root(), 1));
    EXPECT_NE(kNoTrieNodeId, trie.Find(trie.Root(), 2));

    EXPECT_NE(kNoTrieNodeId, copy.Find(copy.Root(), 1));
    EXPECT_EQ(kNoTrieNodeId, copy.Find(copy.Root(), 2));

    EXPECT_FALSE(trie == copy);
    EXPECT_TRUE(trie != copy);

    trie = copy;

    EXPECT_TRUE(trie == copy);
    EXPECT_FALSE(trie != copy);
  }

  static void InsertionAndSerialization() {
    TrieTopology trie;
    // Insertion
    for (absl::string_view key : kTestKeys) {
      int node = trie.Root();
      for (const char ch : key) node = trie.Insert(node, ch);
    }
    ASSERT_EQ(10, trie.NumNodes());
    // Look up
    {
      int prev_node = kNoTrieNodeId;
      for (absl::string_view key : kTestKeys) {
        int node = trie.Root();
        for (const char ch : key) {
          node = trie.Find(node, ch);
          ASSERT_NE(kNoTrieNodeId, node);
        }
        EXPECT_NE(prev_node, node);
        prev_node = node;
      }
    }
    // Serialization
    {
      TrieTopology copy;
      ASSERT_TRUE(trie != copy);

      std::ostringstream ostrm;
      trie.Write(ostrm);
      ASSERT_TRUE(ostrm);
      std::istringstream istrm(ostrm.str());
      copy.Read(istrm);
      ASSERT_TRUE(istrm);

      EXPECT_EQ(trie.NumNodes(), copy.NumNodes());
      EXPECT_TRUE(trie == copy);
    }
  }
};

TEST(NestedTrieTopologyTest, EmptyTrie) {
  TrieTopologyTester<NestedTrieTopology<int, std::hash<int>>>::EmptyTrie();
}

TEST(NestedTrieTopologyTest, InsertionAndSerialization) {
  TrieTopologyTester<
      NestedTrieTopology<int, std::hash<int>>>::InsertionAndSerialization();
}

TEST(NestedTrieTopologyTest, ChildrenOf) {
  NestedTrieTopology<int, std::hash<int>> trie;
  int node = trie.Root();
  for (int i = -10; i <= 10; ++i) node = trie.Insert(node, i);
  NestedTrieTopology<int, std::hash<int>> copy(trie), assign;
  assign = trie;

  EXPECT_TRUE(trie == copy);
  EXPECT_TRUE(trie == assign);

  for (int i = 0; i < trie.NumNodes(); ++i) {
    EXPECT_TRUE(trie.ChildrenOf(i) == copy.ChildrenOf(i));
    EXPECT_TRUE(trie.ChildrenOf(i) == assign.ChildrenOf(i));
    EXPECT_FALSE(&trie.ChildrenOf(i) == &copy.ChildrenOf(i));
    EXPECT_FALSE(&trie.ChildrenOf(i) == &assign.ChildrenOf(i));
  }
}

TEST(FlatTrieTopologyTest, EmptyTrie) {
  TrieTopologyTester<FlatTrieTopology<int, std::hash<int>>>::EmptyTrie();
}

TEST(FlatTrieTopologyTest, InsertionAndSerialization) {
  TrieTopologyTester<
      FlatTrieTopology<int, std::hash<int>>>::InsertionAndSerialization();
}

TEST(FlatTrieTopologyTest, Conversion) {
  NestedTrieTopology<int, std::hash<int>> nested;
  FlatTrieTopology<int, std::hash<int>> flat;
  int nested_node = nested.Root(), flat_node = flat.Root();
  EXPECT_EQ(nested_node, flat_node);
  for (int i = -10; i <= 10; ++i) {
    nested_node = nested.Insert(nested_node, i);
    flat_node = flat.Insert(flat_node, i);
    EXPECT_EQ(nested_node, flat_node);
  }
  FlatTrieTopology<int, std::hash<int>> copy(nested);
  EXPECT_TRUE(flat == copy);
}

// Reuse some testing code from `TrieTopologyTester` because both
// tries and topologies share the same basic method set.
template <class Trie>
class TrieTester : public TrieTopologyTester<Trie> {
 public:
  using TrieTopologyTester<Trie>::EmptyTrie;
  using TrieTopologyTester<Trie>::InsertionAndSerialization;

  static void AgreeWithTopology() {
    Trie trie;
    // Insertion
    for (absl::string_view key : kTestKeys) {
      int node = trie.Root();
      for (const char ch : key) node = trie.Insert(node, ch);
      trie[node] = -node;
    }
    const typename Trie::Topology& topology = trie.TrieTopology();
    EXPECT_EQ(topology.Root(), trie.Root());
    EXPECT_EQ(topology.NumNodes(), trie.NumNodes());
    for (absl::string_view key : kTestKeys) {
      int trie_node = trie.Root();
      int topology_node = topology.Root();
      for (const char ch : key) {
        EXPECT_EQ(0, trie[trie_node]);
        trie_node = trie.Find(trie_node, ch);
        topology_node = topology.Find(topology_node, ch);
        EXPECT_EQ(topology_node, trie_node);
      }
      EXPECT_EQ(-trie_node, trie[trie_node]);
      trie_node = trie.Find(trie_node, -1);
      topology_node = topology.Find(topology_node, -1);
      EXPECT_EQ(topology_node, trie_node);
    }
  }
};

TEST(MutableTrieTest, BasicOperations) {
  typedef MutableTrie<int, int, NestedTrieTopology<int, std::hash<int>>>
      TestTrie;
  TrieTester<TestTrie>::EmptyTrie();
  TrieTester<TestTrie>::InsertionAndSerialization();
  TrieTester<TestTrie>::AgreeWithTopology();
}

}  // namespace fst
