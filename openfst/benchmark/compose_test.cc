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
// Benchmark for FST composition.

#include "openfst/lib/compose.h"

#include <functional>
#include <memory>
#include <string>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "absl/flags/flag.h"
#include "absl/log/check.h"
#include "absl/log/die_if_null.h"
#include "absl/log/log.h"
#include "absl/strings/string_view.h"
#include "benchmark/benchmark.h"
#include "openfst/extensions/ngram/ngram-fst.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/bi-table.h"
#include "openfst/lib/cache.h"
#include "openfst/lib/compact-fst.h"
#include "openfst/lib/compose-filter.h"
#include "openfst/lib/const-fst.h"
#include "openfst/lib/filter-state.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/lookahead-matcher.h"
#include "openfst/lib/matcher-fst.h"
#include "openfst/lib/matcher.h"
#include "openfst/lib/project.h"
#include "openfst/lib/properties.h"
#include "openfst/lib/state-table.h"
#include "openfst/lib/vector-fst.h"

// Test data from www.openfst.org/twiki/bin/view/FST/FstExamples

ABSL_FLAG(std::string, fst1,
          "openfst/benchmark/testdata/lexicon.fst",
          "1st fst for compose");

ABSL_FLAG(std::string, fst2,
          "openfst/benchmark/testdata/wotw.lm.fst",
          "2nd fst for compose (an n-gram fst)");

ABSL_FLAG(std::string, lookahead_fst,
          "openfst/benchmark/testdata/lexicon_opt.fst",
          "1st fst for lookahead compose");

// From randgen of default fst2 below.
ABSL_FLAG(std::string, string_fst,
          "openfst/benchmark/testdata/string.fst",
          "1st fst for string compose");

