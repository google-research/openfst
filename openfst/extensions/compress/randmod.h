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
// Generates a random FST according to a class-specific transition model.

#ifndef OPENFST_EXTENSIONS_COMPRESS_RANDMOD_H_
#define OPENFST_EXTENSIONS_COMPRESS_RANDMOD_H_

#include <cstdlib>
#include <vector>

#include "absl/random/bit_gen_ref.h"
#include "absl/random/random.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/mutable-fst.h"

namespace fst {

template <class Arc, class G>
class RandMod {
 public:
  using Label = typename Arc::Label;
  using StateId = typename Arc::StateId;
  using Weight = typename Arc::Weight;

  // Generates random FST with 'nstates' with 'nclasses' in the probability
  // generation model, and 'nlabels' in the alphabet. If 'trans' = true, then
  // a transducer is generated; iff 'generate_' is non-null, the output is
  // randomly weighted.
  RandMod(StateId nstates, StateId nclasses, Label nlabels, bool trans,
          const G* generate, absl::BitGenRef bit_gen)
      : nstates_(nstates),
        nlabels_(nlabels),
        trans_(trans),
        generate_(generate) {
    for (StateId s = 0; s < nstates; ++s) {
      classes_.push_back(absl::Uniform<StateId>(bit_gen, 0, nclasses));
    }
  }

  // Generates a random FST according to a class-specific transition model.
  void Generate(StdMutableFst* fst, absl::BitGenRef bit_gen) {
    StateId start = absl::Uniform<StateId>(bit_gen, 0, nstates_);
    fst->DeleteStates();
    for (StateId s = 0; s < nstates_; ++s) {
      fst->AddState();
      if (s == start) fst->SetStart(start);
      for (StateId n = 0; n <= nstates_; ++n) {
        Arc arc;
        StateId d = n == nstates_ ? kNoStateId : n;
        if (!RandArc(s, d, bit_gen, &arc)) continue;
        if (d == kNoStateId) {  // A super-final transition?
          fst->SetFinal(s, arc.weight);
        } else {
          fst->AddArc(s, arc);
        }
      }
    }
  }

 private:
  // Generates a transition from s to d. If d == kNoStateId, a superfinal
  // transition is generated. Returns false if no transition generated.
  bool RandArc(StateId s, StateId d, absl::BitGenRef bit_gen, Arc* arc) {
    StateId sclass = classes_[s];
    StateId dclass = d != kNoStateId ? classes_[d] : 0;
    int r = sclass + dclass + 2;
    if (!absl::Bernoulli(bit_gen, 1.0 / r)) return false;
    arc->nextstate = d;
    Label ilabel = kNoLabel;
    Label olabel = kNoLabel;
    if (d != kNoStateId) {
      ilabel = (dclass % nlabels_) + 1;
      olabel = trans_ ? (sclass % nlabels_) + 1 : ilabel;
    }
    arc->ilabel = ilabel;
    arc->olabel = olabel;
    arc->weight = generate_ ? (*generate_)(bit_gen) : Weight::One();
    return true;
  }

  StateId nstates_;
  Label nlabels_;
  bool trans_;
  const G* generate_;
  std::vector<StateId> classes_;
};

}  // namespace fst

#endif  // OPENFST_EXTENSIONS_COMPRESS_RANDMOD_H_
