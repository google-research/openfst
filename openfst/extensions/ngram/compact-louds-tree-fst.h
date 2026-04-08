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
// LoudsTreeCompactor implements a compactor backed by a louds tree class to
// save space. The compacted fst should be a tree with no multiedges. See
// "Engineering the LOUDS Succinct Tree Representation" - Delpratt, Rahman,
// Raman for information about the LOUDS tree data structure.
// https://link.springer.com/chapter/10.1007/11764298_12
//
// The input automaton must meet a few constraints:
// 1) Trim
// 2) Tree structure with the root having stateid 0
// 3) No multiedges
// These properties are checked by HasLoudsTreeStructure.
// The input Fst and the resulting Fst may have different state
// numberings, since LOUDS implicitly orders nodes in breadth first order.
//
// The element compactors (to compact arcs and weights) also have constraints
// based on if the fst is an acceptor/transducer or weighted/unweighted.

#ifndef OPENFST_EXTENSIONS_NGRAM_COMPACT_LOUDS_TREE_FST_H_
#define OPENFST_EXTENSIONS_NGRAM_COMPACT_LOUDS_TREE_FST_H_

#include <cstddef>
#include <cstdint>
#include <istream>
#include <limits>
#include <memory>
#include <ostream>
#include <queue>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "absl/base/no_destructor.h"
#include "openfst/extensions/ngram/louds-tree.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/compact-fst.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/properties.h"

namespace fst {

template <class Arc>
class DefaultLoudsTreeElementCompactor {
 public:
  using Weight = typename Arc::Weight;
  using Label = typename Arc::Label;
  using StateId = typename Arc::StateId;

  using FinalElement = Weight;
  using ArcElement = std::tuple<Label, Label, Weight>;

  DefaultLoudsTreeElementCompactor() = default;

  FinalElement CompactFinalWeight(const Weight& w) const { return w; }
  ArcElement CompactArc(const Arc& arc) const {
    return std::make_tuple(arc.ilabel, arc.olabel, arc.weight);
  }
  Arc GetArc(const ArcElement& element, StateId nextstate) const {
    return Arc(std::get<0>(element), std::get<1>(element), std::get<2>(element),
               nextstate);
  }
  Weight GetFinalWeight(const FinalElement& element) const { return element; }
  static const std::string& Type() {
    static const absl::NoDestructor<std::string> type("default");
    return *type;
  }

  bool IsCompatible(const Fst<Arc>& fst) const { return true; }

  size_t Properties() const { return 0; }
};

template <class Arc>
class UnweightedLoudsTreeElementCompactor {
 public:
  using Weight = typename Arc::Weight;
  using Label = typename Arc::Label;
  using StateId = typename Arc::StateId;

  using FinalElement = char;
  using ArcElement = std::pair<Label, Label>;

  UnweightedLoudsTreeElementCompactor() = default;

  FinalElement CompactFinalWeight(const Weight& w) const {
    return w == Weight::One();
  }
  ArcElement CompactArc(const Arc& arc) const {
    return std::make_pair(arc.ilabel, arc.olabel);
  }
  Arc GetArc(const ArcElement& element, StateId nextstate) const {
    return Arc(element.first, element.second, Weight::One(), nextstate);
  }
  Weight GetFinalWeight(const FinalElement& element) const {
    return element ? Weight::One() : Weight::Zero();
  }

  static const std::string& Type() {
    static const absl::NoDestructor<std::string> type("unweighted");
    return *type;
  }

  bool IsCompatible(const Fst<Arc>& fst) const {
    return fst.Properties(kUnweighted, true) == kUnweighted;
  }

  size_t Properties() const { return kUnweighted; }
};

template <class Arc>
class AcceptorLoudsTreeElementCompactor {
 public:
  using Weight = typename Arc::Weight;
  using Label = typename Arc::Label;
  using StateId = typename Arc::StateId;

  using FinalElement = Weight;
  using ArcElement = std::pair<Label, Weight>;
  AcceptorLoudsTreeElementCompactor() = default;

  FinalElement CompactFinalWeight(const Weight& w) const { return w; }
  ArcElement CompactArc(const Arc& arc) const {
    return std::make_pair(arc.ilabel, arc.weight);
  }
  Arc GetArc(const ArcElement& element, StateId nextstate) const {
    return Arc(element.first, element.first, element.second, nextstate);
  }
  Weight GetFinalWeight(const FinalElement& element) const { return element; }

  static const std::string& Type() {
    static const absl::NoDestructor<std::string> type("acceptor");
    return *type;
  }
  bool IsCompatible(const Fst<Arc>& fst) const {
    return fst.Properties(kAcceptor, true) == kAcceptor;
  }

