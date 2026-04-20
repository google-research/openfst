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
// Benchmark for various FST matchers.

#include "openfst/lib/matcher.h"

#include <cstddef>
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
#include "openfst/lib/vector-fst.h"

ABSL_FLAG(std::string, input_fst,
          "openfst/benchmark/testdata/wotw.lm.fst",
          "FST to test.");

namespace fst {
namespace {

constexpr StdArc::Label kTestInputLabel = 7;

template <typename FST>
std::unique_ptr<const FST> ReadFst() {
  return std::unique_ptr<const FST>(
      ABSL_DIE_IF_NULL(FST::Read(JoinPathRespectAbsolute(
          std::string("."), absl::GetFlag(FLAGS_input_fst)))));
}

template <typename MatcherType>
void BM_InputMatcher(benchmark::State& state) {
  auto ifst = ReadFst<StdVectorFst>();
  StdVectorFst ofst(*ifst);
  ofst.Properties(kFstProperties, true);
  MatcherType matcher(ofst, MATCH_INPUT);
  size_t nmatch = 0, nstates = 0;
  for (auto _ : state) {
    for (StateIterator<StdVectorFst> siter(ofst); !siter.Done(); siter.Next()) {
      auto s = siter.Value();
      matcher.SetState(s);
      nmatch += matcher.Find(kTestInputLabel);
      ++nstates;
    }
  }
  CHECK_GT(nmatch, 0);
  state.SetItemsProcessed(nstates);
}

// Benchmark for input SortedMatcher (default Matcher for VectorFst).
void BM_SortedInputMatcher(benchmark::State& state) {
  BM_InputMatcher<Matcher<StdVectorFst>>(state);
}
BENCHMARK(BM_SortedInputMatcher);

// Benchmark for input HashMatcher.
void BM_HashInputMatcher(benchmark::State& state) {
  BM_InputMatcher<HashMatcher<StdVectorFst>>(state);
}
BENCHMARK(BM_HashInputMatcher);

}  // namespace
}  // namespace fst
