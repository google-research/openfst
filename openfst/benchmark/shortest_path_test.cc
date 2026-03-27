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
// Benchmark for FST shortest path.

#include <cstdint>
#include <memory>
#include <string>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "absl/flags/flag.h"
#include "absl/log/check.h"
#include "absl/log/die_if_null.h"
#include "benchmark/benchmark.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/properties.h"
#include "openfst/lib/shortest-path.h"
#include "openfst/lib/vector-fst.h"

// Test data from www.openfst.org/twiki/bin/view/FST/FstExamples

ABSL_FLAG(std::string, acyclic_fst,
          "openfst/benchmark/testdata/lattice.fst",
          "acyclic FST for shortest path");

ABSL_FLAG(std::string, cyclic_fst,
          "openfst/benchmark/testdata/wotw.lm.fst",
          "cyclic FST for shortest path");

namespace fst {
namespace {

using Arc = StdArc;
using FstType = StdFst;

// Tests shortest path on acyclic input.
static void BM_AcyclicShortestPath(benchmark::State& state) {
  std::unique_ptr<const FstType> fst(
      ABSL_DIE_IF_NULL(FstType::Read(JoinPathRespectAbsolute(
          std::string("."), absl::GetFlag(FLAGS_acyclic_fst)))));
  int64_t props = fst->Properties(kFstProperties, true);
  CHECK(props & kAcyclic);
  for (auto _ : state) {
    VectorFst<Arc> path;
    ShortestPath(*fst, &path);
  }
}

// Tests shortest path on cyclic input.
static void BM_CyclicShortestPath(benchmark::State& state) {
  std::unique_ptr<const FstType> fst(
      ABSL_DIE_IF_NULL(FstType::Read(JoinPathRespectAbsolute(
          std::string("."), absl::GetFlag(FLAGS_cyclic_fst)))));
  int64_t props = fst->Properties(kFstProperties, true);
  CHECK(props & kCyclic);
  for (auto _ : state) {
    VectorFst<Arc> path;
    ShortestPath(*fst, &path);
  }
}

BENCHMARK(BM_AcyclicShortestPath);

BENCHMARK(BM_CyclicShortestPath);

}  // namespace
}  // namespace fst
