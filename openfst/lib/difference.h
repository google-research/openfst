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
// Class to compute the difference between two FSAs.

#ifndef OPENFST_LIB_DIFFERENCE_H_
#define OPENFST_LIB_DIFFERENCE_H_

#include <memory>

#include "openfst/lib/arc.h"
#include "openfst/lib/cache.h"
#include "openfst/lib/complement.h"
#include "openfst/lib/compose-filter.h"
#include "openfst/lib/compose.h"
#include "openfst/lib/connect.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/matcher.h"
#include "openfst/lib/mutable-fst.h"
#include "openfst/lib/properties.h"
#include "openfst/lib/state-table.h"
#include "openfst/lib/util.h"

namespace fst {

template <class Arc, class M = Matcher<Fst<Arc>>,
          class Filter = SequenceComposeFilter<M>,
          class StateTable =
              GenericComposeStateTable<Arc, typename Filter::FilterState>>
struct DifferenceFstOptions
    : public ComposeFstOptions<Arc, M, Filter, StateTable> {
  explicit DifferenceFstOptions(const CacheOptions& opts = CacheOptions(),
                                M* matcher1 = nullptr, M* matcher2 = nullptr,
                                Filter* filter = nullptr,
                                StateTable* state_table = nullptr)
      : ComposeFstOptions<Arc, M, Filter, StateTable>(opts, matcher1, matcher2,
                                                      filter, state_table) {}
};

// Computes the difference between two FSAs. This version is a delayed FST.
// Only strings that are in the first automaton but not in second are retained
// in the result.
//
// The first argument must be an acceptor; the second argument must be an
// unweighted, epsilon-free, deterministic acceptor. One of the arguments must
// be label-sorted.
//
// Complexity: same as ComposeFst.
//
// Caveats: same as ComposeFst.
template <class A>
class DifferenceFst : public ComposeFst<A> {
  using Base = ComposeFst<A>;

 public:
  using Arc = A;
  using Weight = typename Arc::Weight;
  using StateId = typename Arc::StateId;
  using typename Base::Impl;

  using Base::CreateBase1;

  // A - B = A ^ B'.
  DifferenceFst(const Fst<Arc>& fst1, const Fst<Arc>& fst2,
                const CacheOptions& opts = CacheOptions())
      : Base(CreateDifferenceImplWithCacheOpts(fst1, fst2, opts)) {
    if (!fst1.Properties(kAcceptor, true)) {
      FSTERROR() << "DifferenceFst: 1st argument not an acceptor";
      GetImpl()->SetProperties(kError, kError);
    }
  }

  template <class Matcher, class Filter, class StateTable>
  DifferenceFst(
      const Fst<Arc>& fst1, const Fst<Arc>& fst2,
      const DifferenceFstOptions<Arc, Matcher, Filter, StateTable>& opts)
      : Base(CreateDifferenceImplWithDifferenceOpts(fst1, fst2, opts)) {
    if (!fst1.Properties(kAcceptor, true)) {
      FSTERROR() << "DifferenceFst: 1st argument not an acceptor";
      GetImpl()->SetProperties(kError, kError);
    }
  }

  // See Fst<>::Copy() for doc.
  DifferenceFst(const DifferenceFst& fst, bool safe = false)
      : Base(fst, safe) {}

  // Get a copy of this DifferenceFst. See Fst<>::Copy() for further doc.
  DifferenceFst* Copy(bool safe = false) const override {
    return new DifferenceFst(*this, safe);
  }

 private:
  using Base::GetImpl;

  static std::shared_ptr<Impl> CreateDifferenceImplWithCacheOpts(
      const Fst<Arc>& fst1, const Fst<Arc>& fst2, const CacheOptions& opts) {
    using RM = RhoMatcher<Matcher<Fst<A>>>;
    ComplementFst<Arc> cfst(fst2);
    ComposeFstOptions<A, RM> copts(
        opts, new RM(fst1, MATCH_NONE),
        new RM(cfst, MATCH_INPUT, ComplementFst<Arc>::kRhoLabel));
    return CreateBase1(fst1, cfst, copts);
  }

