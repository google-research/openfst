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
// Benchmark for CompactFst classes.

#include <climits>
#include <cstdint>
#include <memory>
#include <string>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "absl/base/casts.h"
#include "absl/flags/flag.h"
#include "absl/log/check.h"
#include "absl/log/die_if_null.h"
#include "absl/log/log.h"
#include "benchmark/benchmark.h"
#include "openfst/lib/accumulator.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/arcfilter.h"
#include "openfst/lib/cache.h"
#include "openfst/lib/cc-visitors.h"
#include "openfst/lib/compact-fst.h"
#include "openfst/lib/compose.h"
#include "openfst/lib/const-fst.h"
#include "openfst/lib/dfs-visit.h"
#include "openfst/lib/fst-decl.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/lookahead-matcher.h"
#include "openfst/lib/matcher-fst.h"
#include "openfst/lib/matcher.h"
#include "openfst/lib/project.h"
#include "openfst/lib/properties.h"
#include "openfst/lib/queue.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/lib/visit.h"

ABSL_FLAG(bool, fst_safe_copy, true, "");
ABSL_FLAG(std::string, input_fst,
          "openfst/benchmark/testdata/lexicon_opt.fst",
          "FST to compact");

ABSL_FLAG(std::string, lm_fst,
          "openfst/benchmark/testdata/wotw.lm.fst",
          "2nd FST for compose");
ABSL_FLAG(std::string, lookahead_fst,
          "openfst/benchmark/testdata/lexicon_opt.fst",
          "1st FST for compose");

