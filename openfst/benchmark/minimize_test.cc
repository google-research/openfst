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
// Benchmark for FST minimization..

#include "openfst/lib/minimize.h"

#include <memory>
#include <string>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "absl/flags/flag.h"
#include "absl/log/check.h"
#include "benchmark/benchmark.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/determinize.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/properties.h"
#include "openfst/lib/vector-fst.h"

ABSL_FLAG(std::string, cyclic_fst,
          "openfst/benchmark/testdata/lexicon_disamb.fst",
          "Cyclic FST to minimize.");

ABSL_FLAG(std::string, acyclic_fst,
          "openfst/benchmark/testdata/lattice.fst",
          "Acyclic FST to minimize.");

namespace fst {
namespace {

using Arc = StdArc;
using FstType = StdFst;

void MinimizeTransducer(const StdFst& fst, benchmark::State& state) {
  VectorFst<Arc> det;
  Determinize(fst, &det);
  det.Properties(kFstProperties, true);
  for (auto _ : state) {
    VectorFst<Arc> min(det);
    Minimize(&min);
  }
}

// Tests minimization of cyclic transducer.
static void BM_MinimizeCyclicTransducer(benchmark::State& state) {
  std::unique_ptr<const FstType> cyclic(
      FstType::Read(JoinPathRespectAbsolute(
          std::string("."), absl::GetFlag(FLAGS_cyclic_fst))));
  CHECK(cyclic != nullptr);
  const int64_t props = cyclic->Properties(kFstProperties, true);
  CHECK(props & kCyclic);
  MinimizeTransducer(*cyclic, state);
}

// Tests minimization of acyclic transducer.
static void BM_MinimizeAcyclicTransducer(benchmark::State& state) {
  std::unique_ptr<const FstType> acyclic(
      FstType::Read(JoinPathRespectAbsolute(
          std::string("."), absl::GetFlag(FLAGS_acyclic_fst))));
  CHECK(acyclic != nullptr);
  const int64_t props = acyclic->Properties(kFstProperties, true);
  CHECK(props & kAcyclic);
  MinimizeTransducer(*acyclic, state);
}

BENCHMARK(BM_MinimizeCyclicTransducer);
BENCHMARK(BM_MinimizeAcyclicTransducer);

}  // namespace
}  // namespace fst
