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

#include <climits>
#include <cstddef>
#include <type_traits>
#include <vector>

#include "absl/flags/flag.h"
#include "benchmark/benchmark.h"
#include "openfst/lib/pair-weight.h"
#include "openfst/lib/set-weight.h"
#include "openfst/lib/signed-log-weight.h"

#define LOG_TRAIT(type, trait) \
  LOG(INFO) << #trait "<" #type ">: " << std::boolalpha << trait<type>

namespace fst {
namespace {

using UISetWeight = SetWeight<int, SET_UNION_INTERSECT>;

// Tests move constructor of SetWeight.
static void BM_SetWeight(benchmark::State& state) {
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
static void BM_SignedLogWeight(benchmark::State& state) {
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

// Benchmark for PairWeight::Hash
static void BM_PairWeightHash_New(benchmark::State& state) {
  PairWeight<SignedLog64Weight, SignedLog64Weight> w(
      SignedLog64Weight(SignedLog64Weight::W1(-1), SignedLog64Weight::W2(1)),
      SignedLog64Weight(SignedLog64Weight::W1(1), SignedLog64Weight::W2(2)));
  for (auto _ : state) {
    benchmark::DoNotOptimize(w.Hash());
  }
}
BENCHMARK(BM_PairWeightHash_New);

static void BM_PairWeightHash_Old(benchmark::State& state) {
  PairWeight<SignedLog64Weight, SignedLog64Weight> w(
      SignedLog64Weight(SignedLog64Weight::W1(-1), SignedLog64Weight::W2(1)),
      SignedLog64Weight(SignedLog64Weight::W1(1), SignedLog64Weight::W2(2)));
  for (auto _ : state) {
    const auto h1 = w.Value1().Hash();
    const auto h2 = w.Value2().Hash();
    static constexpr int lshift = 5;
    static constexpr int rshift = CHAR_BIT * sizeof(size_t) - 5;
    size_t h = h1 << lshift ^ h1 >> rshift ^ h2;
    benchmark::DoNotOptimize(h);
  }
}
BENCHMARK(BM_PairWeightHash_Old);

}  // namespace
}  // namespace fst
