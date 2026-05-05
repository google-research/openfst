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

// Utility templates to inspect FST types.

#ifndef OPENFST_LIB_LOOKAHEAD_MATCHER_TRAITS_H_
#define OPENFST_LIB_LOOKAHEAD_MATCHER_TRAITS_H_

#include <cstdint>
#include <type_traits>

#include "openfst/lib/lookahead-matcher.h"
#include "openfst/lib/matcher-fst.h"

namespace fst {

// Extracts template arguments from instances of LabelLookAheadMatcher.
template <typename>
struct LabelLookAheadMatcherTypes;

template <class M, uint32_t F, class S, class D, class LB,
          template <class, class, class, class> class R>
struct LabelLookAheadMatcherTypes<
    LabelLookAheadMatcher<M, F, S, R<typename M::Arc, S, D, LB>>> {
  using Matcher = M;
  using Accumulator = S;
  using Arc = typename Matcher::Arc;
  using Reachable = R<Arc, S, D, LB>;
  enum : uint32_t { kFlags = F };

  // Replaces the accumulator type in the LabelReachable instance with
  // NewAccumulator.
  template <class NewAccumulator>
  using ChangeReachableAccumulator = R<Arc, NewAccumulator, D, LB>;

  // Type of the same LabelLookAheadMatcher but with NewAccumulator as
  // accumulator type.
  template <class NewAccumulator>
  using ChangeAccumulator =
      LabelLookAheadMatcher<Matcher, kFlags, NewAccumulator,
                            ChangeReachableAccumulator<NewAccumulator>>;
};

// Extracts template arguments from instances of MatcherFst.
template <typename>
struct MatcherFstTypes;

template <class F, class M, const char* N, class I, class A>
struct MatcherFstTypes<MatcherFst<F, M, N, I, A>> {
  using Fst = F;
  using Arc = typename Fst::Arc;
  using Matcher = M;
  using MatcherData = typename M::MatcherData;
  using Initializer = I;
  using AddOn = A;

  static const char* name() { return N; }

  // Type of the same MatcherFst but with NewMatcher as accumulator and
  // NewName as name. Requires that Matcher and NewMatcher share the same type
  // of AddOn data.
  template <class NewMatcher, const char* NewName = N>
  using ChangeMatcher = MatcherFst<F, NewMatcher, NewName, Initializer, AddOn>;
};

// Is true for instances of LabelLookAheadMatcher.
template <class T>
struct IsLabelLookAheadMatcher : std::false_type {};

template <class M, uint32_t F, class... Args>
struct IsLabelLookAheadMatcher<LabelLookAheadMatcher<M, F, Args...>>
    : std::true_type {};

// Is true for instances of MatcherFst with LabelLookAheadMatcher.
template <typename T>
struct IsLabelLookAheadMatcherFst : std::false_type {};

template <class F, const char* N, class M, class... Args>
struct IsLabelLookAheadMatcherFst<MatcherFst<F, M, N, Args...>>
    : IsLabelLookAheadMatcher<M> {};

}  // namespace fst

#endif  // OPENFST_LIB_LOOKAHEAD_MATCHER_TRAITS_H_