namespace fst {
namespace {

// Functors for converting to target type T.
template <class F>
struct Convert {
  F* operator()(const Fst<typename F::Arc>& fst) { return new F(fst); }
};

// Allows changing the default cache options for CompactFst.
template <class A, class C, class S>
struct Convert<CompactFst<A, C, S>> {
  CompactFst<A, C, S>* operator()(const Fst<A>& fst) {
    return new CompactFst<A, C, S>(
        fst, CompactFstOptions(CacheOptions(
                 absl::GetFlag(FLAGS_fst_default_cache_gc),
                 absl::GetFlag(FLAGS_fst_default_cache_gc_limit))));
  }
};

// F is the concrete Fst type representing the input Fst, G is the Fst type
// seen by Visit.
template <class F, class G>
static void BM_BfsVisit(benchmark::State& state) {
  using Arc = typename F::Arc;
  std::unique_ptr<Fst<Arc>> fst(
      ABSL_DIE_IF_NULL(Fst<Arc>::Read(JoinPathRespectAbsolute(
          std::string("."), absl::GetFlag(FLAGS_input_fst)))));

  std::unique_ptr<F> f(Convert<F>()(*fst));
  for (auto _ : state) {
    std::unique_ptr<F> fp(f->Copy(absl::GetFlag(FLAGS_fst_safe_copy)));
    PartialVisitor<Arc> visitor(INT_MAX);
    FifoQueue<typename Arc::StateId> queue;
    AnyArcFilter<Arc> arc_filter;
    Visit(absl::implicit_cast<const G&>(*fp), &visitor, &queue, arc_filter);
  }
}

BENCHMARK_TEMPLATE(BM_BfsVisit, StdConstFst, StdFst);
BENCHMARK_TEMPLATE(BM_BfsVisit, StdConstFst, StdConstFst);
BENCHMARK_TEMPLATE(BM_BfsVisit, StdCompactUnweightedFst, StdFst);
BENCHMARK_TEMPLATE(BM_BfsVisit, StdCompactUnweightedFst,
                   StdCompactUnweightedFst);

// F is the concrete Fst type representing the input Fst, G is the Fst type
// seen by DfsVisit.
template <class F, class G>
static void BM_DfsVisit(benchmark::State& state) {
  using Arc = typename F::Arc;
  std::unique_ptr<Fst<Arc>> fst(
      ABSL_DIE_IF_NULL(Fst<Arc>::Read(JoinPathRespectAbsolute(
          std::string("."), absl::GetFlag(FLAGS_input_fst)))));

  std::unique_ptr<F> f(Convert<F>()(*fst));
  for (auto _ : state) {
    std::unique_ptr<F> fp(f->Copy(absl::GetFlag(FLAGS_fst_safe_copy)));
    uint64_t props;
    SccVisitor<Arc> visitor(&props);
    AnyArcFilter<Arc> arc_filter;
    DfsVisit(absl::implicit_cast<const G&>(*fp), &visitor, arc_filter);
  }
}

BENCHMARK_TEMPLATE(BM_DfsVisit, StdConstFst, StdFst);
BENCHMARK_TEMPLATE(BM_DfsVisit, StdConstFst, StdConstFst);
BENCHMARK_TEMPLATE(BM_DfsVisit, StdCompactUnweightedFst, StdFst);
BENCHMARK_TEMPLATE(BM_DfsVisit, StdCompactUnweightedFst,
                   StdCompactUnweightedFst);

template <typename F>
struct CustomLookAheadRelabeler {
  using Type = LabelLookAheadRelabeler<typename F::Arc>;
};

// Takes a lookahead Fst of type F and a standardFst and composes.
template <typename F>
static void BM_LookAheadCompose(benchmark::State& state) {
  using Arc = typename F::Arc;
  using LookAheadRelabeler = typename CustomLookAheadRelabeler<F>::Type;
  using CustomOLabelLookAheadFst =
      MatcherFst<F,
                 LabelLookAheadMatcher<SortedMatcher<F>, olabel_lookahead_flags,
                                       FastLogAccumulator<Arc>>,
                 olabel_lookahead_fst_type, LookAheadRelabeler>;

  std::unique_ptr<Fst<Arc>> fst1(
      ABSL_DIE_IF_NULL(Fst<Arc>::Read(JoinPathRespectAbsolute(
          std::string("."), absl::GetFlag(FLAGS_lookahead_fst)))));

  std::unique_ptr<Fst<Arc>> fst2(
      ABSL_DIE_IF_NULL(Fst<Arc>::Read(JoinPathRespectAbsolute(
          std::string("."), absl::GetFlag(FLAGS_lm_fst)))));

  CustomOLabelLookAheadFst f1(*fst1);

  StdVectorFst f2(*fst2);
  LabelLookAheadRelabeler<Arc>::Relabel(&f2, f1, true);

  Project(&f2, ProjectType::INPUT);
  f1.Properties(kFstProperties, true);
  f2.Properties(kFstProperties, true);
  StdVectorFst output;
  ComposeOptions opts(false);
  for (auto _ : state) {
    Compose(f1, f2, &output, opts);
    if (state.iterations() == 0) {
      // An insufficiently tested lookahead FST may run suspiciously quickly,
      // but have the wrong number of states.  Use --vmodule=compact_test=2
      // and check that all FST types produce the same number of states.
      VLOG(2) << "Composed states: " << output.NumStates();
    }
  }
}

BENCHMARK_TEMPLATE(BM_LookAheadCompose, StdVectorFst);
BENCHMARK_TEMPLATE(BM_LookAheadCompose, StdConstFst);
BENCHMARK_TEMPLATE(BM_LookAheadCompose, StdCompactUnweightedFst);

template <class F>
static void BM_Copy(benchmark::State& state) {
  using Arc = typename F::Arc;
  std::unique_ptr<Fst<Arc>> fst(
      ABSL_DIE_IF_NULL(Fst<Arc>::Read(JoinPathRespectAbsolute(
          std::string("."), absl::GetFlag(FLAGS_input_fst)))));

  std::unique_ptr<F> f(Convert<F>()(*fst));
  for (auto _ : state) {
    std::unique_ptr<F> fp(f->Copy(absl::GetFlag(FLAGS_fst_safe_copy)));
    CHECK(fp != nullptr);
  }
}

BENCHMARK_TEMPLATE(BM_Copy, StdVectorFst);
BENCHMARK_TEMPLATE(BM_Copy, StdConstFst);
BENCHMARK_TEMPLATE(BM_Copy, StdCompactUnweightedFst);

}  // namespace
}  // namespace fst