namespace fst {
namespace {

using StdNGramFst = NGramFst<StdArc>;

// Loads fst of type F from file and computes its properties.
template <class F>
std::unique_ptr<const F> LoadFst(absl::string_view filename) {
  std::unique_ptr<Fst<typename F::Arc>> raw_fst(
      Fst<typename F::Arc>::Read(filename));
  auto fst = std::make_unique<F>(*ABSL_DIE_IF_NULL(raw_fst.get()));
  fst->Properties(kFstProperties, true);  // compute properties
  return fst;
}

// Returns the (static) instance of ConstFst<Arc> read from FLAGS_fst1.
template <class Arc>
const ConstFst<Arc>& GetFst1() {
  static const auto fst = LoadFst<ConstFst<Arc>>(JoinPathRespectAbsolute(
      std::string("."), absl::GetFlag(FLAGS_fst1)));
  return *fst;
}

// Returns the (static) instance of Fst<Arc> read from FLAGS_fst2.
template <class Arc>
const Fst<Arc>& GetFst2() {
  static const std::unique_ptr<const Fst<Arc>> fst(
      Fst<Arc>::Read(JoinPathRespectAbsolute(std::string("."),
                                                   absl::GetFlag(FLAGS_fst2))));
  CHECK(fst != nullptr) << absl::GetFlag(FLAGS_fst2);
  return *fst;
}

// Returns the (static) instance of StdOLabelLookAheadFstread from
// FLAGS_lookahead_fst.
const StdOLabelLookAheadFst& GetLookAheadFst() {
  static const auto fst =
      LoadFst<StdOLabelLookAheadFst>(JoinPathRespectAbsolute(
          std::string("."), absl::GetFlag(FLAGS_lookahead_fst)));
  return *fst;
}

// Tests composition (by default) between an FST and an n-gram LM.
template <typename F>
void BM_Compose(benchmark::State& bm_state) {
  using Arc = typename F::Arc;

  const ConstFst<Arc>& f1 = GetFst1<Arc>();
  F f2(GetFst2<Arc>());
  f2.Properties(kFstProperties, true);

  ComposeOptions opts(false);
  for (auto _ : bm_state) {
    VectorFst<Arc> output;
    Compose(f1, f2, &output, opts);
  }
}

BENCHMARK(BM_Compose<StdVectorFst>);
BENCHMARK(BM_Compose<StdConstFst>);
BENCHMARK(BM_Compose<StdCompactAcceptorFst>);
BENCHMARK(BM_Compose<StdNGramFst>);

template <typename A, HSType HS,
          typename ST =
              DefaultComposeStateTuple<typename A::StateId, CharFilterState>>
class TestCompactHashStateTable
    : public CompactHashBiTable<typename A::StateId, ST, ComposeHash<ST>,
                                std::equal_to<ST>, HS> {
 public:
  using StateTuple = ST;
  using Super = CompactHashBiTable<typename A::StateId, StateTuple,
                                   ComposeHash<StateTuple>,
                                   std::equal_to<StateTuple>, HS>;
  typename A::StateId FindState(const StateTuple& tuple) {
    return Super::FindId(tuple);
  }
  const StateTuple& Tuple(typename A::StateId s) const {
    return Super::FindEntry(s);
  }
};

template <typename A, HSType HS>
class TestCompactComposeStateTable : public TestCompactHashStateTable<A, HS> {
 public:
  using typename TestCompactHashStateTable<A, HS>::StateTuple;
  TestCompactComposeStateTable(const Fst<A>& fst1, const Fst<A>& fst2) {}
  bool Error() const { return false; }
};

// Tests on-the-fly composition (by default) between an FST and an n-gram LM,
// under a variety of different bi-table implementations.
template <typename F, HSType HS>
void BM_ComposeFst(benchmark::State& bm_state) {
  using Arc = typename F::Arc;

  const ConstFst<Arc>& f1 = GetFst1<Arc>();
  F f2(GetFst2<Arc>());
  f2.Properties(kFstProperties, true);

  for (auto _ : bm_state) {
    const CacheOptions cache_options;
    const ComposeFstImplOptions<
        Matcher<ConstFst<Arc>>, Matcher<F>,
        SequenceComposeFilter<Matcher<ConstFst<Arc>>, Matcher<F>>,
        TestCompactComposeStateTable<Arc, HS>>
        options(cache_options);
    ComposeFst<Arc> compose(f1, f2, options);
    CountStates(compose);
  }
}

BENCHMARK(BM_ComposeFst<StdVectorFst, HS_STL>);
BENCHMARK(BM_ComposeFst<StdConstFst, HS_STL>);
BENCHMARK(BM_ComposeFst<StdCompactAcceptorFst, HS_STL>);
BENCHMARK(BM_ComposeFst<StdNGramFst, HS_STL>);
BENCHMARK(BM_ComposeFst<StdVectorFst, HS_FLAT>);
BENCHMARK(BM_ComposeFst<StdConstFst, HS_FLAT>);
BENCHMARK(BM_ComposeFst<StdCompactAcceptorFst, HS_FLAT>);
BENCHMARK(BM_ComposeFst<StdNGramFst, HS_FLAT>);

template <typename A,
          typename StateTuple =
              DefaultComposeStateTuple<typename A::StateId, CharFilterState>>
class TestComposeStateTable
    : public HashStateTable<StateTuple, ComposeHash<StateTuple>> {
 public:
  TestComposeStateTable(const Fst<A>& fst1, const Fst<A>& fst2) {}
  bool Error() const { return false; }
};

template <typename F>
void BM_ComposeFstHashBiTable(benchmark::State& bm_state) {
  using Arc = typename F::Arc;

  const ConstFst<Arc>& f1 = GetFst1<Arc>();
  F f2(GetFst2<Arc>());
  f2.Properties(kFstProperties, true);

  for (auto _ : bm_state) {
    CacheOptions cache_options;
    ComposeFstImplOptions<
        Matcher<ConstFst<Arc>>, Matcher<F>,
        SequenceComposeFilter<Matcher<ConstFst<Arc>>, Matcher<F>>,
        TestComposeStateTable<Arc>>
        options(cache_options);
    ComposeFst<Arc> compose(f1, f2, options);
    CountStates(compose);
  }
}

BENCHMARK(BM_ComposeFstHashBiTable<StdVectorFst>);
BENCHMARK(BM_ComposeFstHashBiTable<StdConstFst>);
BENCHMARK(BM_ComposeFstHashBiTable<StdCompactAcceptorFst>);
BENCHMARK(BM_ComposeFstHashBiTable<StdNGramFst>);

// Tests composition (by default) between a string and an n-gram LM.
template <typename F>
void BM_StringCompose(benchmark::State& bm_state) {
  using Arc = typename F::Arc;

  std::unique_ptr<Fst<Arc>> f1_ptr(Fst<Arc>::Read(JoinPathRespectAbsolute(
      std::string("."), absl::GetFlag(FLAGS_string_fst))));
  ConstFst<Arc> f1(*ABSL_DIE_IF_NULL(f1_ptr.get()));
  F f2(GetFst2<Arc>());
  f1.Properties(kFstProperties, true);
  f2.Properties(kFstProperties, true);

  VectorFst<Arc> output;
  ComposeOptions opts(false);
  for (auto _ : bm_state) {
    Compose(f1, f2, &output, opts);
  }
}

BENCHMARK(BM_StringCompose<StdVectorFst>);
BENCHMARK(BM_StringCompose<StdConstFst>);
BENCHMARK(BM_StringCompose<StdCompactAcceptorFst>);
BENCHMARK(BM_StringCompose<StdNGramFst>);

// Tests lookhead composition (by default) between an FST and an n-gram LM.
template <typename F>
void BM_LookAheadCompose(benchmark::State& bm_state) {
  using Arc = StdArc;

  const StdOLabelLookAheadFst& f1 = GetLookAheadFst();
  VectorFst<Arc> fr(GetFst2<Arc>());

  LabelLookAheadRelabeler<Arc>::Relabel(&fr, f1, true);
  Project(&fr, ProjectType::INPUT);
  F f2(fr);
  f2.Properties(kFstProperties, true);

  VectorFst<Arc> output;
  ComposeOptions opts(false);
  for (auto _ : bm_state) {
    Compose(f1, f2, &output, opts);
  }
}

BENCHMARK(BM_LookAheadCompose<StdVectorFst>);
BENCHMARK(BM_LookAheadCompose<StdConstFst>);
BENCHMARK(BM_LookAheadCompose<StdCompactAcceptorFst>);
BENCHMARK(BM_LookAheadCompose<StdNGramFst>);

}  // namespace
}  // namespace fst
