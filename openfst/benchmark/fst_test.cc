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
// Benchmark for FST classes.

#include "openfst/lib/fst.h"

#include <cstddef>
#include <memory>
#include <string>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "absl/flags/flag.h"
#include "absl/log/check.h"
#include "absl/log/die_if_null.h"
#include "benchmark/benchmark.h"
#include "openfst/extensions/ngram/ngram-fst.h"
#include "openfst/lib/arc-map.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/compact-fst.h"
#include "openfst/lib/const-fst.h"
#include "openfst/lib/expanded-fst.h"
#include "openfst/lib/matcher-fst.h"
#include "openfst/lib/matcher.h"
#include "openfst/lib/memory.h"
#include "openfst/lib/properties.h"
#include "openfst/lib/vector-fst.h"

// Test data from www.openfst.org/twiki/bin/view/FST/FstExamples
// Must be a `StdFst` (such as `StdVectorFst`).
ABSL_FLAG(std::string, input_fst,
          "openfst/benchmark/testdata/wotw.lm.fst",
          "FST to test.");

namespace fst {
namespace {

// Tests class-specialized state and arc iterators
template <typename F>
static void BM_SpecializedIterators(benchmark::State& state) {
  std::unique_ptr<StdFst> ifst(
      ABSL_DIE_IF_NULL(StdFst::Read(JoinPathRespectAbsolute(
          std::string("."), absl::GetFlag(FLAGS_input_fst)))));

  F ofst(*ifst);
  size_t narcs = 0;
  for (auto _ : state) {
    for (StateIterator<F> siter(ofst); !siter.Done(); siter.Next()) {
      using StateId = typename F::Arc::StateId;
      StateId s = siter.Value();
      for (ArcIterator<F> aiter(ofst, s); !aiter.Done(); aiter.Next()) {
        ++narcs;
      }
    }
  }
  CHECK_GT(narcs, 0);
  state.SetItemsProcessed(narcs);
}

// Tests generic state and arc iterators
template <typename F>
static void BM_GenericIterators(benchmark::State& state) {
  std::unique_ptr<StdFst> ifst(
      ABSL_DIE_IF_NULL(StdFst::Read(JoinPathRespectAbsolute(
          std::string("."), absl::GetFlag(FLAGS_input_fst)))));

  using Arc = typename F::Arc;
  using StateId = typename Arc::StateId;
  using FstType = Fst<Arc>;
  F sfst(*ifst);
  FstType* ofst = &sfst;
  size_t narcs = 0;
  for (auto _ : state) {
    for (StateIterator<FstType> siter(*ofst); !siter.Done(); siter.Next()) {
      StateId s = siter.Value();
      for (ArcIterator<FstType> aiter(*ofst, s); !aiter.Done(); aiter.Next()) {
        ++narcs;
      }
    }
  }
  CHECK_GT(narcs, 0);
  state.SetItemsProcessed(narcs);
}

// Tests class-specialized matcher
template <typename F>
static void BM_SpecializedMatcher(benchmark::State& state) {
  std::unique_ptr<StdFst> ifst(
      ABSL_DIE_IF_NULL(StdFst::Read(JoinPathRespectAbsolute(
          std::string("."), absl::GetFlag(FLAGS_input_fst)))));

  F ofst(*ifst);
  ofst.Properties(kFstProperties, true);
  Matcher<F> matcher(ofst, MATCH_INPUT);
  size_t nmatch = 0, nstates = 0;
  for (auto _ : state) {
    for (StateIterator<F> siter(ofst); !siter.Done(); siter.Next()) {
      using StateId = typename F::Arc::StateId;
      StateId s = siter.Value();
      matcher.SetState(s);
      nmatch += matcher.Find(7);
      ++nstates;
    }
  }
  CHECK_GT(nmatch, 0);
  state.SetItemsProcessed(nstates);
}

// Tests generic matcher
template <typename F>
static void BM_GenericMatcher(benchmark::State& state) {
  std::unique_ptr<StdFst> ifst(
      ABSL_DIE_IF_NULL(StdFst::Read(JoinPathRespectAbsolute(
          std::string("."), absl::GetFlag(FLAGS_input_fst)))));

  using Arc = typename F::Arc;
  using FstType = Fst<Arc>;
  F sfst(*ifst);
  sfst.Properties(kFstProperties, true);
  FstType* ofst = &sfst;
  Matcher<FstType> matcher(*ofst, MATCH_INPUT);
  size_t nmatch = 0, nstates = 0;
  for (auto _ : state) {
    for (StateIterator<FstType> siter(*ofst); !siter.Done(); siter.Next()) {
      using StateId = typename Arc::StateId;
      StateId s = siter.Value();
      matcher.SetState(s);
      nmatch += matcher.Find(7);
      ++nstates;
    }
  }
  CHECK_GT(nmatch, 0);
  state.SetItemsProcessed(nstates);
}

// Tests copy of input FST via copy constructor.
template <typename F>
static void BM_IntrinsicCopy(benchmark::State& state) {
  std::unique_ptr<StdFst> ifst(
      ABSL_DIE_IF_NULL(StdFst::Read(JoinPathRespectAbsolute(
          std::string("."), absl::GetFlag(FLAGS_input_fst)))));
  for (auto _ : state) {
    F ofst(*ifst);
  }
  state.SetItemsProcessed(state.iterations() * CountArcs(*ifst));
}

// Tests copy of input FST via mutators
template <typename F>
static void BM_ExtrinsicCopy(benchmark::State& state) {
  std::unique_ptr<StdFst> ifst(
      ABSL_DIE_IF_NULL(StdFst::Read(JoinPathRespectAbsolute(
          std::string("."), absl::GetFlag(FLAGS_input_fst)))));

  size_t narcs = 0;
  for (auto _ : state) {
    F ofst;
    for (StateIterator<StdFst> siter(*ifst); !siter.Done(); siter.Next()) {
      StdArc::StateId s = siter.Value();
      ofst.AddState();
      ofst.SetFinal(s, ifst->Final(s));
      for (ArcIterator<StdFst> aiter(*ifst, s); !aiter.Done(); aiter.Next()) {
        const StdArc& arc = aiter.Value();
        ofst.AddArc(s, arc);
        ++narcs;
      }
    }
    ofst.SetStart(ifst->Start());
  }
  state.SetItemsProcessed(narcs);
}

// Tests Fst::Properties().
template <typename F>
static void BM_Properties(benchmark::State& state) {
  std::unique_ptr<StdFst> fst(
      ABSL_DIE_IF_NULL(StdFst::Read(JoinPathRespectAbsolute(
          std::string("."), absl::GetFlag(FLAGS_input_fst)))));

  // Compute the properties.
  fst->Properties(kFstProperties, true);
  for (auto _ : state) {
    // Just retrieve them.
    benchmark::DoNotOptimize(fst->Properties(kFstProperties, false));
  }
}

using StdToLogArcMapFst = ArcMapFst<StdArc, LogArc, StdToLogMapper>;

using BlockArcAllocator = BlockAllocator<StdArc>;
using BlockState = VectorState<StdArc, BlockArcAllocator>;
using BlockVectorFst = VectorFst<StdArc, BlockState>;

using PoolArcAllocator = PoolAllocator<StdArc>;
using PoolState = VectorState<StdArc, PoolArcAllocator>;
using PoolVectorFst = VectorFst<StdArc, PoolState>;

using StdNGramFst = NGramFst<StdArc>;

BENCHMARK_TEMPLATE(BM_SpecializedIterators, StdVectorFst);
BENCHMARK_TEMPLATE(BM_SpecializedIterators, StdToLogArcMapFst);
BENCHMARK_TEMPLATE(BM_SpecializedIterators, BlockVectorFst);
BENCHMARK_TEMPLATE(BM_SpecializedIterators, PoolVectorFst);
BENCHMARK_TEMPLATE(BM_SpecializedIterators, StdConstFst);
BENCHMARK_TEMPLATE(BM_SpecializedIterators, StdCompactAcceptorFst);
BENCHMARK_TEMPLATE(BM_SpecializedIterators, StdNGramFst);

BENCHMARK_TEMPLATE(BM_GenericIterators, StdVectorFst);
BENCHMARK_TEMPLATE(BM_GenericIterators, StdToLogArcMapFst);
BENCHMARK_TEMPLATE(BM_GenericIterators, BlockVectorFst);
BENCHMARK_TEMPLATE(BM_GenericIterators, PoolVectorFst);
BENCHMARK_TEMPLATE(BM_GenericIterators, StdConstFst);
BENCHMARK_TEMPLATE(BM_GenericIterators, StdCompactAcceptorFst);
BENCHMARK_TEMPLATE(BM_GenericIterators, StdNGramFst);

BENCHMARK_TEMPLATE(BM_SpecializedMatcher, StdVectorFst);
BENCHMARK_TEMPLATE(BM_SpecializedMatcher, BlockVectorFst);
BENCHMARK_TEMPLATE(BM_SpecializedMatcher, PoolVectorFst);
BENCHMARK_TEMPLATE(BM_SpecializedMatcher, StdConstFst);
BENCHMARK_TEMPLATE(BM_SpecializedMatcher, StdCompactAcceptorFst);
BENCHMARK_TEMPLATE(BM_SpecializedMatcher, StdNGramFst);

BENCHMARK_TEMPLATE(BM_GenericMatcher, StdVectorFst);
BENCHMARK_TEMPLATE(BM_GenericMatcher, BlockVectorFst);
BENCHMARK_TEMPLATE(BM_GenericMatcher, PoolVectorFst);
BENCHMARK_TEMPLATE(BM_GenericMatcher, StdConstFst);
BENCHMARK_TEMPLATE(BM_GenericMatcher, StdCompactAcceptorFst);
BENCHMARK_TEMPLATE(BM_GenericMatcher, StdNGramFst);

BENCHMARK_TEMPLATE(BM_IntrinsicCopy, StdVectorFst);
BENCHMARK_TEMPLATE(BM_IntrinsicCopy, BlockVectorFst);
BENCHMARK_TEMPLATE(BM_IntrinsicCopy, PoolVectorFst);
BENCHMARK_TEMPLATE(BM_IntrinsicCopy, StdConstFst);
BENCHMARK_TEMPLATE(BM_IntrinsicCopy, StdCompactAcceptorFst);
BENCHMARK_TEMPLATE(BM_IntrinsicCopy, StdNGramFst);

BENCHMARK_TEMPLATE(BM_ExtrinsicCopy, StdVectorFst);
BENCHMARK_TEMPLATE(BM_ExtrinsicCopy, BlockVectorFst);
BENCHMARK_TEMPLATE(BM_ExtrinsicCopy, PoolVectorFst);

BENCHMARK_TEMPLATE(BM_Properties, StdVectorFst);
BENCHMARK_TEMPLATE(BM_Properties, StdConstFst);

}  // namespace
}  // namespace fst
