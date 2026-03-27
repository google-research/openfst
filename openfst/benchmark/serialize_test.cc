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

#include <memory>
#include <sstream>
#include <string>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "absl/flags/flag.h"
#include "absl/log/die_if_null.h"
#include "benchmark/benchmark.h"
#include "openfst/lib/const-fst.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/symbol-table.h"
#include "openfst/lib/vector-fst.h"

// This file was created using:
// bazel-bin/nlp/grm2/ngram/ngramshrink
//   --theta=0.0001
//   nlp/grm2/ngram/testdata/earnest.mod
//   openfst/benchmark/testdata/compose.fst
ABSL_FLAG(std::string, input_fst,
          "openfst/benchmark/testdata/serialize.fst",
          "fst to compose with self for testing");

namespace fst {
namespace {

static void SerializeVectorFst(benchmark::State& state) {
  std::unique_ptr<const StdFst> fst(
      ABSL_DIE_IF_NULL(StdFst::Read(JoinPathRespectAbsolute(
          std::string("."), absl::GetFlag(FLAGS_input_fst)))));
  const FstWriteOptions opts;
  for (auto _ : state) {
    std::ostringstream str;
    StdVectorFst::WriteFst(*fst, str, opts);
  }
}
BENCHMARK(SerializeVectorFst);

static void SerializeConstFst(benchmark::State& state) {
  std::unique_ptr<const StdFst> fst(
      ABSL_DIE_IF_NULL(StdFst::Read(JoinPathRespectAbsolute(
          std::string("."), absl::GetFlag(FLAGS_input_fst)))));
  const FstWriteOptions opts;
  for (auto _ : state) {
    std::ostringstream str;
    StdConstFst::WriteFst(*fst, str, opts);
  }
}
BENCHMARK(SerializeConstFst);

static void SerializeSymbolTable(benchmark::State& state) {
  std::unique_ptr<const StdFst> fst(
      ABSL_DIE_IF_NULL(StdFst::Read(JoinPathRespectAbsolute(
          std::string("."), absl::GetFlag(FLAGS_input_fst)))));
  for (auto _ : state) {
    std::ostringstream str;
    fst->InputSymbols()->WriteText(str);
  }
}
BENCHMARK(SerializeSymbolTable);

}  // namespace
}  // namespace fst
