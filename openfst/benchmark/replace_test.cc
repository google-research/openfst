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
// Benchmark for FST replacement.

#include "openfst/lib/replace.h"

#include <memory>
#include <utility>
#include <vector>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "absl/log/die_if_null.h"
#include "absl/strings/string_view.h"
#include "benchmark/benchmark.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/vector-fst.h"

constexpr absl::string_view kTestDataDir =
    "openfst/test/testdata/replace";

namespace fst {
namespace {

using Arc = StdArc;
using FstType = StdFst;
using FstTuple = std::pair<Arc::Label, const Fst<Arc>*>;

void BM_Replace(benchmark::State& state) {
  std::unique_ptr<const FstType> g1(ABSL_DIE_IF_NULL(FstType::Read(
      JoinPath(std::string("."), kTestDataDir, "g1.fst"))));
  std::unique_ptr<const FstType> g2(ABSL_DIE_IF_NULL(FstType::Read(
      JoinPath(std::string("."), kTestDataDir, "g2.fst"))));
  std::unique_ptr<const FstType> g3(ABSL_DIE_IF_NULL(FstType::Read(
      JoinPath(std::string("."), kTestDataDir, "g3.fst"))));
  std::unique_ptr<const FstType> g4(ABSL_DIE_IF_NULL(FstType::Read(
      JoinPath(std::string("."), kTestDataDir, "g4.fst"))));

  std::vector<FstTuple> input_fsts;
  input_fsts.push_back(FstTuple(1, g1.get()));
  input_fsts.push_back(FstTuple(2, g2.get()));
  input_fsts.push_back(FstTuple(3, g3.get()));
  input_fsts.push_back(FstTuple(4, g4.get()));

  for (auto _ : state) {
    VectorFst<Arc> replaced;
    Replace(input_fsts, &replaced, 1);
  }
}

BENCHMARK(BM_Replace);

}  // namespace
}  // namespace fst
