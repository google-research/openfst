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

#include "benchmark/benchmark.h"

#include <cstdint>
#include <string>
#include <vector>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "absl/flags/flag.h"
#include "absl/log/check.h"
#include "absl/log/log.h"
#include "openfst/extensions/ngram/ngram-fst.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/cache.h"
#include "openfst/lib/compose.h"
#include "openfst/lib/float-weight.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/matcher.h"
#include "openfst/lib/shortest-distance.h"
#include "openfst/lib/symbol-table.h"
#include "openfst/lib/vector-fst.h"

ABSL_FLAG(std::string, fst_file, "", "Fst file to use for tests");
ABSL_FLAG(int32_t, num_tests, 10, "Number of iterations to peform tests");
ABSL_FLAG(int32_t, test_length, 100, "Maximum size strings to test");
ABSL_FLAG(int32_t, max_symbol, 200, "Maximum word id in the test fst");
ABSL_FLAG(int32_t, min_symbol, 3, "Maximum word id in the test fst");

namespace fst {
namespace {

std::string Testfile() {
  if (absl::GetFlag(FLAGS_fst_file).empty()) {
    return JoinPath(
        std::string("."),
        "openfst/extensions/ngram/testdata/earnest.mod");
  }
  return absl::GetFlag(FLAGS_fst_file);
}

template <class Arc>
std::vector<VectorFst<Arc>> GenerateSequences(bool epsilons) {
  std::vector<VectorFst<Arc>> seqs;
  for (int j = 0; j < absl::GetFlag(FLAGS_num_tests); j++) {
    VectorFst<Arc> ltor;
    typename Arc::StateId state = ltor.AddState();
    ltor.SetStart(state);
    typename Arc::Label label;
    for (int i = 0; i < absl::GetFlag(FLAGS_test_length); ++i) {
      if (epsilons) {
        label = ((i + j) % (absl::GetFlag(FLAGS_max_symbol) -
                            absl::GetFlag(FLAGS_min_symbol) + 1) +
                 absl::GetFlag(FLAGS_min_symbol)) %
                absl::GetFlag(FLAGS_max_symbol);
      } else {
        label = (i + j) % (absl::GetFlag(FLAGS_max_symbol) -
                           absl::GetFlag(FLAGS_min_symbol)) +
                absl::GetFlag(FLAGS_min_symbol);
      }
      typename Arc::StateId next_state = ltor.AddState();
      ltor.AddArc(state, Arc(label, label, Arc::Weight::One(), next_state));
      state = next_state;
    }
    ltor.SetFinal(state, Arc::Weight::One());
    seqs.push_back(ltor);
  }
  return seqs;
}

template <class F, class M>
typename F::Arc::Weight ScoreFst(const F& fst, const Fst<typename F::Arc>& seq,
                                 M* matcher) {
  typedef typename F::Arc Arc;
  CacheOptions cache_opts;
  cache_opts.gc_limit = 0;
  ComposeFstImplOptions<Matcher<Fst<Arc>>, M> copts(
      cache_opts, new Matcher<Fst<Arc>>(seq, MATCH_OUTPUT), matcher->Copy());
  ComposeFst<Arc> cfst(seq, fst, copts);
  return ShortestDistance(cfst);
}

NGramFst<StdArc>* ngram_fst = nullptr;
Fst<StdArc>* bm_fst = nullptr;
std::vector<VectorFst<StdArc>>* bm_seqs;

void FstSpeed(benchmark::State& state) {
  StdArc::Weight total;
  Matcher<StdFst> m(*bm_fst, MATCH_INPUT);
  for (auto s : state) {
    for (int s = 0; s < bm_seqs->size(); ++s) {
      total = Plus(
          total, ScoreFst<StdFst, Matcher<StdFst>>(*bm_fst, (*bm_seqs)[s], &m));
    }
    VLOG(1) << "Total score = " << total;
  }
}

void FstPhiSpeed(benchmark::State& state) {
  StdArc::Weight total;
  typedef PhiMatcher<Matcher<StdFst>> PM;
  PM m(*bm_fst, MATCH_INPUT, 0);
  for (auto i : state) {
    for (int s = 0; s < bm_seqs->size(); ++s) {
      total = Plus(total, ScoreFst<StdFst, PM>(*bm_fst, (*bm_seqs)[s], &m));
    }
    VLOG(1) << "Total score = " << total;
  }
}

void NGramFstSpeed(benchmark::State& state) { FstSpeed(state); }

void NGramPhiFstSpeed(benchmark::State& state) {
  StdArc::Weight total;
  typedef PhiMatcher<Matcher<NGramFst<StdArc>>> PM;
  PM m(*ngram_fst, MATCH_INPUT, 0);
  for (auto s : state) {
    for (int s = 0; s < bm_seqs->size(); ++s) {
      total = Plus(
          total, ScoreFst<NGramFst<StdArc>, PM>(*ngram_fst, (*bm_seqs)[s], &m));
    }
    VLOG(1) << "Total score = " << total;
  }
}

void FstSpeedSetup(const benchmark::State& state) {
  absl::SetFlag(&FLAGS_fst_compat_symbols, false);
  CHECK_EQ(state.threads(), 1)
      << "Only single threaded benchmarks are supported";
  bm_fst = StdFst::Read(Testfile());
  bm_seqs = new std::vector<StdVectorFst>();
  *bm_seqs = GenerateSequences<StdArc>(false);
}

void NGramFstSpeedSetup(const benchmark::State& state) {
  FstSpeedSetup(state);
  ngram_fst = new NGramFst<StdArc>(*bm_fst);
  delete bm_fst;
  bm_fst = ngram_fst;
}

void SpeedTeardown(const benchmark::State& state) {
  delete bm_fst;
  delete bm_seqs;
}

BENCHMARK(FstSpeed)->Setup(FstSpeedSetup)->Teardown(SpeedTeardown);

BENCHMARK(FstPhiSpeed)->Setup(FstSpeedSetup)->Teardown(SpeedTeardown);

BENCHMARK(NGramFstSpeed)->Setup(NGramFstSpeedSetup)->Teardown(SpeedTeardown);

BENCHMARK(NGramPhiFstSpeed)->Setup(NGramFstSpeedSetup)->Teardown(SpeedTeardown);

}  // namespace
}  // namespace fst
