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
// Benchmark for FST memory utilities.

#include "openfst/lib/memory.h"

#include <deque>
#include <functional>
#include <list>
#include <set>
#include <unordered_set>
#include <vector>

#include "absl/container/flat_hash_set.h"
#include "absl/container/node_hash_set.h"
#include "benchmark/benchmark.h"

namespace fst {
namespace {

// Tests sequence container allocation
template <typename S>
static void BM_SeqContainer(benchmark::State& state) {
  const int n = 16;
  typename S::allocator_type alloc;
  for (auto unused : state) {
    S s(alloc);
    for (int j = 0; j < n; ++j) s.push_back(j);
    for (int j = 0; j < n; ++j) s.pop_back();
  }
}

using List = std::list<int>;
using BlockAllocList = std::list<int, BlockAllocator<int>>;
using PoolAllocList = std::list<int, PoolAllocator<int>>;

BENCHMARK_TEMPLATE(BM_SeqContainer, List);
BENCHMARK_TEMPLATE(BM_SeqContainer, BlockAllocList);
BENCHMARK_TEMPLATE(BM_SeqContainer, PoolAllocList);

using Vector = std::vector<int>;
using BlockAllocVector = std::vector<int, BlockAllocator<int>>;
using PoolAllocVector = std::vector<int, PoolAllocator<int>>;

BENCHMARK_TEMPLATE(BM_SeqContainer, Vector);
BENCHMARK_TEMPLATE(BM_SeqContainer, BlockAllocVector);
BENCHMARK_TEMPLATE(BM_SeqContainer, PoolAllocVector);

using Deque = std::deque<int>;
using BlockAllocDeque = std::deque<int, BlockAllocator<int>>;
using PoolAllocDeque = std::deque<int, PoolAllocator<int>>;

BENCHMARK_TEMPLATE(BM_SeqContainer, Deque);
BENCHMARK_TEMPLATE(BM_SeqContainer, BlockAllocDeque);
BENCHMARK_TEMPLATE(BM_SeqContainer, PoolAllocDeque);

// Note as to why these benchmarks are not representative:
// * users will not insert a dense sequential range of inputs. This input
//   sequence favors unordered_map because the identity hash + the probing logic
//   there reduces the L1 cache misses and collisions. Swisstable does much
//   better than unordered_* when provided with more production-like
//   distributions.
// * `s.erase(s.begin(), s.end());` is not a common way to clean up a table
//   and it is known to be slower in swisstable.
// * the table is way too small for a general benchmark. The effects of node
//   based containers are not felt as much because all the extra pointer
//   indirection is not triggering cache misses. If small tables are desired
//   for the benchmark, then you must create enough of them (and cycle through
//   them) to make sure the cache behavior matches what your application will
//   have.

// Tests associative container allocation
template <typename S>
static void BM_AssocContainer(benchmark::State& state) {
  const int n = 16;
  S s;
  for (auto unused : state) {
    for (int j = 0; j < n; ++j) s.insert(j);
    s.erase(s.begin(), s.end());
  }
}

using Set = std::set<int>;
using PoolAllocSet = std::set<int, std::less<int>, PoolAllocator<int>>;

BENCHMARK_TEMPLATE(BM_AssocContainer, Set);
BENCHMARK_TEMPLATE(BM_AssocContainer, PoolAllocSet);

using UnorderedSet = std::unordered_set<int>;
using PoolAllocUnorderedSet =
    std::unordered_set<int, std::hash<int>, std::equal_to<int>,
                       PoolAllocator<int>>;

BENCHMARK_TEMPLATE(BM_AssocContainer, UnorderedSet);
BENCHMARK_TEMPLATE(BM_AssocContainer, PoolAllocUnorderedSet);

// std::hash is not a good hash function for the absl types;
// use their defaults.
using FlatHashSet = absl::flat_hash_set<int>;
using PoolAllocFlatHashSet =
    absl::flat_hash_set<int, absl::flat_hash_set<int>::hasher,
                        absl::flat_hash_set<int>::key_equal,
                        PoolAllocator<int>>;

BENCHMARK_TEMPLATE(BM_AssocContainer, FlatHashSet);
BENCHMARK_TEMPLATE(BM_AssocContainer, PoolAllocFlatHashSet);

using NodeHashSet = absl::node_hash_set<int>;
using PoolAllocNodeHashSet =
    absl::node_hash_set<int, absl::node_hash_set<int>::hasher,
                        absl::node_hash_set<int>::key_equal,
                        PoolAllocator<int>>;

BENCHMARK_TEMPLATE(BM_AssocContainer, NodeHashSet);
BENCHMARK_TEMPLATE(BM_AssocContainer, PoolAllocNodeHashSet);

}  // namespace
}  // namespace fst
