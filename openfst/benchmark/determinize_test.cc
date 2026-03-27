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
// Benchmark for FST determinization.

#include "openfst/lib/determinize.h"

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
#include "openfst/lib/expanded-fst.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/project.h"
#include "openfst/lib/properties.h"
#include "openfst/lib/vector-fst.h"

// Test data from www.openfst.org/twiki/bin/view/FST/FstExamples

ABSL_FLAG(std::string, input_fst,
          "openfst/benchmark/testdata/lexicon_disamb.fst",
          "fst to determinize");

namespace fst {
namespace {

using Arc = StdArc;
using FstType = StdFst;

// Tests determinization of an acceptor.
static void BM_DeterminizeAccept(benchmark::State& state) {
  std::unique_ptr<const FstType> fst(
      ABSL_DIE_IF_NULL(FstType::Read(JoinPathRespectAbsolute(
          std::string("."), absl::GetFlag(FLAGS_input_fst)))));

  fst->Properties(kFstProperties, true);
  VectorFst<Arc> f(*fst);
  Project(&f, ProjectType::INPUT);
  for (auto _ : state) {
    VectorFst<Arc> det;
    Determinize(f, &det);
  }
}

// Tests on-the-fly determinization of an acceptor.
static void BM_DeterminizeFstAccept(benchmark::State& state) {
  std::unique_ptr<const FstType> fst(
      ABSL_DIE_IF_NULL(FstType::Read(JoinPathRespectAbsolute(
          std::string("."), absl::GetFlag(FLAGS_input_fst)))));

  fst->Properties(kFstProperties, true);
  VectorFst<Arc> f(*fst);
  Project(&f, ProjectType::INPUT);
  for (auto _ : state) {
    DeterminizeFst<Arc> det(f);
    CountStates(det);
  }
}

// Tests determinization of a transducer.
static void BM_DeterminizeTrans(benchmark::State& state) {
  std::unique_ptr<const FstType> fst(
      ABSL_DIE_IF_NULL(FstType::Read(JoinPathRespectAbsolute(
          std::string("."), absl::GetFlag(FLAGS_input_fst)))));

  const int64_t props = fst->Properties(kFstProperties, true);
  CHECK(!(props & kAcceptor));
  VectorFst<Arc> f(*fst);
  for (auto _ : state) {
    VectorFst<Arc> det;
    Determinize(f, &det);
  }
}

// Tests on-the-fly determinization of a transducer.
static void BM_DeterminizeFstTrans(benchmark::State& state) {
  std::unique_ptr<const FstType> fst(
      ABSL_DIE_IF_NULL(FstType::Read(JoinPathRespectAbsolute(
          std::string("."), absl::GetFlag(FLAGS_input_fst)))));

  const int64_t props = fst->Properties(kFstProperties, true);
  CHECK(!(props & kAcceptor));
  VectorFst<Arc> f(*fst);
  for (auto _ : state) {
    DeterminizeFst<Arc> det(f);
    CountStates(det);
  }
}

BENCHMARK(BM_DeterminizeAccept);
BENCHMARK(BM_DeterminizeTrans);

BENCHMARK(BM_DeterminizeFstAccept);
BENCHMARK(BM_DeterminizeFstTrans);

}  // namespace
}  // namespace fst