  size_t Properties() const { return kAcceptor; }
};

template <class Arc>
class UnweightedAcceptorLoudsTreeElementCompactor {
 public:
  using Weight = typename Arc::Weight;
  using Label = typename Arc::Label;
  using StateId = typename Arc::StateId;

  using FinalElement = char;
  using ArcElement = Label;
  UnweightedAcceptorLoudsTreeElementCompactor() = default;

  FinalElement CompactFinalWeight(const Weight& w) const {
    return w == Weight::One();
  }
  ArcElement CompactArc(const Arc& arc) const { return arc.ilabel; }
  Arc GetArc(const ArcElement& element, StateId nextstate) const {
    return Arc(element, element, Weight::One(), nextstate);
  }
  Weight GetFinalWeight(const FinalElement& element) const {
    return element ? Weight::One() : Weight::Zero();
  }
  static const std::string& Type() {
    static const absl::NoDestructor<std::string> type("unweighted-acceptor");
    return *type;
  }

  bool IsCompatible(const Fst<Arc>& fst) const {
    return fst.Properties(kUnweighted | kAcceptor, true) ==
           (kUnweighted | kAcceptor);
  }

  size_t Properties() const { return kUnweighted | kAcceptor; }
};

// The LoudsTreeCompactor implements the new interface described in
// compact-fst.h. The LOUDS tree topology and the
// arc weights/labels and final weights are stored in this class. The weights
// and labels are compacted/expanded via the LoudsTreeElementCompactor.
template <class Arc, class LoudsTreeElementCompactor>
class LoudsTreeCompactor {
 public:
  using StateId = typename Arc::StateId;
  using Label = typename Arc::Label;
  using Weight = typename Arc::Weight;

  using FinalElement = typename LoudsTreeElementCompactor::FinalElement;
  using ArcElement = typename LoudsTreeElementCompactor::ArcElement;

  LoudsTreeCompactor()
      : elementcompactor_(std::make_shared<LoudsTreeElementCompactor>()) {}

  explicit LoudsTreeCompactor(const Fst<Arc>& fst,
                              std::shared_ptr<LoudsTreeCompactor> = nullptr)
      : tree_(std::make_shared<LoudsTree>()),
        elementcompactor_(std::make_shared<LoudsTreeElementCompactor>()) {
    std::vector<size_t> degreeseq;
    std::queue<StateId> q;
    q.push(fst.Start());
    StateId cur;
    StateId nextstate = 1;
    while (!q.empty()) {
      cur = q.front();
      q.pop();
      degreeseq.push_back(fst.NumArcs(cur));
      nodes_.push_back(elementcompactor_->CompactFinalWeight(fst.Final(cur)));
      for (ArcIterator<Fst<Arc>> aiter(fst, cur); !aiter.Done(); aiter.Next()) {
        auto arc = aiter.Value();
        q.push(arc.nextstate);
        // relabeling the fst to be labeled in bfs order
        arcs_.push_back(elementcompactor_->CompactArc(
            Arc(arc.ilabel, arc.olabel, arc.weight, nextstate++)));
      }
    }
    tree_->Init(degreeseq);
  }

  LoudsTreeCompactor(std::shared_ptr<LoudsTree> tree,
                     std::vector<ArcElement> arcs,
                     std::vector<FinalElement> nodes)
      : tree_(tree),
        arcs_(arcs),
        nodes_(nodes),
        elementcompactor_(std::make_shared<LoudsTreeElementCompactor>()) {}

  class LoudsTreeState {
   public:
    LoudsTreeState() : stateid_(kNoStateId) {}

    LoudsTreeState(const LoudsTreeCompactor* c, StateId s)
        : c_(c), stateid_(s) {
      const auto firstandlast = c_->tree_->FirstAndLastChild(stateid_);
      if (firstandlast.first == std::numeric_limits<size_t>::max()) {
        firstchild_ = -1;
        numchildren_ = 0;
      } else {
        firstchild_ = firstandlast.first;
        numchildren_ = firstandlast.second - firstandlast.first + 1;
      }
    }

    StateId GetStateId() const { return stateid_; }

    Weight Final() const {
      return c_->elementcompactor_->GetFinalWeight(
          c_->GetFinalElement(stateid_));
    }

    size_t NumArcs() const { return numchildren_; }

    Arc GetArc(size_t i, uint32_t flag = kArcValueFlags) const {
      const size_t nextstate = firstchild_ + i;
      return c_->elementcompactor_->GetArc(c_->GetArcElement(nextstate - 1),
                                           nextstate);
    }

   private:
    const LoudsTreeCompactor* c_;
    StateId stateid_;
    StateId firstchild_;
    size_t numchildren_;
  };

  using State = LoudsTreeState;
  friend State;

  /* DefaultLoudsTreeState class */

  StateId Start() const { return 0; }

  StateId NumStates() const { return tree_->NumNodes(); }

  size_t NumArcs() const { return NumStates() - 1; }

