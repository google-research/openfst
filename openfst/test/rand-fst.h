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

#ifndef OPENFST_TEST_RAND_FST_H_
#define OPENFST_TEST_RAND_FST_H_

#include <cstddef>
#include <cstdint>
#include <random>

#include "absl/log/log.h"
#include "absl/status/status.h"
#include "openfst/lib/mutable-fst.h"
#include "openfst/lib/properties.h"
#include "openfst/lib/verify.h"

namespace fst {

// Generates a random FST.
// If acyclic_prob is 1.0, the FST is guaranteed to be acyclic.
// If acyclic_prob is 0.0, the FST is likely to be cyclic.
template <class Arc, class Generate>
absl::Status RandFst(const int num_random_states, const int num_random_arcs,
                     const int num_random_labels, const float acyclic_prob,
                     Generate generate, uint64_t seed, MutableFst<Arc>* fst) {
  if (num_random_states < 0) {
    return absl::InvalidArgumentError("num_random_states must be non-negative");
  }
  if (num_random_arcs < 0) {
    return absl::InvalidArgumentError("num_random_arcs must be non-negative");
  }
  if (num_random_arcs > 0 && num_random_labels <= 0) {
    return absl::InvalidArgumentError("num_random_labels must be positive");
  }
  if (acyclic_prob < 0.0 || acyclic_prob > 1.0) {
    return absl::InvalidArgumentError("acyclic_prob must be in [0, 1]");
  }

  using Label = typename Arc::Label;
  using StateId = typename Arc::StateId;

  // Determines direction of the arcs wrt state numbering. This way we
  // can force acyclicity when desired.
  enum ArcDirection {
    ANY_DIRECTION = 0,
    FORWARD_DIRECTION = 1,
    REVERSE_DIRECTION = 2,
    NUM_DIRECTIONS = 3
  };

  fst->DeleteStates();
  if (num_random_states == 0) return absl::OkStatus();

  std::mt19937_64 rand(seed);
  const StateId ns =
      std::uniform_int_distribution<>(0, num_random_states - 1)(rand);
  if (ns == 0) return absl::OkStatus();
  fst->AddStates(ns);

  std::uniform_int_distribution<StateId> ns_dist(0, ns - 1);
  const StateId start = ns_dist(rand);
  fst->SetStart(start);

  ArcDirection arc_direction = ANY_DIRECTION;
  if (std::bernoulli_distribution(acyclic_prob)(rand)) {
    arc_direction = std::bernoulli_distribution(.5)(rand) ? FORWARD_DIRECTION
                                                          : REVERSE_DIRECTION;
  }

  if (num_random_arcs > 0) {
    std::uniform_int_distribution<size_t> arc_dist(0, num_random_arcs - 1);
    std::uniform_int_distribution<Label> label_dist(0, num_random_labels - 1);
    const size_t na = arc_dist(rand);
    for (size_t n = 0; n < na; ++n) {
      StateId s = ns_dist(rand);
      Arc arc;
      arc.ilabel = label_dist(rand);
      arc.olabel = label_dist(rand);
      arc.weight = generate();
      arc.nextstate = ns_dist(rand);
      if ((arc_direction == FORWARD_DIRECTION ||
           arc_direction == REVERSE_DIRECTION) &&
          s == arc.nextstate) {
        continue;  // Skips self-loops.
      }

      if ((arc_direction == FORWARD_DIRECTION && s > arc.nextstate) ||
          (arc_direction == REVERSE_DIRECTION && s < arc.nextstate)) {
        StateId t = s;  // reverses arcs
        s = arc.nextstate;
        arc.nextstate = t;
      }

      fst->AddArc(s, arc);
    }
  }

  const StateId nf = std::uniform_int_distribution<>(0, ns)(rand);
  for (StateId n = 0; n < nf; ++n) {
    const StateId s = ns_dist(rand);
    fst->SetFinal(s, generate());
  }
  VLOG(1) << "Check FST for sanity (including property bits).";
  if (!Verify(*fst)) {
    return absl::InternalError("FST verification failed");
  }

  // Get/compute all properties.
  const uint64_t props = fst->Properties(kFstProperties, true);

  // Select random set of properties to be unknown.
  uint64_t mask = 0;
  for (int n = 0; n < 8; ++n) {
    mask |= std::uniform_int_distribution<>(0, 0xff)(rand);
    mask <<= 8;
  }
  mask &= ~kTrinaryProperties;
  fst->SetProperties(props & ~mask, mask);
  return absl::OkStatus();
}

}  // namespace fst

#endif  // OPENFST_TEST_RAND_FST_H_
