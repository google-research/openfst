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

#ifndef OPENFST_EXTENSIONS_NGRAM_LOUDS_TREE_H_
#define OPENFST_EXTENSIONS_NGRAM_LOUDS_TREE_H_

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <istream>
#include <limits>
#include <memory>
#include <ostream>
#include <utility>

#include "absl/memory/memory.h"
#include "absl/types/span.h"
#include "openfst/extensions/ngram/bitmap-index.h"

namespace fst {

// This is an implementation of the LOUDS tree data structure
// built on the BitmapIndex bitstring class.
// See: Engineering the LOUDS Succinct Tree Representation
// Init(degreesequence) must be called to verify the tree structure
// and initialize the tree.
class LoudsTree {
 public:
  LoudsTree() : initialized_(false) {}

  LoudsTree(const LoudsTree& other) : initialized_(other.initialized_) {
    if (other.initialized_) {
      const size_t array_size = other.bitmap_.ArraySize();
      bm_ = absl::make_unique_for_overwrite<uint64_t[]>(array_size);
      std::copy_n(other.bm_.get(), array_size, bm_.get());
      bitmap_.BuildIndex(bm_.get(), other.bitmap_.Bits());
    }
  }

  LoudsTree(LoudsTree&&) = default;

  // Initializes the LOUDS tree from a degree sequence. The degree sequence
  // must encode a valid tree in BFS order.
  // The degree sequence is the number of children per node. So, the tree
  //                     ●
  //                    ╱ ╲
  //                   ●   ●
  //                      ╱ ╲
  //                     ●   ●
  // would have degree sequence {2, 0, 2, 0, 0}
  // Returns true if the initialization is successful.
  bool Init(absl::Span<const size_t> degreeseq) {
    size_t nodes = degreeseq.size();
    bm_ = std::make_unique<uint64_t[]>(BitmapIndex::StorageSize(2 * nodes + 1));
    if (nodes == 0) {
      initialized_ = true;
      return true;
    }
    size_t degsum = degreeseq[0];
    size_t currentrun = degreeseq[0];
    for (size_t i = 1; i < nodes; ++i) {
      degsum += degreeseq[i];
      currentrun += degreeseq[i] - 1;
      if (currentrun <= 0 && i < nodes - 1) {
        initialized_ = false;
        return false;
      }
    }
    if (degsum != nodes - 1) {
      initialized_ = false;
      return false;
    }

    size_t cur = 2;
    BitmapIndex::Set(bm_.get(), 0);
    for (auto deg : degreeseq) {
      for (int i = 0; i < deg; ++i) BitmapIndex::Set(bm_.get(), cur++);
      ++cur;
    }
    bitmap_.BuildIndex(bm_.get(), 2 * nodes + 1);
    initialized_ = true;
    return true;
  }

  size_t NumNodes() const {
    // If nodes > 0, then bits = 2 * nodes + 1, so nodes = (bits - 1) / 2. The
    // number of bits is always odd, so using bits / 2 gives the same answer
    // for bits > 0, and also handles bits == 0.
    return bitmap_.Bits() / 2;
  }

  size_t NumEdges() const {
    const size_t num_nodes = NumNodes();
    return num_nodes == 0 ? 0 : num_nodes - 1;
  }

  // REQUIRES: 0 <= node < NumStates().
  bool IsLeaf(size_t node) const {
    const auto selects = bitmap_.Select0s(node);
    const auto s0n = selects.first;
    const auto s0n1 = selects.second;
    return bitmap_.Rank1(s0n) == bitmap_.Rank1(s0n1);
  }

  // REQUIRES: 0 <= node < NumStates().
  size_t FirstChild(size_t node) const {
    const auto selects = bitmap_.Select0s(node);
    const auto s0n = selects.first;
    const auto s0n1 = selects.second;
    const auto first = bitmap_.Rank1(s0n1);
    const auto last = bitmap_.Rank1(s0n);
    return first == last ? std::numeric_limits<size_t>::max()
                         : bitmap_.Rank1(s0n + 1);
  }

  // REQUIRES: 0 <= node < NumStates().
  size_t LastChild(size_t node) const {
    const auto selects = bitmap_.Select0s(node);
    const auto s0n = selects.first;
    const auto s0n1 = selects.second;
    const auto first = bitmap_.Rank1(s0n1);
    const auto last = bitmap_.Rank1(s0n);
    return first == last ? std::numeric_limits<size_t>::max()
                         : bitmap_.Rank1(s0n1 - 1);
  }

