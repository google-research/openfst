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
// Class to reweight/push an FST, and utility functions to weigh and reweight
// an FST.

#ifndef OPENFST_LIB_PUSH_H_
#define OPENFST_LIB_PUSH_H_

#include <cstdint>
#include <utility>
#include <vector>

#include "absl/log/log.h"
#include "openfst/lib/arc-map.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/factor-weight.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/mutable-fst.h"
#include "openfst/lib/reweight.h"
#include "openfst/lib/shortest-distance.h"
#include "openfst/lib/string-weight.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/lib/weight.h"

namespace fst {

// Computes the total weight (sum of the weights of all accepting paths) from
// the output of ShortestDistance, using the shortest distance from the final
// state when reverse is true and from the initial state otherwise.
template <class Arc>
typename Arc::Weight ComputeTotalWeight(
    const Fst<Arc>& fst, const std::vector<typename Arc::Weight>& distance,
    bool reverse) {
  const auto start = fst.Start();
  if (start < 0) return Arc::Weight::Zero();  // kNoStateId is -1.
  if (reverse) {
    return start < distance.size() ? distance[start] : Arc::Weight::Zero();
  }
  auto sum = Arc::Weight::Zero();
  for (typename Arc::StateId s = 0; s < distance.size(); ++s) {
    sum = Plus(sum, Times(distance[s], fst.Final(s)));
  }
  return sum;
}

// Divides the weight of every accepting path by a fixed weight. This weight
// is also divided at the final state if at_final is true and at the initial
// state otherwise.
template <class Arc>
void RemoveWeight(MutableFst<Arc>* fst, const typename Arc::Weight& weight,
                  bool at_final) {
  using Weight = typename Arc::Weight;
  if ((weight == Weight::One()) || (weight == Weight::Zero())) return;
  const auto start = fst->Start();
  if (start < 0) return;  // kNoStateId is -1.
  if (at_final) {
    for (StateIterator<MutableFst<Arc>> siter(*fst); !siter.Done();
         siter.Next()) {
      fst->SetFinal(siter.Value(),
                    Divide(fst->Final(siter.Value()), weight, DIVIDE_RIGHT));
    }
  } else {
    for (MutableArcIterator<MutableFst<Arc>> aiter(fst, start); !aiter.Done();
         aiter.Next()) {
      auto arc = aiter.Value();
      arc.weight = Divide(arc.weight, weight, DIVIDE_LEFT);
      aiter.SetValue(arc);
    }
    fst->SetFinal(start, Divide(fst->Final(start), weight, DIVIDE_LEFT));
  }
}

// Pushes the weights in FST in the requested direction. If pushing towards the
// initial state, the sum of the weight of the outgoing transitions and final
// weight at a non-initial state is equal to One() in the resulting machine. If
// pushing towards the final state, the same property holds on the reverse
// machine.
//
// Weight needs to be left distributive when pushing towards the initial state
// and right distributive when pushing towards the final states.
template <class Arc>
void Push(MutableFst<Arc>* fst, ReweightType type = REWEIGHT_TO_INITIAL,
          float delta = kShortestDelta, bool remove_total_weight = false) {
  using Weight = typename Arc::Weight;
  std::vector<Weight> distance;
  const bool reverse = type == REWEIGHT_TO_INITIAL;
  ShortestDistance(*fst, &distance, reverse, delta);
  if (remove_total_weight) {
    const auto total_weight = ComputeTotalWeight(*fst, distance, reverse);
    Reweight(fst, distance, type);
    RemoveWeight(fst, total_weight, !reverse);
  } else {
    Reweight(fst, distance, type);
  }
}

inline constexpr uint8_t kPushWeights = 0x01;
inline constexpr uint8_t kPushLabels = 0x02;
inline constexpr uint8_t kPushRemoveTotalWeight = 0x04;
inline constexpr uint8_t kPushRemoveCommonAffix = 0x08;

// Pushes the weights and/or labels of the input FST into the output mutable FST
// by pushing weights and/or labels (as determined by the ptype argument)
// towards the initial state or final states (as determined by the rtype
// template parameter). The weight type must be left distributive when pushing
// weights towards the initial state, and right distribution when pushing
// weights towards the final states.
template <class Arc, ReweightType rtype>
void Push(const Fst<Arc>& ifst, MutableFst<Arc>* ofst, uint8_t ptype,
          float delta = kShortestDelta) {
  using Label = typename Arc::Label;
  using Weight = typename Arc::Weight;
  if ((ptype & (kPushWeights | kPushLabels)) == kPushWeights) {
    *ofst = ifst;
    Push(ofst, rtype, delta, ptype & kPushRemoveTotalWeight);
  } else if (ptype & kPushLabels) {
    const auto gtype =
        rtype == REWEIGHT_TO_INITIAL ? GALLIC_LEFT : GALLIC_RIGHT;
    using StringWeightType = StringWeight<Label, GallicStringType(gtype)>;
    using GallicWeight = typename GallicArc<Arc, gtype>::Weight;
    std::vector<GallicWeight> gdistance;
    VectorFst<GallicArc<Arc, gtype>> gfst;
    ArcMap(ifst, &gfst, ToGallicMapper<Arc, gtype>());
    if (ptype & kPushWeights) {
      ShortestDistance(gfst, &gdistance, rtype == REWEIGHT_TO_INITIAL, delta);
    } else {
      ArcMapFst uwfst(ifst, RmWeightMapper<Arc>());
      ArcMapFst guwfst(uwfst, ToGallicMapper<Arc, gtype>());
      ShortestDistance(guwfst, &gdistance, rtype == REWEIGHT_TO_INITIAL, delta);
    }
    auto total_weight = GallicWeight::One();
    const bool remove_total_weight = ptype & kPushRemoveTotalWeight;
    const bool remove_common_affix = ptype & kPushRemoveCommonAffix;
    if (remove_total_weight || remove_common_affix) {
      const auto sum =
          ComputeTotalWeight(gfst, gdistance, rtype == REWEIGHT_TO_INITIAL);
      // Leave a Zero() component as One() so RemoveWeight does not divide by
      // Zero() when there is no accepting path or when path weights cancel.
      auto common_affix = StringWeightType::One();
      if (remove_common_affix && sum.Value1() != StringWeightType::Zero()) {
        common_affix = sum.Value1();
      }
      auto weight_sum = Weight::One();
      if (remove_total_weight && sum.Value2() != Weight::Zero()) {
        weight_sum = sum.Value2();
      }
      total_weight =
          GallicWeight(std::move(common_affix), std::move(weight_sum));
    }
    Reweight(&gfst, gdistance, rtype);
    if (remove_total_weight || remove_common_affix) {
      RemoveWeight(&gfst, total_weight, rtype == REWEIGHT_TO_FINAL);
    }
    FactorWeightFst<GallicArc<Arc, gtype>, GallicFactor<Label, Weight, gtype>>
        fwfst(gfst);
    ArcMap(fwfst, ofst, FromGallicMapper<Arc, gtype>());
    ofst->SetOutputSymbols(ifst.OutputSymbols());
  } else {
    LOG(WARNING) << "Push: pushing type is set to 0, so not pushing";
    *ofst = ifst;
  }
}

}  // namespace fst

#endif  // OPENFST_LIB_PUSH_H_
