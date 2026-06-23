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

// Factory helpers for creating FSTs from structures known at compile time.

#ifndef OPENFST_TEST_FST_FROM_TUPLES_H_
#define OPENFST_TEST_FST_FROM_TUPLES_H_

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/symbol-table.h"
#include "openfst/lib/vector-fst.h"

namespace fst {

// Allow use of InlineFstArc below with a single argument for the labels of the
// arc.
template <class A = StdArc>
struct AcceptorLabel {
  typedef typename A::Label Label;
  explicit AcceptorLabel(Label l) : label(l) {}
  Label label;
};

namespace internal {

// Forward declaration for use by InlineFstArc.
template <typename A>
struct SymbolicInlineFstArc;

// Arc definition used in an initializer list. Used by CreateFstFromTuples only.
template <class A>
struct InlineFstArc {
  typedef typename A::StateId StateId;
  typedef typename A::Label Label;
  typedef typename A::Weight Weight;

  InlineFstArc(StateId p, StateId n, Label i, Label o, Weight w)
      : prevstate(p), nextstate(n), ilabel(i), olabel(o), weight(w) {}

  template <class W>
  InlineFstArc(StateId p, StateId n, Label i, Label o, W w)
      : prevstate(p), nextstate(n), ilabel(i), olabel(o), weight(w) {}

  InlineFstArc(StateId p, StateId n, AcceptorLabel<A> label, Weight w)
      : prevstate(p),
        nextstate(n),
        ilabel(label.label),
        olabel(label.label),
        weight(w) {}

  template <class W>
  InlineFstArc(StateId p, StateId n, AcceptorLabel<A> label, W w)
      : prevstate(p),
        nextstate(n),
        ilabel(label.label),
        olabel(label.label),
        weight(w) {}

  StateId prevstate;
  StateId nextstate;
  Label ilabel;
  Label olabel;
  Weight weight;
};

// Similar to InlineFstArc, but uses string identifiers for the arc labels,
// rather than integer labels, to make tests more legibile. Used by
// CreateFstFromTuples() only.
template <class A>
struct SymbolicInlineFstArc {
  typedef typename A::StateId StateId;
  typedef typename A::Weight Weight;

  SymbolicInlineFstArc(StateId p, StateId n, absl::string_view i,
                       absl::string_view o, Weight w)
      : prevstate(p), nextstate(n), ilabel(i), olabel(o), weight(w) {}

  template <class W>
  SymbolicInlineFstArc(StateId p, StateId n, absl::string_view i,
                       absl::string_view o, W w)
      : prevstate(p), nextstate(n), ilabel(i), olabel(o), weight(w) {}

  StateId prevstate;
  StateId nextstate;
  std::string ilabel;
  std::string olabel;
  Weight weight;
};

// Final state used in an initializer list. Used by CreateFstFromTuples only.
template <class F>
struct InlineFinalState {
  typedef typename F::StateId StateId;
  typedef typename F::Weight Weight;

  InlineFinalState(StateId s, Weight w) : state(s), weight(w) {}

  template <class W>
  InlineFinalState(StateId s, W w) : state(s), weight(w) {}

