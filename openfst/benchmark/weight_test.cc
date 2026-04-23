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

// Benchmark for FST weight classes.

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <vector>

#include "absl/container/flat_hash_set.h"
#include "absl/numeric/bits.h"
#include "absl/random/random.h"
#include "benchmark/benchmark.h"
#include "openfst/lib/float-weight.h"
#include "openfst/lib/pair-weight.h"
#include "openfst/lib/set-weight.h"
#include "openfst/lib/signed-log-weight.h"
#include "openfst/lib/sparse-tuple-weight.h"
#include "openfst/lib/string-weight.h"
#include "openfst/lib/tuple-weight.h"

namespace fst {
namespace {

using UISetWeight = SetWeight<int, SET_UNION_INTERSECT>;

// Tests move constructor of SetWeight.
void BM_SetWeight(benchmark::State& state) {
  static constexpr int kSetSize = 1 << 12;
  UISetWeight set_weight;
  for (int i = 1; i <= kSetSize; ++i) {
    set_weight.PushBack(i);
  }

  const int size = state.range(0);
  std::vector<UISetWeight> weights;
  weights.reserve(size);
  for (int i = 0; i < size; ++i) {
    weights.push_back(set_weight);
  }

  for (auto _ : state) {
    weights.resize(size * 10);
    weights.resize(size);
    weights.shrink_to_fit();
  }
}
BENCHMARK(BM_SetWeight)->Range(1, 1 << 12);

// Tests move constructor of SignedLogWeight.
void BM_SignedLogWeight(benchmark::State& state) {
  const int size = state.range(0);
  std::vector<SignedLog64Weight> weights;
  weights.reserve(size);
  for (int i = 0; i < size; ++i) {
    weights.emplace_back(
        SignedLog64Weight(SignedLog64Weight::W1(-1), SignedLog64Weight::W2(i)));
  }

  for (auto _ : state) {
    weights.resize(size * 10);
    weights.resize(size);
    weights.shrink_to_fit();
  }
}
BENCHMARK(BM_SignedLogWeight)->Range(1, 1 << 21);

template <typename W>
W MakeWeight(float w) {
  return W(w);
}

template <>
SignedLog64Weight MakeWeight<SignedLog64Weight>(float w) {
  return SignedLog64Weight(TropicalWeight(1.0), LogWeightTpl<double>(w));
}

template <typename W>
W MakeRandomWeight(absl::BitGen& bitgen) {
  return W(absl::Uniform(bitgen, -10.0, 10.0));
}

template <>
SignedLog64Weight MakeRandomWeight<SignedLog64Weight>(absl::BitGen& bitgen) {
  return SignedLog64Weight(TropicalWeight(absl::Uniform(bitgen, -10.0, 10.0)),
                           Log64Weight(absl::Uniform(bitgen, -10.0, 10.0)));
}

template <>
StringWeight<int> MakeWeight<StringWeight<int>>(float w) {
  return StringWeight<int>(static_cast<int>(w));
}

template <>
StringWeight<int> MakeRandomWeight<StringWeight<int>>(absl::BitGen& bitgen) {
  StringWeight<int> sw;
  const int length = absl::Uniform(bitgen, 1, 10);
  for (int i = 0; i < length; ++i) {
    sw.PushBack(absl::Uniform(bitgen, 1, 10000));
  }
  return sw;
}

template <>
TupleWeight<LogWeight, 3> MakeWeight<TupleWeight<LogWeight, 3>>(float w) {
  return TupleWeight<LogWeight, 3>(LogWeight(w));
}

template <>
TupleWeight<LogWeight, 3> MakeRandomWeight<TupleWeight<LogWeight, 3>>(
    absl::BitGen& bitgen) {
  return TupleWeight<LogWeight, 3>(
      LogWeight(absl::Uniform(bitgen, -10.0, 10.0)));
}

template <>
SparseTupleWeight<LogWeight, int> MakeWeight<SparseTupleWeight<LogWeight, int>>(
    float w) {
  return SparseTupleWeight<LogWeight, int>(0, LogWeight(w), LogWeight::Zero());
}

template <>
SparseTupleWeight<LogWeight, int>
MakeRandomWeight<SparseTupleWeight<LogWeight, int>>(absl::BitGen& bitgen) {
  SparseTupleWeight<LogWeight, int> sw;
  const int length = absl::Uniform(bitgen, 1, 10);
  for (int i = 0; i < length; ++i) {
    sw.PushBack(i, LogWeight(absl::Uniform(bitgen, -10.0, 10.0)));
  }
  return sw;
}

template <>
TupleWeight<SignedLog64Weight, 3> MakeWeight<TupleWeight<SignedLog64Weight, 3>>(
    float w) {
  return TupleWeight<SignedLog64Weight, 3>(
      SignedLog64Weight(SignedLog64Weight::W1(1.0), SignedLog64Weight::W2(w)));
}

template <>
TupleWeight<SignedLog64Weight, 3>
MakeRandomWeight<TupleWeight<SignedLog64Weight, 3>>(absl::BitGen& bitgen) {
  return TupleWeight<SignedLog64Weight, 3>(
      SignedLog64Weight(TropicalWeight(absl::Uniform(bitgen, -10.0, 10.0)),
                        Log64Weight(absl::Uniform(bitgen, -10.0, 10.0))));
}

template <>
TupleWeight<TropicalWeight, 3> MakeWeight<TupleWeight<TropicalWeight, 3>>(
    float w) {
  return TupleWeight<TropicalWeight, 3>(TropicalWeight(w));
}

template <>
TupleWeight<TropicalWeight, 3> MakeRandomWeight<TupleWeight<TropicalWeight, 3>>(
    absl::BitGen& bitgen) {
  return TupleWeight<TropicalWeight, 3>(
      TropicalWeight(absl::Uniform(bitgen, -10.0, 10.0)));
}

template <>
SparseTupleWeight<SignedLog64Weight, int>
MakeWeight<SparseTupleWeight<SignedLog64Weight, int>>(float w) {
  return SparseTupleWeight<SignedLog64Weight, int>(
      0,
      SignedLog64Weight(SignedLog64Weight::W1(1.0), SignedLog64Weight::W2(w)),
      SignedLog64Weight::Zero());
}

template <>
SparseTupleWeight<SignedLog64Weight, int>
MakeRandomWeight<SparseTupleWeight<SignedLog64Weight, int>>(
    absl::BitGen& bitgen) {
  SparseTupleWeight<SignedLog64Weight, int> sw;
  const int length = absl::Uniform(bitgen, 1, 10);
  for (int i = 0; i < length; ++i) {
    sw.PushBack(
        i, SignedLog64Weight(TropicalWeight(absl::Uniform(bitgen, -10.0, 10.0)),
                             Log64Weight(absl::Uniform(bitgen, -10.0, 10.0))));
  }
  return sw;
}

template <>
SparseTupleWeight<TropicalWeight, int>
MakeWeight<SparseTupleWeight<TropicalWeight, int>>(float w) {
  return SparseTupleWeight<TropicalWeight, int>(0, TropicalWeight(w),
                                                TropicalWeight::Zero());
}

template <>
SparseTupleWeight<TropicalWeight, int>
MakeRandomWeight<SparseTupleWeight<TropicalWeight, int>>(absl::BitGen& bitgen) {
  SparseTupleWeight<TropicalWeight, int> sw;
  const int length = absl::Uniform(bitgen, 1, 10);
  for (int i = 0; i < length; ++i) {
    sw.PushBack(i, TropicalWeight(absl::Uniform(bitgen, -10.0, 10.0)));
  }
  return sw;
}

// Generic helper for collision benchmarks.
template <typename Container, typename Hasher>
void BM_Collisions_Generic(benchmark::State& state, const Container& weights,
                           Hasher hasher) {
  for (auto _ : state) {
    std::vector<size_t> hashes;
    hashes.reserve(weights.size());
    for (const auto& w : weights) {
      hashes.push_back(hasher(w));
    }
    std::sort(hashes.begin(), hashes.end());
    int collisions = 0;
    for (size_t i = 1; i < hashes.size(); ++i) {
      if (hashes[i] == hashes[i - 1]) {
        ++collisions;
      }
    }
    state.counters["collisions"] = collisions;
  }
}

// Measures the avalanche effect.
//
// The avalanche effect, a term coined by Horst Feistel (1973) in "Cryptography
// and Computer Privacy", refers to the property that a small change in the
// input (e.g., flipping a single bit) results in a significant change in the
// output (ideally, half of the output bits should flip).
//
// This benchmark flips each bit of the input and counts the average number of
// bits that change in the resulting hash. For a 64-bit hash, the ideal value
// is 32.
// Generic helper for avalanche benchmarks.
template <typename W, typename Generator, typename Hasher>
void BM_Avalanche_Generic(benchmark::State& state, Generator generator,
                          Hasher hasher) {
  absl::BitGen bitgen;
  const int num_samples = 1000;
  for (auto _ : state) {
    int64_t total_changed_bits = 0;
    int total_flips = 0;
    for (int s = 0; s < num_samples; ++s) {
      W w = generator(bitgen);
      const size_t base_hash = hasher(w);
      const size_t num_bytes = sizeof(w);
      unsigned char* bytes = reinterpret_cast<unsigned char*>(&w);
      for (size_t i = 0; i < num_bytes * 8; ++i) {
        const unsigned char original_byte = bytes[i / 8];
        bytes[i / 8] ^= (1 << (i % 8));
        const size_t new_hash = hasher(w);
        total_changed_bits += absl::popcount(base_hash ^ new_hash);
        ++total_flips;
        bytes[i / 8] = original_byte;
      }
    }
    state.counters["avg_changed_bits"] =
        static_cast<double>(total_changed_bits) / total_flips;
  }
}

// Measures the Strict Avalanche Criterion (SAC).
//
// SAC was introduced by Webster and Tavares (1985) in their paper "On the
// Design of S-Boxes" (Advances in Cryptology - CRYPTO '85) as a generalization
// of the avalanche effect. A hash function satisfies SAC if, whenever a single
// input bit is complemented, each of the output bits changes with a 50%
// probability.
//
// This benchmark creates a matrix of size (input_bits) x (hash_bits) and
// counts how many times each output bit changes when each input bit is flipped.
// It reports the maximum deviation of any probability from the ideal 0.5.
// Generic helper for SAC benchmarks.
template <typename W, typename Generator, typename Hasher>
void BM_SAC_Generic(benchmark::State& state, Generator generator,
                    Hasher hasher) {
  absl::BitGen bitgen;
  const int num_samples = 1000;
  const size_t num_bytes = sizeof(W);
  const size_t num_bits = num_bytes * 8;
  const size_t hash_bits = sizeof(size_t) * 8;

  // matrix[input_bit][output_bit]
  std::vector<std::vector<int>> change_counts(num_bits,
                                              std::vector<int>(hash_bits, 0));
  int total_samples = 0;
  for (auto _ : state) {
    for (int s = 0; s < num_samples; ++s) {
      W w = generator(bitgen);
      const size_t base_hash = hasher(w);
      unsigned char* bytes = reinterpret_cast<unsigned char*>(&w);

      for (size_t i = 0; i < num_bits; ++i) {
        const unsigned char original_byte = bytes[i / 8];
        bytes[i / 8] ^= (1 << (i % 8));
        const size_t new_hash = hasher(w);
        const size_t diff = base_hash ^ new_hash;

        for (size_t j = 0; j < hash_bits; ++j) {
          if ((diff >> j) & 1) {
            change_counts[i][j]++;
          }
        }
        bytes[i / 8] = original_byte;
      }
      total_samples++;
    }
  }

  double max_deviation = 0.0;
  for (size_t i = 0; i < num_bits; ++i) {
    for (size_t j = 0; j < hash_bits; ++j) {
      const double p = static_cast<double>(change_counts[i][j]) / total_samples;
      const double deviation = std::abs(p - 0.5);
      if (deviation > max_deviation) {
        max_deviation = deviation;
      }
    }
  }
  state.counters["max_deviation"] = max_deviation;
}

// --- PairWeight Benchmarks ---

template <typename W>
struct PairHash {
  size_t operator()(const PairWeight<W, W>& w) const { return w.Hash(); }
};

// Measures time to hash a vector of elements with sequential values.
template <typename W>
void BM_PairWeightHash_Vector(benchmark::State& state) {
  const int size = state.range(0);
  std::vector<PairWeight<W, W>> weights;
  weights.reserve(size);
  for (int i = 0; i < size; ++i) {
    weights.emplace_back(MakeWeight<W>(i), MakeWeight<W>(i + 1));
  }
  while (state.KeepRunningBatch(size)) {
    for (const auto& w : weights) {
      benchmark::DoNotOptimize(PairHash<W>()(w));
    }
  }
}

// Measures time to hash a vector of elements with random values.
template <typename W>
void BM_PairWeightHash_Random(benchmark::State& state) {
  const int size = state.range(0);
  std::vector<PairWeight<W, W>> weights;
  weights.reserve(size);
  absl::BitGen bitgen;
  for (int i = 0; i < size; ++i) {
    weights.emplace_back(MakeRandomWeight<W>(bitgen),
                         MakeRandomWeight<W>(bitgen));
  }
  while (state.KeepRunningBatch(size)) {
    for (const auto& w : weights) {
      benchmark::DoNotOptimize(PairHash<W>()(w));
    }
  }
}

// Counts collisions in a large set of 100k partially random, partially
// sequential inputs.
template <typename W>
void BM_PairWeightHash_Collisions(benchmark::State& state) {
  const int size = 100000;
  std::vector<PairWeight<W, W>> weights;
  weights.reserve(size);
  absl::BitGen bitgen;
  for (int i = 0; i < size; ++i) {
    weights.emplace_back(MakeRandomWeight<W>(bitgen), MakeWeight<W>(i));
  }
  BM_Collisions_Generic(state, weights, PairHash<W>());
}

template <typename W>
void BM_PairWeightHash_Avalanche(benchmark::State& state) {
  BM_Avalanche_Generic<PairWeight<W, W>>(
      state,
      [](absl::BitGen& bitgen) {
        return PairWeight<W, W>(MakeRandomWeight<W>(bitgen),
                                MakeRandomWeight<W>(bitgen));
      },
      PairHash<W>());
}

template <typename W>
void BM_PairWeightHash_SAC(benchmark::State& state) {
  BM_SAC_Generic<PairWeight<W, W>>(
      state,
      [](absl::BitGen& bitgen) {
        return PairWeight<W, W>(MakeRandomWeight<W>(bitgen),
                                MakeRandomWeight<W>(bitgen));
      },
      PairHash<W>());
}

// Measures time to hash pairs with swapped elements, using a vector to simulate
// memory access.
template <typename W>
void BM_PairWeightHash_SwappedElements(benchmark::State& state) {
  const int size = state.range(0);
  std::vector<PairWeight<W, W>> weights;
  weights.reserve(size * 2);
  absl::BitGen bitgen;
  for (int i = 0; i < size; ++i) {
    const W w1 = MakeRandomWeight<W>(bitgen);
    const W w2 = MakeRandomWeight<W>(bitgen);
    weights.emplace_back(w1, w2);
    weights.emplace_back(w2, w1);
  }
  while (state.KeepRunningBatch(2 * size)) {
    for (const auto& w : weights) {
      benchmark::DoNotOptimize(PairHash<W>()(w));
    }
  }
}

// Measures time to insert elements into an absl::flat_hash_set.
template <typename W>
void BM_PairWeightHash_HashSet(benchmark::State& state) {
  const int size = state.range(0);
  std::vector<PairWeight<W, W>> weights;
  weights.reserve(size);
  for (int i = 0; i < size; ++i) {
    weights.emplace_back(MakeWeight<W>(i), MakeWeight<W>(i + 1));
  }
  while (state.KeepRunningBatch(size)) {
    absl::flat_hash_set<PairWeight<W, W>, PairHash<W>> my_set;
    for (const auto& w : weights) {
      my_set.insert(w);
    }
  }
}

// Measures time to look up elements in an absl::flat_hash_set.
template <typename W>
void BM_PairWeightHash_Lookup(benchmark::State& state) {
  const int size = state.range(0);
  std::vector<PairWeight<W, W>> weights;
  weights.reserve(size);
  for (int i = 0; i < size; ++i) {
    weights.emplace_back(MakeWeight<W>(i), MakeWeight<W>(i + 1));
  }
  absl::flat_hash_set<PairWeight<W, W>, PairHash<W>> my_set(weights.begin(),
                                                            weights.end());

  std::vector<PairWeight<W, W>> lookup_keys;
  lookup_keys.reserve(size);
  for (int i = 0; i < size; ++i) {
    if (i % 2 == 0) {
      lookup_keys.push_back(weights[i]);
    } else {
      lookup_keys.emplace_back(MakeWeight<W>(i + size),
                               MakeWeight<W>(i + size + 1));
    }
  }

  while (state.KeepRunningBatch(size)) {
    for (const auto& w : lookup_keys) {
      benchmark::DoNotOptimize(my_set.contains(w));
    }
  }
}

// --- Direct Weight Benchmarks ---

template <typename W>
struct DirectHash {
  size_t operator()(const W& w) const { return w.Hash(); }
};

// Measures time to hash a vector of elements with sequential values.
template <typename W>
void BM_WeightHash_Vector(benchmark::State& state) {
  const int size = state.range(0);
  std::vector<W> weights;
  weights.reserve(size);
  for (int i = 0; i < size; ++i) {
    weights.emplace_back(MakeWeight<W>(i));
  }
  while (state.KeepRunningBatch(size)) {
    for (const auto& w : weights) {
      benchmark::DoNotOptimize(w.Hash());
    }
  }
}

// Measures time to hash a vector of elements with random values.
template <typename W>
void BM_WeightHash_Random(benchmark::State& state) {
  const int size = state.range(0);
  std::vector<W> weights;
  weights.reserve(size);
  absl::BitGen bitgen;
  for (int i = 0; i < size; ++i) {
    weights.emplace_back(MakeRandomWeight<W>(bitgen));
  }
  while (state.KeepRunningBatch(size)) {
    for (const auto& w : weights) {
      benchmark::DoNotOptimize(w.Hash());
    }
  }
}

// Counts collisions in a large set of partially random, partially
// sequential inputs.
template <typename W>
void BM_WeightHash_Collisions(benchmark::State& state) {
  const int size = 100000;
  std::vector<W> weights;
  weights.reserve(size);
  absl::BitGen bitgen;
  for (int i = 0; i < size; ++i) {
    weights.emplace_back(MakeRandomWeight<W>(bitgen));
  }
  BM_Collisions_Generic(state, weights, [](const W& w) { return w.Hash(); });
}

// Measures time to insert elements into an absl::flat_hash_set.
template <typename W>
void BM_WeightHash_HashSet(benchmark::State& state) {
  const int size = state.range(0);
  std::vector<W> weights;
  weights.reserve(size);
  for (int i = 0; i < size; ++i) {
    weights.emplace_back(MakeWeight<W>(i));
  }
  while (state.KeepRunningBatch(size)) {
    absl::flat_hash_set<W, DirectHash<W>> my_set;
    for (const auto& w : weights) {
      my_set.insert(w);
    }
  }
}

// Measures time to look up elements in an absl::flat_hash_set.
template <typename W>
void BM_WeightHash_Lookup(benchmark::State& state) {
  const int size = state.range(0);
  std::vector<W> weights;
  weights.reserve(size);
  for (int i = 0; i < size; ++i) {
    weights.emplace_back(MakeWeight<W>(i));
  }
  absl::flat_hash_set<W, DirectHash<W>> my_set(weights.begin(), weights.end());

  std::vector<W> lookup_keys;
  lookup_keys.reserve(size);
  for (int i = 0; i < size; ++i) {
    if (i % 2 == 0) {
      lookup_keys.push_back(weights[i]);
    } else {
      lookup_keys.emplace_back(MakeWeight<W>(i + size));
    }
  }

  while (state.KeepRunningBatch(size)) {
    for (const auto& w : lookup_keys) {
      benchmark::DoNotOptimize(my_set.contains(w));
    }
  }
}

// Measures the avalanche effect for direct weights.
template <typename W>
void BM_WeightHash_Avalanche(benchmark::State& state) {
  BM_Avalanche_Generic<W>(
      state, [](absl::BitGen& bitgen) { return MakeRandomWeight<W>(bitgen); },
      [](const W& w) { return w.Hash(); });
}

// Measures the Strict Avalanche Criterion (SAC) for direct weights.
template <typename W>
void BM_WeightHash_SAC(benchmark::State& state) {
  BM_SAC_Generic<W>(
      state, [](absl::BitGen& bitgen) { return MakeRandomWeight<W>(bitgen); },
      [](const W& w) { return w.Hash(); });
}

// --- PairWeight Benchmark Instantiations ---

BENCHMARK_TEMPLATE(BM_PairWeightHash_Vector, SignedLog64Weight)
    ->Range(1 << 10, 1 << 16);
BENCHMARK_TEMPLATE(BM_PairWeightHash_Vector, LogWeight)
    ->Range(1 << 10, 1 << 16);
BENCHMARK_TEMPLATE(BM_PairWeightHash_Vector, TropicalWeight)
    ->Range(1 << 10, 1 << 16);

BENCHMARK_TEMPLATE(BM_PairWeightHash_Random, SignedLog64Weight)
    ->Range(1 << 10, 1 << 16);
BENCHMARK_TEMPLATE(BM_PairWeightHash_Random, LogWeight)
    ->Range(1 << 10, 1 << 16);
BENCHMARK_TEMPLATE(BM_PairWeightHash_Random, TropicalWeight)
    ->Range(1 << 10, 1 << 16);

BENCHMARK_TEMPLATE(BM_PairWeightHash_Collisions, SignedLog64Weight);
BENCHMARK_TEMPLATE(BM_PairWeightHash_Collisions, LogWeight);
BENCHMARK_TEMPLATE(BM_PairWeightHash_Collisions, TropicalWeight);

BENCHMARK_TEMPLATE(BM_PairWeightHash_Avalanche, SignedLog64Weight);
BENCHMARK_TEMPLATE(BM_PairWeightHash_Avalanche, LogWeight);
BENCHMARK_TEMPLATE(BM_PairWeightHash_Avalanche, TropicalWeight);

BENCHMARK_TEMPLATE(BM_PairWeightHash_SAC, SignedLog64Weight);
BENCHMARK_TEMPLATE(BM_PairWeightHash_SAC, LogWeight);
BENCHMARK_TEMPLATE(BM_PairWeightHash_SAC, TropicalWeight);

BENCHMARK_TEMPLATE(BM_PairWeightHash_SwappedElements, SignedLog64Weight)
    ->Range(1 << 10, 1 << 16);
BENCHMARK_TEMPLATE(BM_PairWeightHash_SwappedElements, LogWeight)
    ->Range(1 << 10, 1 << 16);
BENCHMARK_TEMPLATE(BM_PairWeightHash_SwappedElements, TropicalWeight)
    ->Range(1 << 10, 1 << 16);

BENCHMARK_TEMPLATE(BM_PairWeightHash_HashSet, SignedLog64Weight)
    ->Range(1 << 10, 1 << 16);
BENCHMARK_TEMPLATE(BM_PairWeightHash_HashSet, LogWeight)
    ->Range(1 << 10, 1 << 16);
BENCHMARK_TEMPLATE(BM_PairWeightHash_HashSet, TropicalWeight)
    ->Range(1 << 10, 1 << 16);

BENCHMARK_TEMPLATE(BM_PairWeightHash_Lookup, SignedLog64Weight)
    ->Range(1 << 10, 1 << 16);
BENCHMARK_TEMPLATE(BM_PairWeightHash_Lookup, LogWeight)
    ->Range(1 << 10, 1 << 16);
BENCHMARK_TEMPLATE(BM_PairWeightHash_Lookup, TropicalWeight)
    ->Range(1 << 10, 1 << 16);

// --- Direct Weight Benchmark Instantiations ---

// Vector.
BENCHMARK_TEMPLATE(BM_WeightHash_Vector, SignedLog64Weight)
    ->Range(1 << 10, 1 << 16);
BENCHMARK_TEMPLATE(BM_WeightHash_Vector, LogWeight)->Range(1 << 10, 1 << 16);
BENCHMARK_TEMPLATE(BM_WeightHash_Vector, TropicalWeight)
    ->Range(1 << 10, 1 << 16);
BENCHMARK_TEMPLATE(BM_WeightHash_Vector, StringWeight<int>)
    ->Range(1 << 10, 1 << 16);
BENCHMARK_TEMPLATE(BM_WeightHash_Vector, TupleWeight<LogWeight, 3>)
    ->Range(1 << 10, 1 << 16);
BENCHMARK_TEMPLATE(BM_WeightHash_Vector, TupleWeight<SignedLog64Weight, 3>)
    ->Range(1 << 10, 1 << 16);
BENCHMARK_TEMPLATE(BM_WeightHash_Vector, TupleWeight<TropicalWeight, 3>)
    ->Range(1 << 10, 1 << 16);
BENCHMARK_TEMPLATE(BM_WeightHash_Vector, SparseTupleWeight<LogWeight, int>)
    ->Range(1 << 10, 1 << 16);
BENCHMARK_TEMPLATE(BM_WeightHash_Vector,
                   SparseTupleWeight<SignedLog64Weight, int>)
    ->Range(1 << 10, 1 << 16);
BENCHMARK_TEMPLATE(BM_WeightHash_Vector, SparseTupleWeight<TropicalWeight, int>)
    ->Range(1 << 10, 1 << 16);

// Random.
BENCHMARK_TEMPLATE(BM_WeightHash_Random, SignedLog64Weight)
    ->Range(1 << 10, 1 << 16);
BENCHMARK_TEMPLATE(BM_WeightHash_Random, LogWeight)->Range(1 << 10, 1 << 16);
BENCHMARK_TEMPLATE(BM_WeightHash_Random, TropicalWeight)
    ->Range(1 << 10, 1 << 16);
BENCHMARK_TEMPLATE(BM_WeightHash_Random, StringWeight<int>)
    ->Range(1 << 10, 1 << 16);
BENCHMARK_TEMPLATE(BM_WeightHash_Random, TupleWeight<LogWeight, 3>)
    ->Range(1 << 10, 1 << 16);
BENCHMARK_TEMPLATE(BM_WeightHash_Random, TupleWeight<SignedLog64Weight, 3>)
    ->Range(1 << 10, 1 << 16);
BENCHMARK_TEMPLATE(BM_WeightHash_Random, TupleWeight<TropicalWeight, 3>)
    ->Range(1 << 10, 1 << 16);
BENCHMARK_TEMPLATE(BM_WeightHash_Random, SparseTupleWeight<LogWeight, int>)
    ->Range(1 << 10, 1 << 16);
BENCHMARK_TEMPLATE(BM_WeightHash_Random,
                   SparseTupleWeight<SignedLog64Weight, int>)
    ->Range(1 << 10, 1 << 16);
BENCHMARK_TEMPLATE(BM_WeightHash_Random, SparseTupleWeight<TropicalWeight, int>)
    ->Range(1 << 10, 1 << 16);

// Collisions.
BENCHMARK_TEMPLATE(BM_WeightHash_Collisions, SignedLog64Weight);
BENCHMARK_TEMPLATE(BM_WeightHash_Collisions, LogWeight);
BENCHMARK_TEMPLATE(BM_WeightHash_Collisions, TropicalWeight);
BENCHMARK_TEMPLATE(BM_WeightHash_Collisions, StringWeight<int>);
BENCHMARK_TEMPLATE(BM_WeightHash_Collisions, TupleWeight<LogWeight, 3>);
BENCHMARK_TEMPLATE(BM_WeightHash_Collisions, TupleWeight<SignedLog64Weight, 3>);
BENCHMARK_TEMPLATE(BM_WeightHash_Collisions, TupleWeight<TropicalWeight, 3>);
BENCHMARK_TEMPLATE(BM_WeightHash_Collisions, SparseTupleWeight<LogWeight, int>);
BENCHMARK_TEMPLATE(BM_WeightHash_Collisions,
                   SparseTupleWeight<SignedLog64Weight, int>);
BENCHMARK_TEMPLATE(BM_WeightHash_Collisions,
                   SparseTupleWeight<TropicalWeight, int>);

// HashSet.
BENCHMARK_TEMPLATE(BM_WeightHash_HashSet, SignedLog64Weight)
    ->Range(1 << 10, 1 << 16);
BENCHMARK_TEMPLATE(BM_WeightHash_HashSet, LogWeight)->Range(1 << 10, 1 << 16);
BENCHMARK_TEMPLATE(BM_WeightHash_HashSet, TropicalWeight)
    ->Range(1 << 10, 1 << 16);
BENCHMARK_TEMPLATE(BM_WeightHash_HashSet, StringWeight<int>)
    ->Range(1 << 10, 1 << 16);
BENCHMARK_TEMPLATE(BM_WeightHash_HashSet, TupleWeight<LogWeight, 3>)
    ->Range(1 << 10, 1 << 16);
BENCHMARK_TEMPLATE(BM_WeightHash_HashSet, TupleWeight<SignedLog64Weight, 3>)
    ->Range(1 << 10, 1 << 16);
BENCHMARK_TEMPLATE(BM_WeightHash_HashSet, TupleWeight<TropicalWeight, 3>)
    ->Range(1 << 10, 1 << 16);
BENCHMARK_TEMPLATE(BM_WeightHash_HashSet, SparseTupleWeight<LogWeight, int>)
    ->Range(1 << 10, 1 << 16);
BENCHMARK_TEMPLATE(BM_WeightHash_HashSet,
                   SparseTupleWeight<SignedLog64Weight, int>)
    ->Range(1 << 10, 1 << 16);
BENCHMARK_TEMPLATE(BM_WeightHash_HashSet,
                   SparseTupleWeight<TropicalWeight, int>)
    ->Range(1 << 10, 1 << 16);

// Lookup.
BENCHMARK_TEMPLATE(BM_WeightHash_Lookup, SignedLog64Weight)
    ->Range(1 << 10, 1 << 16);
BENCHMARK_TEMPLATE(BM_WeightHash_Lookup, LogWeight)->Range(1 << 10, 1 << 16);
BENCHMARK_TEMPLATE(BM_WeightHash_Lookup, TropicalWeight)
    ->Range(1 << 10, 1 << 16);
BENCHMARK_TEMPLATE(BM_WeightHash_Lookup, StringWeight<int>)
    ->Range(1 << 10, 1 << 16);
BENCHMARK_TEMPLATE(BM_WeightHash_Lookup, TupleWeight<LogWeight, 3>)
    ->Range(1 << 10, 1 << 16);
BENCHMARK_TEMPLATE(BM_WeightHash_Lookup, TupleWeight<SignedLog64Weight, 3>)
    ->Range(1 << 10, 1 << 16);
BENCHMARK_TEMPLATE(BM_WeightHash_Lookup, TupleWeight<TropicalWeight, 3>)
    ->Range(1 << 10, 1 << 16);
BENCHMARK_TEMPLATE(BM_WeightHash_Lookup, SparseTupleWeight<LogWeight, int>)
    ->Range(1 << 10, 1 << 16);
BENCHMARK_TEMPLATE(BM_WeightHash_Lookup,
                   SparseTupleWeight<SignedLog64Weight, int>)
    ->Range(1 << 10, 1 << 16);
BENCHMARK_TEMPLATE(BM_WeightHash_Lookup, SparseTupleWeight<TropicalWeight, int>)
    ->Range(1 << 10, 1 << 16);

// Avalanche.
BENCHMARK_TEMPLATE(BM_WeightHash_Avalanche, SignedLog64Weight);
BENCHMARK_TEMPLATE(BM_WeightHash_Avalanche, LogWeight);
BENCHMARK_TEMPLATE(BM_WeightHash_Avalanche, TropicalWeight);
BENCHMARK_TEMPLATE(BM_WeightHash_Avalanche, TupleWeight<LogWeight, 3>);
BENCHMARK_TEMPLATE(BM_WeightHash_Avalanche, TupleWeight<SignedLog64Weight, 3>);
BENCHMARK_TEMPLATE(BM_WeightHash_Avalanche, TupleWeight<TropicalWeight, 3>);

// SAC.
BENCHMARK_TEMPLATE(BM_WeightHash_SAC, SignedLog64Weight);
BENCHMARK_TEMPLATE(BM_WeightHash_SAC, LogWeight);
BENCHMARK_TEMPLATE(BM_WeightHash_SAC, TropicalWeight);
BENCHMARK_TEMPLATE(BM_WeightHash_SAC, TupleWeight<LogWeight, 3>);
BENCHMARK_TEMPLATE(BM_WeightHash_SAC, TupleWeight<SignedLog64Weight, 3>);
BENCHMARK_TEMPLATE(BM_WeightHash_SAC, TupleWeight<TropicalWeight, 3>);

}  // namespace
}  // namespace fst