  // Returns a pair containing the first and last child of a node or
  // (size_t max(), size_t max()) if it is a leaf.
  // REQUIRES: 0 <= node < NumStates().
  std::pair<size_t, size_t> FirstAndLastChild(size_t node) const {
    const auto selects = bitmap_.Select0s(node);
    const auto s0n = selects.first;
    const auto s0n1 = selects.second;
    const auto first = bitmap_.Rank1(s0n1);
    const auto last = bitmap_.Rank1(s0n);
    return first == last ? std::make_pair(std::numeric_limits<size_t>::max(),
                                          std::numeric_limits<size_t>::max())
                         : std::make_pair(bitmap_.Rank1(s0n + 1),
                                          bitmap_.Rank1(s0n1 - 1));
  }

  // REQUIRES: 0 <= node < NumStates().
  size_t NumChildren(size_t node) const {
    const auto firstlast = FirstAndLastChild(node);
    return firstlast.first == std::numeric_limits<size_t>::max()
               ? 0
               : firstlast.second - firstlast.first + 1;
  }

  // Returns true if node is a child of parent.
  // REQUIRES: 0 <= node < NumStates().
  bool IsChild(size_t parent, size_t node) const {
    if (node <= parent) {
      return false;
    }
    const auto firstlast = FirstAndLastChild(parent);
    const auto first = firstlast.first;
    const auto last = firstlast.second;
    return (first != std::numeric_limits<size_t>::max()) &&
           (first <= node && node <= last);
  }

  // Returns the parent of a given node or size_t max() if it is the root.
  // REQUIRES: 0 <= node < NumStates()
  size_t Parent(size_t node) const {
    return node == 0 ? std::numeric_limits<size_t>::max()
                     : bitmap_.Rank0(bitmap_.Select1(node)) - 1;
  }

  // Returns the Nth child of a node or size_t max() if it has fewer than n
  // children. REQUIRES: 0 <= node < NumStates().
  size_t NthChild(size_t node, size_t n) const {
    const auto firstlast = FirstAndLastChild(node);
    if (firstlast.first == std::numeric_limits<size_t>::max()) {
      return std::numeric_limits<size_t>::max();
    }
    return firstlast.first + n - 1 <= firstlast.second
               ? firstlast.first + n - 1
               : std::numeric_limits<size_t>::max();
  }

  // Returns true if the tree is properly initialized..
  bool IsInitialized() const { return initialized_; }

  bool Write(std::ostream& strm) const {
    uint64_t nodes = NumNodes();
    strm.write(reinterpret_cast<const char*>(&nodes), sizeof(nodes));
    strm.write(reinterpret_cast<const char*>(bm_.get()),
               sizeof(bm_[0]) * BitmapIndex::StorageSize(NumNodes() * 2 + 1));
    return strm ? true : false;
  }

  static std::shared_ptr<LoudsTree> Read(std::istream& strm) {
    uint64_t nodes;
    strm.read(reinterpret_cast<char*>(&nodes), sizeof(nodes));
    const size_t storage_size = BitmapIndex::StorageSize(nodes * 2 + 1);
    auto bm = absl::make_unique_for_overwrite<uint64_t[]>(storage_size);
    strm.read(reinterpret_cast<char*>(bm.get()),
              sizeof(bm[0]) * BitmapIndex::StorageSize(nodes * 2 + 1));
    if (!strm) return nullptr;
    auto lt = std::make_shared<LoudsTree>();
    lt->Init(std::move(bm), nodes);
    return lt;
  }

 private:
  // Initialize given the bit representation of the tree, used only for
  // read/write.
  bool Init(std::unique_ptr<uint64_t[]> bm, size_t nodes) {
    bm_ = std::move(bm);
    bitmap_.BuildIndex(bm_.get(), 2 * nodes + 1);
    initialized_ = true;
    return true;
  }

  BitmapIndex bitmap_;
  std::unique_ptr<uint64_t[]> bm_;
  bool initialized_;
};

}  // namespace fst

#endif  // OPENFST_EXTENSIONS_NGRAM_LOUDS_TREE_H_