  template <class Matcher, class Filter, class StateTable>
  static std::shared_ptr<Impl> CreateDifferenceImplWithDifferenceOpts(
      const Fst<Arc>& fst1, const Fst<Arc>& fst2,
      const DifferenceFstOptions<Arc, Matcher, Filter, StateTable>& opts) {
    using RM = RhoMatcher<Matcher>;
    ComplementFst<Arc> cfst(fst2);
    ComposeFstOptions<Arc, RM> copts(opts);
    copts.matcher1 = new RM(fst1, MATCH_NONE, kNoLabel, MATCHER_REWRITE_ALWAYS,
                            opts.matcher1);
    copts.matcher2 = new RM(cfst, MATCH_INPUT, ComplementFst<Arc>::kRhoLabel,
                            MATCHER_REWRITE_ALWAYS, opts.matcher2);
    return CreateBase1(fst1, cfst, copts);
  }
};

// Specialization for DifferenceFst.
template <class Arc>
class StateIterator<DifferenceFst<Arc>>
    : public StateIterator<ComposeFst<Arc>> {
 public:
  explicit StateIterator(const DifferenceFst<Arc>& fst)
      : StateIterator<ComposeFst<Arc>>(fst) {}
};

// Specialization for DifferenceFst.
template <class Arc>
class ArcIterator<DifferenceFst<Arc>> : public ArcIterator<ComposeFst<Arc>> {
 public:
  using StateId = typename Arc::StateId;

  ArcIterator(const DifferenceFst<Arc>& fst, StateId s)
      : ArcIterator<ComposeFst<Arc>>(fst, s) {}
};

// Useful alias when using StdArc.
using StdDifferenceFst = DifferenceFst<StdArc>;

using DifferenceOptions = ComposeOptions;

// Computes the difference between two FSAs. This version writes the difference
// to an output MutableFst. Only strings that are in the first automaton but not
// in the second are retained in the result.
//
// The first argument must be an acceptor; the second argument must be an
// unweighted, epsilon-free, deterministic acceptor. One of the arguments must
// be label-sorted.
//
// Complexity: same as Compose.
//
// Caveats: same as Compose.
template <class Arc>
void Difference(const Fst<Arc>& ifst1, const Fst<Arc>& ifst2,
                MutableFst<Arc>* ofst,
                const DifferenceOptions& opts = DifferenceOptions()) {
  using M = Matcher<Fst<Arc>>;
  // In each case, we cache only the last state for fastest copy.
  switch (opts.filter_type) {
    case AUTO_FILTER: {
      CacheOptions nopts;
      nopts.gc_limit = 0;
      *ofst = DifferenceFst<Arc>(ifst1, ifst2, nopts);
      break;
    }
    case SEQUENCE_FILTER: {
      DifferenceFstOptions<Arc> dopts;
      dopts.gc_limit = 0;
      *ofst = DifferenceFst<Arc>(ifst1, ifst2, dopts);
      break;
    }
    case ALT_SEQUENCE_FILTER: {
      DifferenceFstOptions<Arc, M, AltSequenceComposeFilter<M>> dopts;
      dopts.gc_limit = 0;
      *ofst = DifferenceFst<Arc>(ifst1, ifst2, dopts);
      break;
    }
    case MATCH_FILTER: {
      DifferenceFstOptions<Arc, M, MatchComposeFilter<M>> dopts;
      dopts.gc_limit = 0;
      *ofst = DifferenceFst<Arc>(ifst1, ifst2, dopts);
      break;
    }
    case NO_MATCH_FILTER: {
      DifferenceFstOptions<Arc, M, NoMatchComposeFilter<M>> dopts;
      dopts.gc_limit = 0;
      *ofst = DifferenceFst<Arc>(ifst1, ifst2, dopts);
      break;
    }
    case NULL_FILTER: {
      DifferenceFstOptions<Arc, M, NullComposeFilter<M>> dopts;
      dopts.gc_limit = 0;
      *ofst = DifferenceFst<Arc>(ifst1, ifst2, dopts);
      break;
    }
    case TRIVIAL_FILTER: {
      DifferenceFstOptions<Arc, M, TrivialComposeFilter<M>> dopts;
      dopts.gc_limit = 0;
      *ofst = DifferenceFst<Arc>(ifst1, ifst2, dopts);
      break;
    }
  }
  if (opts.connect) Connect(ofst);
}

}  // namespace fst

#endif  // OPENFST_LIB_DIFFERENCE_H_