  bool IsCompatible(const Fst<Arc>& fst) const {
    return HasLoudsTreeStructure(fst) && elementcompactor_->IsCompatible(fst);
  }

  uint64_t Properties(uint64_t props) const {
    props &= kStateSortProperties;  // States are reordered.
    return kExpanded | kAcyclic | kInitialAcyclic | kAccessible |
           kCoAccessible | kTopSorted | elementcompactor_->Properties() | props;
  }

  static const std::string& Type() {
    static const absl::NoDestructor<std::string> type(
        LoudsTreeElementCompactor::Type() + "-compactor");
    return *type;
  }

  bool Error() const { return false; }

  bool Write(std::ostream& strm, const FstWriteOptions& opts) const {
    tree_->Write(strm);
    strm.write(reinterpret_cast<const char*>(arcs_.data()),
               arcs_.size() * sizeof(ArcElement));
    strm.write(reinterpret_cast<const char*>(nodes_.data()),
               nodes_.size() * sizeof(FinalElement));
    return strm ? true : false;
  }

  static LoudsTreeCompactor<Arc, LoudsTreeElementCompactor>* Read(
      std::istream& strm, const FstReadOptions& opts, const FstHeader& hdr) {
    size_t states = hdr.NumStates();
    auto tree_ = LoudsTree::Read(strm);

    std::vector<ArcElement> arcs_;
    arcs_.resize(states - 1);
    std::vector<FinalElement> nodes_;
    nodes_.resize(states);
    strm.read(reinterpret_cast<char*>(&arcs_[0]),
              sizeof(ArcElement) * (states - 1));
    strm.read(reinterpret_cast<char*>(&nodes_[0]),
              sizeof(FinalElement) * states);
    if (!strm) {
      return nullptr;
    }
    return new LoudsTreeCompactor(tree_, arcs_, nodes_);
  }

  void SetState(StateId s, LoudsTreeState* state) const {
    *state = LoudsTreeState(this, s);
  }

  std::shared_ptr<LoudsTree> GetTree() const { return tree_; }

 private:
  ArcElement GetArcElement(size_t i) const { return arcs_[i]; }
  FinalElement GetFinalElement(size_t i) const { return nodes_[i]; }

  std::shared_ptr<LoudsTree> tree_;
  std::vector<ArcElement> arcs_;
  std::vector<FinalElement> nodes_;
  std::shared_ptr<LoudsTreeElementCompactor> elementcompactor_;
};

// Checks for the required structure of a louds tree fst. We only care
// that it is acyclic and trim and that the start state is 0. We relabel
// the states later to give it some ordering.
template <typename Arc>
bool HasLoudsTreeStructure(const Fst<Arc>& fst) {
  size_t numstates = 0;
  size_t numarcs = 0;

  if (fst.Start() != 0) return false;
  for (StateIterator<Fst<Arc>> siter(fst); !siter.Done(); siter.Next()) {
    numstates++;
    numarcs += fst.NumArcs(siter.Value());
  }
  if (numarcs != numstates - 1) return false;
  if (fst.Properties(kAcyclic | kAccessible | kCoAccessible, true) !=
      (kAcyclic | kAccessible | kCoAccessible)) {
    return false;
  }
  return true;
}

// Some useful templated aliases for CompactLoudsTreeFsts.
template <class Arc>
using DefaultCompactLoudsTreeFst =
    CompactFst<Arc,
               LoudsTreeCompactor<Arc, DefaultLoudsTreeElementCompactor<Arc>>>;
template <class Arc>
using UnweightedCompactLoudsTreeFst = CompactFst<
    Arc, LoudsTreeCompactor<Arc, UnweightedLoudsTreeElementCompactor<Arc>>>;
template <class Arc>
using AcceptorCompactLoudsTreeFst =
    CompactFst<Arc,
               LoudsTreeCompactor<Arc, AcceptorLoudsTreeElementCompactor<Arc>>>;
template <class Arc>
using UnweightedAcceptorCompactLoudsTreeFst =
    CompactFst<Arc, LoudsTreeCompactor<
                        Arc, UnweightedAcceptorLoudsTreeElementCompactor<Arc>>>;

// Some useful aliases when using StdArc.
using StdCompactLoudsTreeFst = DefaultCompactLoudsTreeFst<StdArc>;
using StdUnweightedCompactLoudsTreeFst = UnweightedCompactLoudsTreeFst<StdArc>;
using StdAcceptorLoudsTreeFst = AcceptorCompactLoudsTreeFst<StdArc>;
using StdUnweightedAcceptorCompactLoudsTreeFst =
    UnweightedAcceptorCompactLoudsTreeFst<StdArc>;

}  // namespace fst
#endif  // OPENFST_EXTENSIONS_NGRAM_COMPACT_LOUDS_TREE_FST_H_
