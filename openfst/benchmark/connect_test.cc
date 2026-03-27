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
// Benchmark for FST connection/trimming.

#include "openfst/lib/connect.h"

#include <memory>
#include <string>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "absl/flags/flag.h"
#include "absl/log/die_if_null.h"
#include "absl/strings/string_view.h"
#include "benchmark/benchmark.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/memory.h"
#include "openfst/lib/vector-fst.h"

// Test data from www.openfst.org/twiki/bin/view/FST/FstExamples

ABSL_FLAG(std::string, acyclic_fst,
          "openfst/benchmark/testdata/lattice.fst",
          "acyclic FST to connect");

ABSL_FLAG(std::string, cyclic_fst,
          "openfst/benchmark/testdata/disconnected.lm.fst",
          "cyclic fst to connect");

namespace fst {
namespace {

using Arc = StdArc;
using FstType = StdFst;

// Count i/o epsilons - forces walk of FST
template <class Arc>
ssize_t CountEpsilons(const Fst<Arc>& fst) {
  ssize_t neps = 0;
  for (StateIterator<Fst<Arc>> siter(fst); !siter.Done(); siter.Next()) {
    typename Arc::StateId s = siter.Value();
    for (ArcIterator<Fst<Arc>> aiter(fst, s); !aiter.Done(); aiter.Next()) {
      const Arc& arc = aiter.Value();
      if (arc.ilabel == 0 && arc.olabel == 0) ++neps;
    }
  }
  return neps;
}

using FastArcAllocator = BlockAllocator<Arc>;
using FastState = VectorState<Arc, FastArcAllocator>;
using FastVectorFst = VectorFst<Arc, FastState>;

// Tests connection.
static void ConnectTest(absl::string_view file, benchmark::State& state) {
  std::unique_ptr<const FstType> fst(ABSL_DIE_IF_NULL(FstType::Read(file)));
  for (auto _ : state) {
    FastVectorFst cfst(*fst);
    Connect(&cfst);
    CountEpsilons(cfst);
  }
}

// Tests acyclic connection.
static void BM_AcyclicConnect(benchmark::State& state) {
  ConnectTest(JoinPathRespectAbsolute(std::string("."),
                                            absl::GetFlag(FLAGS_acyclic_fst)),
              state);
}

// Tests cyclic connection.
static void BM_CyclicConnect(benchmark::State& state) {
  ConnectTest(JoinPathRespectAbsolute(std::string("."),
                                            absl::GetFlag(FLAGS_cyclic_fst)),
              state);
}

BENCHMARK(BM_AcyclicConnect);

BENCHMARK(BM_CyclicConnect);

}  // namespace
}  // namespace fst
