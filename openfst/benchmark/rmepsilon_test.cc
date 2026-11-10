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
// Benchmark for FST epsilon removal.

#include "openfst/lib/rmepsilon.h"

#include <cstdint>
#include <memory>
#include <string>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "absl/flags/flag.h"
#include "absl/log/check.h"
#include "absl/log/die_if_null.h"
#include "benchmark/benchmark.h"  // IWYU pragma: keep
#include "openfst/lib/arc.h"
#include "openfst/lib/expanded-fst.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/properties.h"
#include "openfst/lib/vector-fst.h"

// Test data from www.openfst.org/twiki/bin/view/FST/FstExamples

ABSL_FLAG(std::string, acyclic_fst,
          "openfst/benchmark/testdata/string.fst",
          "Acyclic FST for epsilon removal.");

ABSL_FLAG(std::string, cyclic_fst,
          "openfst/benchmark/testdata/lexicon.fst",
          "Cyclic FST for epsilon removal.");

namespace fst {
namespace {

using Arc = StdArc;
using FstType = StdFst;

// Tests destructive epsilon removal on acyclic input.
static void BM_AcyclicRmEpsilon(benchmark::State& state) {
  std::unique_ptr<const FstType> fst(
      ABSL_DIE_IF_NULL(FstType::Read(JoinPathRespectAbsolute(
          std::string("."), absl::GetFlag(FLAGS_acyclic_fst)))));
  VectorFst<Arc> ifst(*fst);
  const uint64_t props = ifst.Properties(kFstProperties, true);
  CHECK(props & kAcyclic);
  CHECK(props & kEpsilons);
  for (auto _ : state) {
    VectorFst<Arc> ofst(ifst);
    RmEpsilon(&ofst);
    benchmark::DoNotOptimize(ofst);
  }
}

// Tests destructive epsilon removal on cyclic input.
static void BM_CyclicRmEpsilon(benchmark::State& state) {
  std::unique_ptr<const FstType> fst(
      ABSL_DIE_IF_NULL(FstType::Read(JoinPathRespectAbsolute(
          std::string("."), absl::GetFlag(FLAGS_cyclic_fst)))));
  VectorFst<Arc> ifst(*fst);
  const uint64_t props = ifst.Properties(kFstProperties, true);
  CHECK(props & kCyclic);
  CHECK(props & kEpsilons);
  for (auto _ : state) {
    VectorFst<Arc> ofst(ifst);
    RmEpsilon(&ofst);
    benchmark::DoNotOptimize(ofst);
  }
}

// Tests on-the-fly epsilon removal on acyclic input.
static void BM_AcyclicRmEpsilonFst(benchmark::State& state) {
  std::unique_ptr<const FstType> fst(
      ABSL_DIE_IF_NULL(FstType::Read(JoinPathRespectAbsolute(
          std::string("."), absl::GetFlag(FLAGS_acyclic_fst)))));
  VectorFst<Arc> ifst(*fst);
  const uint64_t props = ifst.Properties(kFstProperties, true);
  CHECK(props & kAcyclic);
  CHECK(props & kEpsilons);
  for (auto _ : state) {
    RmEpsilonFst<Arc> ofst(ifst);
    benchmark::DoNotOptimize(CountStates(ofst));
  }
}

// Tests on-the-fly epsilon removal on cyclic input.
static void BM_CyclicRmEpsilonFst(benchmark::State& state) {
  std::unique_ptr<const FstType> fst(
      ABSL_DIE_IF_NULL(FstType::Read(JoinPathRespectAbsolute(
          std::string("."), absl::GetFlag(FLAGS_cyclic_fst)))));
  VectorFst<Arc> ifst(*fst);
  const uint64_t props = ifst.Properties(kFstProperties, true);
  CHECK(props & kCyclic);
  CHECK(props & kEpsilons);
  for (auto _ : state) {
    RmEpsilonFst<Arc> ofst(ifst);
    benchmark::DoNotOptimize(CountStates(ofst));
  }
}

BENCHMARK(BM_AcyclicRmEpsilon);
BENCHMARK(BM_CyclicRmEpsilon);

BENCHMARK(BM_AcyclicRmEpsilonFst);
BENCHMARK(BM_CyclicRmEpsilonFst);

}  // namespace
}  // namespace fst
