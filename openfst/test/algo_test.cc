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
// Regression test for various FST algorithms.

#include "openfst/test/algo_test.h"

#include <cstdint>
#include <random>

#include "gtest/gtest.h"
#include "absl/flags/flag.h"
#include "absl/log/flags.h"
#include "absl/log/log.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/cache.h"
#include "openfst/lib/fst-decl.h"
#include "openfst/lib/test-properties.h"

ABSL_FLAG(uint64_t, seed, 403, "random seed");
ABSL_FLAG(int32_t, repeat, 25, "number of test repetitions");

namespace fst {
namespace {

// These macros determine which semirings are tested; they are controlled by the
// `defines` attributes of the associated build rules.  We can try to use one
// test with all these as `ArcTypes`, but that may blow up the build time.
#if defined(TEST_TROPICAL)
using Arc = StdArc;
#elif defined(TEST_LOG)
using Arc = LogArc;
#elif defined(TEST_MINMAX)
using Arc = MinMaxArc;
#elif defined(TEST_LEFT_STRING)
using Arc = StringArc<STRING_LEFT>;
#elif defined(TEST_RIGHT_STRING)
using Arc = StringArc<STRING_RIGHT>;
#elif defined(TEST_GALLIC)
using Arc = GallicArc<StdArc>;
#elif defined(TEST_LEXICOGRAPHIC)
using Arc = LexicographicArc<TropicalWeight, TropicalWeight>;
#elif defined(TEST_POWER)
using Arc = ArcTpl<PowerWeight<TropicalWeight, 3> >;
#else
#error "Must have one of the TEST_* macros defined."
#endif

using ArcTypes = ::testing::Types<Arc>;
// The extra comma is for C++17 compat.  We need to include an empty "..."
// arg instead of omitting it.  It can be removed when we require C++20.
INSTANTIATE_TYPED_TEST_SUITE_P(MyRational, RationalTest, ArcTypes, );
INSTANTIATE_TYPED_TEST_SUITE_P(MyMap, MapTest, ArcTypes, );
INSTANTIATE_TYPED_TEST_SUITE_P(MyCompose, ComposeTest, ArcTypes, );
INSTANTIATE_TYPED_TEST_SUITE_P(MySort, SortTest, ArcTypes, );
INSTANTIATE_TYPED_TEST_SUITE_P(MyOptimize, OptimizeTest, ArcTypes, );
INSTANTIATE_TYPED_TEST_SUITE_P(MySearch, SearchTest, ArcTypes, );
using UnweightedArcTypes = ::testing::Types<fst::StdArc>;
INSTANTIATE_TYPED_TEST_SUITE_P(MyUnweighted, UnweightedTest,
                               UnweightedArcTypes, );

}  // namespace
}  // namespace fst

int main(int argc, char** argv) {
  absl::SetFlag(&FLAGS_fst_verify_properties, true);
  ::testing::InitGoogleTest(&argc, argv);

  static const int kCacheGcLimit = 20;

  LOG(INFO) << "Seed = " << absl::GetFlag(FLAGS_seed);

  std::mt19937_64 rand(absl::GetFlag(FLAGS_seed));

  absl::SetFlag(&FLAGS_fst_default_cache_gc,
                std::bernoulli_distribution(.5)(rand));
  absl::SetFlag(&FLAGS_fst_default_cache_gc_limit,
                std::uniform_int_distribution<>(0, kCacheGcLimit)(rand));
  VLOG(1) << "default_cache_gc:" << absl::GetFlag(FLAGS_fst_default_cache_gc);
  VLOG(1) << "default_cache_gc_limit:"
          << absl::GetFlag(FLAGS_fst_default_cache_gc_limit);

  return RUN_ALL_TESTS();
}
