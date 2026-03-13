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

// Main file for running OpenFst tests (possibly with benchmarks).

#include <algorithm>
#include <cstring>

#include "gtest/gtest.h"
#include "benchmark/benchmark.h"

int main(int argc, char* argv[]) {
  // Disable ASLR for benchmarks, but not tests.  `MaybeReenterWithoutASLR`
  // calls `execve`, so do it right away.
  const bool has_benchmark_filter =
      std::any_of(&argv[1], &argv[argc], [](const char* arg) {
        // `string_view::starts_with()` is C++20, so use `strncmp`.
        const char prefix[] = "--benchmark_filter";
        return strncmp(arg, prefix, sizeof(prefix) - 1) == 0;
      });
  if (has_benchmark_filter) {
    benchmark::MaybeReenterWithoutASLR(argc, argv);
  }

  benchmark::Initialize(&argc, argv);
  ::testing::InitGoogleTest(&argc, argv);

  if (!benchmark::GetBenchmarkFilter().empty()) {
    benchmark::RunSpecifiedBenchmarks();
    return 0;
  }
  return RUN_ALL_TESTS();
}