  StateId state;
  Weight weight;
};

// Creates an FST from a collection of arcs.  Used to implement the
// CreateFstFromTuples() functions below.
template <typename F>
inline F CreateFstFromInlineArcs(
    absl::Span<const InlineFstArc<typename F::Arc>> arcs,
    absl::Span<const InlineFinalState<F>> final_states) {
  F fst;
  for (const auto& arc : arcs) {
    while (arc.prevstate >= fst.NumStates() ||
           arc.nextstate >= fst.NumStates()) {
      fst.AddState();
    }
    fst.AddArc(arc.prevstate, typename F::Arc(arc.ilabel, arc.olabel,
                                              arc.weight, arc.nextstate));
  }
  for (const auto& final : final_states) {
    while (final.state >= fst.NumStates()) fst.AddState();
    fst.SetFinal(final.state, final.weight);
  }
  if (!arcs.empty())
    fst.SetStart(arcs.begin()->prevstate);
  else if (!final_states.empty())
    fst.SetStart(final_states.begin()->state);
  return fst;
}

}  // namespace internal

// Creates an FST with constant structure known at compile time. The input
// format is similar to the FST text format. Arcs are specified as an
// initializer list {state, next_state, ilabel, olabel, weight} and final
// weights are specified as an initializer list {state, weight}.
// F must be a mutable Fst, i.e. implement AddState(), AddArc(), SetFinal().
// Example usage:
//   CreateFstFromTuples<StdVectorFst>(  // one state with self loop
//       {{0, 0, 1, 2, 3.0}},  // implicit arc weight converted from float
//       {{0, StdArc::Weight::One()}});
//
//   CreateFstFromTuples<StdVectorFst>(
//       {{0, 0, 1, 2, StdArc::Weight(3.0)}},  // explicit arc weight
//       {{0, StdArc::Weight::One()}});
//
//   CreateFstFromTuples<StdVectorFst>(  // 3 states, 3 arcs
//       {{0, 1, 7, 8, 10.0},
//        {1, 2, 3, 5, 4.0},
//        {1, 2, 4, 2, 3.0}},
//       {{2, 0.1}});
template <class F = StdVectorFst>
inline F CreateFstFromTuples(
    std::initializer_list<internal::InlineFstArc<typename F::Arc>> arcs,
    std::initializer_list<internal::InlineFinalState<F>> final_states) {
  return internal::CreateFstFromInlineArcs<F>(arcs, final_states);
}

// Variant of CreateFstFromTuples() which uses symbolic, rather than numeric,
// arc labels.  Requires a symbol table to be provided.  Returns an error status
// if symbol lookup fails.
//
// Example usage:
//   CreateFstFromTuples<StdVectorFst>(  // 3 states, 3 arcs
//       {{0, 1, "a", "abd", 10.0},
//        {1, 2, "b", "<epsilon>", 4.0},
//        {1, 2, "d", "<epsilon>", 3.0}},
//       {{2, 0.1}},
//       isymbols, osymbols);
template <typename F = StdVectorFst>
inline absl::StatusOr<F> CreateFstFromTuples(
    std::initializer_list<internal::SymbolicInlineFstArc<typename F::Arc>>
        symbolic_arcs,
    std::initializer_list<internal::InlineFinalState<F>> final_states,
    const SymbolTable& isymbols, const SymbolTable& osymbols) {
  std::vector<internal::InlineFstArc<typename F::Arc>> resolved_arcs;
  resolved_arcs.reserve(symbolic_arcs.size());
  for (const auto& symbolic_arc : symbolic_arcs) {
    const typename F::Arc::Label ilabel = isymbols.Find(symbolic_arc.ilabel);
    if (ilabel == kNoSymbol) {
      return absl::InvalidArgumentError(absl::StrCat(
          "Could not find label for input symbol '", symbolic_arc.ilabel, "'"));
    }
    const typename F::Arc::Label olabel = osymbols.Find(symbolic_arc.olabel);
    if (olabel == kNoSymbol) {
      return absl::InvalidArgumentError(absl::StrCat(
          "Could not find label for output symbol '",
          symbolic_arc.olabel, "'"));
    }
    resolved_arcs.push_back(internal::InlineFstArc<typename F::Arc>(
        symbolic_arc.prevstate, symbolic_arc.nextstate, ilabel, olabel,
        symbolic_arc.weight));
  }
  return internal::CreateFstFromInlineArcs<F>(resolved_arcs, final_states);
}

// Variant of CreateFstFromTuples() with symbolic arc labels that uses the same
// symbol table for both input and output symbols.
template <typename F = StdVectorFst>
inline absl::StatusOr<F> CreateFstFromTuples(
    std::initializer_list<internal::SymbolicInlineFstArc<typename F::Arc>>
        symbolic_arcs,
    std::initializer_list<internal::InlineFinalState<F>> final_states,
    const SymbolTable& symbols) {
  return CreateFstFromTuples(symbolic_arcs, final_states, symbols, symbols);
}

}  // namespace fst

#endif  // OPENFST_TEST_FST_FROM_TUPLES_H_
