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
// Implementation of a heap as in STL, but allows tracking positions in heap
// using a key. The key can be used to do an in-place update of values in the
// heap.

#ifndef OPENFST_LIB_HEAP_H_
#define OPENFST_LIB_HEAP_H_

#include <utility>
#include <vector>

#include "absl/log/check.h"

namespace fst {

// A templated heap implementation that supports in-place update of values.
//
// The templated heap implementation is a little different from the STL
// priority_queue and the *_heap operations in STL. This heap supports
// indexing of values in the heap via an associated key.
//
// Each value is internally associated with a key which is returned to the
// calling functions on heap insert. This key can be used to later update
// the specific value in the heap.
//
// T: the element type of the hash. It can be POD, Data or a pointer to Data.
// Compare: comparison functor for determining min-heapness.
template <class T, class Compare>
class Heap {
 public:
  using Value = T;

  static constexpr int kNoKey = -1;

  // Initializes with a specific comparator.
  explicit Heap(Compare comp = Compare()) : comp_(comp), size_(0) {}

  // Inserts a value into the heap.
  int Insert(const Value& value) {
    if (size_ < elements_.size()) {
      elements_[size_].value = value;
      pos_[elements_[size_].key] = size_;
    } else {
      elements_.push_back({value, size_});
      pos_.push_back(size_);
    }
    ++size_;
    return Insert(value, size_ - 1);
  }

  // Updates a value at position given by the key. The pos_ array is first
  // indexed by the key. The position gives the position in the heap array.
  // Once we have the position we can then use the standard heap operations
  // to calculate the parent and child positions.
  void Update(int key, const Value& value) {
    const auto i = pos_[key];
    const bool is_better = comp_(value, elements_[Parent(i)].value);
    elements_[i].value = value;
    if (is_better) {
      Insert(value, i);
    } else {
      Heapify(i);
    }
  }

  // Returns the least value.
  Value Pop() {
    DCHECK(!Empty());
    const Value top = elements_.front().value;
    Swap(0, size_ - 1);
    size_--;
    Heapify(0);
    return top;
  }

  // Returns the least value w.r.t. the comparison function from the
  // heap.
  const Value& Top() const {
    DCHECK(!Empty());
    return elements_.front().value;
  }

  // Returns the element for the given key.
  const Value& Get(int key) const {
    DCHECK_LT(key, pos_.size());
    return elements_[pos_[key]].value;
  }

  // Checks if the heap is empty.
  bool Empty() const { return size_ == 0; }

  void Clear() { size_ = 0; }

  int Size() const { return size_; }

  void Reserve(int size) {
    elements_.reserve(size);
    pos_.reserve(size);
  }

  const Compare& GetCompare() const { return comp_; }

 private:
  // The following private routines are used in a supportive role
  // for managing the heap and keeping the heap properties.

  // Computes left child of parent.
  static int Left(int i) {
    return 2 * (i + 1) - 1;  // 0 -> 1, 1 -> 3
  }

  // Computes right child of parent.
  static int Right(int i) {
    return 2 * (i + 1);  // 0 -> 2, 1 -> 4
  }

  // Given a child computes parent.
  static int Parent(int i) {
    return (i - 1) / 2;  // 0 -> 0, 1 -> 0, 2 -> 0,  3 -> 1,  4 -> 1, ...
  }

  // Swaps a child and parent. Use to move element up/down tree. Note the use of
  // a little trick here. When we swap we need to swap:
  //
  // - the value
  // - the associated keys
  // - the position of the value in the heap
  void Swap(int j, int k) {
    if (j == k) return;
    pos_[elements_[j].key] = k;
    pos_[elements_[k].key] = j;
    std::swap(elements_[j], elements_[k]);
  }

  // Heapifies the subtree rooted at index i.
  void Heapify(int i) {
    while (true) {
      const auto l = Left(i);
      const auto r = Right(i);
      auto largest =
          (l < size_ && comp_(elements_[l].value, elements_[i].value)) ? l : i;
      if (r < size_ && comp_(elements_[r].value, elements_[largest].value)) {
        largest = r;
      }
      if (largest != i) {
        Swap(i, largest);
        i = largest;
      } else {
        break;
      }
    }
  }

  // Inserts (updates) element at subtree rooted at index i.
  int Insert(const Value& value, int i) {
    int p;
    while (i > 0 && !comp_(elements_[p = Parent(i)].value, value)) {
      Swap(i, p);
      i = p;
    }
    return elements_[i].key;
  }

 private:
  struct Node {
    Value value;
    int key;
  };

  const Compare comp_;

  std::vector<int> pos_;
  std::vector<Node> elements_;
  int size_;
};

}  // namespace fst

#endif  // OPENFST_LIB_HEAP_H_
