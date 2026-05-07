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

#ifndef OPENFST_SCRIPT_RANDEQUIVALENT_H_
#define OPENFST_SCRIPT_RANDEQUIVALENT_H_

#include <cstdint>
#include <optional>
#include <random>
#include <tuple>
#include <variant>

#include "absl/random/bit_gen_ref.h"
#include "absl/random/random.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/randequivalent.h"
#include "openfst/lib/randgen.h"
#include "openfst/lib/weight.h"
#include "openfst/script/arg-packs.h"
#include "openfst/script/fst-class.h"
#include "openfst/script/script-impl.h"

namespace fst {
namespace script {

using FstRandEquivalentInnerArgs =
    std::tuple<const FstClass&, const FstClass&, int32_t,
               const RandGenOptions<RandArcSelection>&, float,
               std::optional<uint64_t>>;

using FstRandEquivalentArgs = WithReturnValue<bool, FstRandEquivalentInnerArgs>;

template <class Arc>
void RandEquivalent(FstRandEquivalentArgs* args) {
  const Fst<Arc>& fst1 = *std::get<0>(args->args).GetFst<Arc>();
  const Fst<Arc>& fst2 = *std::get<1>(args->args).GetFst<Arc>();
  const int32_t npath = std::get<2>(args->args);
  const auto& opts = std::get<3>(args->args);
  const float delta = std::get<4>(args->args);

  const std::optional<uint64_t> seed = std::get<5>(args->args);
  std::variant<std::monostate, absl::BitGen, std::mt19937_64> bit_gen;
  if (seed.has_value()) {
    bit_gen.emplace<std::mt19937_64>(*seed);
  } else {
    bit_gen.emplace<absl::BitGen>();
  }
  absl::BitGenRef bit_gen_ref =
      seed.has_value() ? absl::BitGenRef(std::get<std::mt19937_64>(bit_gen))
                       : absl::BitGenRef(std::get<absl::BitGen>(bit_gen));

  switch (opts.selector) {
    case RandArcSelection::UNIFORM: {
      const UniformArcSelector<Arc> selector;
      const RandGenOptions<UniformArcSelector<Arc>> ropts(selector,
                                                          opts.max_length);
      args->retval =
          RandEquivalent(fst1, fst2, npath, ropts, bit_gen_ref, delta);
      return;
    }
    case RandArcSelection::FAST_LOG_PROB: {
      const FastLogProbArcSelector<Arc> selector;
      const RandGenOptions<FastLogProbArcSelector<Arc>> ropts(selector,
                                                              opts.max_length);
      args->retval =
          RandEquivalent(fst1, fst2, npath, ropts, bit_gen_ref, delta);
      return;
    }
    case RandArcSelection::LOG_PROB: {
      const LogProbArcSelector<Arc> selector;
      const RandGenOptions<LogProbArcSelector<Arc>> ropts(selector,
                                                          opts.max_length);
      args->retval =
          RandEquivalent(fst1, fst2, npath, ropts, bit_gen_ref, delta);
      return;
    }
  }
}

bool RandEquivalent(const FstClass& fst1, const FstClass& fst2, int32_t npath,
                    const RandGenOptions<RandArcSelection>& opts, float delta,
                    std::optional<uint64_t> seed);

bool RandEquivalent(
    const FstClass& fst1, const FstClass& fst2, int32_t npath = 1,
    const RandGenOptions<RandArcSelection>& opts =
        RandGenOptions<RandArcSelection>(RandArcSelection::UNIFORM),
    float delta = kDelta);

}  // namespace script
}  // namespace fst

#endif  // OPENFST_SCRIPT_RANDEQUIVALENT_H_
