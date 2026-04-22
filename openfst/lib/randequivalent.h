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
// Tests if two FSTS are equivalent by checking if random strings from one FST
// are transduced the same by both FSTs.

#ifndef OPENFST_LIB_RANDEQUIVALENT_H_
#define OPENFST_LIB_RANDEQUIVALENT_H_

#include <cstdint>
#include <limits>
#include <optional>
#include <random>

#include "absl/log/log.h"
#include "absl/random/bit_gen_ref.h"
#include "absl/random/random.h"
#include "openfst/lib/arcsort.h"
#include "openfst/lib/compose.h"
#include "openfst/lib/connect.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/project.h"
#include "openfst/lib/properties.h"
#include "openfst/lib/randgen.h"
#include "openfst/lib/shortest-distance.h"
#include "openfst/lib/symbol-table.h"
#include "openfst/lib/util.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/lib/weight.h"

namespace fst {

// Test if two FSTs are stochastically equivalent by randomly generating
// random paths through the FSTs.
//
// For each randomly generated path, the algorithm computes for each
// of the two FSTs the sum of the weights of all the successful paths
// sharing the same input and output labels as the considered randomly
// generated path and checks that these two values are within a user-specified
// delta. Returns optional error value (when FLAGS_error_fatal = false).
template <class Arc, class ArcSelector>
bool RandEquivalent(const Fst<Arc>& fst1, const Fst<Arc>& fst2, int32_t npath,
                    const RandGenOptions<ArcSelector>& opts,
                    absl::BitGenRef bit_gen, float delta = kDelta,
                    bool* error = nullptr) {
  using Weight = typename Arc::Weight;
  if (error) *error = false;
  // Checks that the symbol table are compatible.
  if (!CompatSymbols(fst1.InputSymbols(), fst2.InputSymbols()) ||
      !CompatSymbols(fst1.OutputSymbols(), fst2.OutputSymbols())) {
    FSTERROR() << "RandEquivalent: Input/output symbol tables of 1st "
               << "argument do not match input/output symbol tables of 2nd "
               << "argument";
    if (error) *error = true;
    return false;
  }
  static const ILabelCompare<Arc> icomp;
  static const OLabelCompare<Arc> ocomp;
  VectorFst<Arc> sfst1(fst1);
  VectorFst<Arc> sfst2(fst2);
  Connect(&sfst1);
  Connect(&sfst2);
  ArcSort(&sfst1, icomp);
  ArcSort(&sfst2, icomp);
  bool result = true;
  for (int32_t n = 0; n < npath; ++n) {
    VectorFst<Arc> path;
    const auto& fst = absl::Bernoulli(bit_gen, .5) ? sfst1 : sfst2;
    RandGen(fst, &path, opts, bit_gen);
    VectorFst<Arc> ipath(path);
    VectorFst<Arc> opath(path);
    Project(&ipath, ProjectType::INPUT);
    Project(&opath, ProjectType::OUTPUT);
    VectorFst<Arc> cfst1, pfst1;
    Compose(ipath, sfst1, &cfst1);
    ArcSort(&cfst1, ocomp);
    Compose(cfst1, opath, &pfst1);
    // Gives up if there are epsilon cycles in a non-idempotent semiring.
    if (!IsIdempotent<Weight>::value && pfst1.Properties(kCyclic, true)) {
      continue;
    }
    const auto sum1 = ShortestDistance(pfst1);
    VectorFst<Arc> cfst2;
    Compose(ipath, sfst2, &cfst2);
    ArcSort(&cfst2, ocomp);
    VectorFst<Arc> pfst2;
    Compose(cfst2, opath, &pfst2);
    // Gives up if there are epsilon cycles in a non-idempotent semiring.
    if (!IsIdempotent<Weight>::value && pfst2.Properties(kCyclic, true)) {
      continue;
    }
    const auto sum2 = ShortestDistance(pfst2);
    if (!ApproxEqual(sum1, sum2, delta)) {
      VLOG(1) << "Sum1 = " << sum1;
      VLOG(1) << "Sum2 = " << sum2;
      result = false;
      break;
    }
  }
  if (fst1.Properties(kError, false) || fst2.Properties(kError, false)) {
    if (error) *error = true;
    return false;
  }
  return result;
}

template <class Arc, class ArcSelector>
[[deprecated("Use overload with absl::BitGenRef.")]]
bool RandEquivalent(const Fst<Arc>& fst1, const Fst<Arc>& fst2, int32_t npath,
                    const RandGenOptions<ArcSelector>& opts,
                    float delta = kDelta,
                    std::optional<uint64_t> seed = std::nullopt,
                    bool* error = nullptr) {
  if (seed.has_value()) {
    std::mt19937_64 bit_gen(*seed);
    return RandEquivalent(fst1, fst2, npath, opts, bit_gen, delta, error);
  } else {
    absl::BitGen bit_gen;
    return RandEquivalent(fst1, fst2, npath, opts, bit_gen, delta, error);
  }
}

// Tests if two FSTs are equivalent by randomly generating a nnpath paths
// (no longer than the path_length) using a user-specified seed, optionally
// indicating an error setting an optional error argument to true.
template <class Arc>
bool RandEquivalent(const Fst<Arc>& fst1, const Fst<Arc>& fst2, int32_t npath,
                    absl::BitGenRef bit_gen, float delta = kDelta,
                    int32_t max_length = std::numeric_limits<int32_t>::max(),
                    bool* error = nullptr) {
  const UniformArcSelector<Arc> uniform_selector;
  const RandGenOptions<UniformArcSelector<Arc>> opts(uniform_selector,
                                                     max_length);
  return RandEquivalent(fst1, fst2, npath, opts, bit_gen, delta, error);
}

template <class Arc>
[[deprecated("Use overload with absl::BitGenRef.")]]
bool RandEquivalent(const Fst<Arc>& fst1, const Fst<Arc>& fst2, int32_t npath,
                    float delta = kDelta,
                    std::optional<uint64_t> seed = std::nullopt,
                    int32_t max_length = std::numeric_limits<int32_t>::max(),
                    bool* error = nullptr) {
  if (seed.has_value()) {
    std::mt19937_64 bit_gen(*seed);
    return RandEquivalent(fst1, fst2, npath, bit_gen, delta, max_length, error);
  } else {
    absl::BitGen bit_gen;
    return RandEquivalent(fst1, fst2, npath, bit_gen, delta, max_length, error);
  }
}

}  // namespace fst

#endif  // OPENFST_LIB_RANDEQUIVALENT_H_
