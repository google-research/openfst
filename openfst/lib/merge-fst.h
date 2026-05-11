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

#ifndef OPENFST_LIB_MERGE_FST_H_
#define OPENFST_LIB_MERGE_FST_H_

// The MergeFst class provides an efficient low-overhead means for
// adding states and transitions to an Fst (the primary Fst) by
// specifying a secondary Fst encoding the transitions and state to be added.
//
// See Cyril Allauzen, Michael Riley (2015): "Rapid vocabulary addition to
// context-dependent decoder graphs", Proceedings of Interspeech 2015,
// pages 2112--2116, Dresden, Germany. doi: 10.21437/Interspeech.2015-477.
// https://www.isca-archive.org/interspeech_2015/allauzen15_interspeech.html
//
// A MergeFst is specified by:
//  * a primary Fst (ExpandedFst),
//  * a secondary Fst (ExpandedFst), and
//  * a state map (hash_map<StateId, StateId>).
//
// A StateId 's' defines a state in the MergeFst iff:
//  (a) s < the number of states in the primary Fst, or
//  (b) state_map[s] != kNoStateId.
// If only (a) holds, 's' is a primary state unchanged by the merger.
// If only (b) holds, 's' is a new state added by the merger.
// If both (a) and (b) hold, 's' is a primary state merged with a
// secondary state, i.e. a merged state.
//
// The arcs out of state 's' in the MergeFst hence are:
//  (1) If s < primary_fst.NumStates(),
//      (a) if state_map[s] == kNoStateId, the arcs out of state 's'
//          in the primary Fst,
//      (b) otherwise, the union of the arcs out of state 's' in
//          the primary Fst and out of state 'state_map[s]' in
//          the secondary Fst.
//  (2) Otherwise, (state_map[s] != kNoStateId), the arcs out of state
//      'state_map[s]' in the secondary Fst.
//
// Observe that, the arcs in the primary and secondary Fst will be
// considered as-is. This means the the destination states of the arcs
// needs to be StateId's in the MergeFst. This implies that
// the secondary Fst is not well-formed Fst. It is can be viewed as
// a partial Fst specification.
//
// The MergeFst class is templated on the class that is used
// to internally represent the state map specification.
//
// // This class defines the state map used to define the MergeFst
// // It maps a state ID 's' in the merge Fst to the corresponding
// // state in the secondary Fst.
// //
// template <class S>
// class MergeStateMap {
//  public:
//   // Constructor for a hash_map and the number of states in the primary Fst.
//   MergeStateMap(const hash_map<S, S> &state_map, S primary_num_states);
//
//   // Copy constructor
//   MergeStateMap(const MergeStateMap &state_map);
//
//   // Returns the state of the secondary Fst that is merged with
//   // state 's' in the merged Fst. Returns 'kNoStateId' if no state of the
//   // secondary Fst are merged with 's'.
//   S Find(S s) const;
//
//   // Read and write methods.
//   bool Write(std::ostream &ostrm) const;
//   static MergeStateMap<S> *Read(std::istream *istrm);
// };

#include <cstddef>
#include <cstdint>
#include <istream>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/log/check.h"
#include "absl/log/log.h"
#include "absl/memory/memory.h"
#include "openfst/compat/compat_memory.h"
#include "absl/strings/string_view.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/cache.h"
#include "openfst/lib/expanded-fst.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/matcher.h"
#include "openfst/lib/properties.h"
#include "openfst/lib/util.h"

namespace fst {

// State map implemented using an hash_map. Imposes no restriction
// on the map. Suitable for testing/use on a CPU.
template <class S>
class HashMergeStateMap {
 public:
  explicit HashMergeStateMap(const absl::flat_hash_map<S, S> &state_map,
                             S = S())
      : state_map_(state_map) {}

  explicit HashMergeStateMap(absl::flat_hash_map<S, S> &&state_map, S = S())
      : state_map_(std::move(state_map)) {}

  HashMergeStateMap(const HashMergeStateMap &) = default;
  HashMergeStateMap(HashMergeStateMap &&) = default;
  HashMergeStateMap &operator=(const HashMergeStateMap &) = default;
  HashMergeStateMap &operator=(HashMergeStateMap &&) = default;

  S Find(S s) const {
    auto iter = state_map_.find(s);
    return iter != state_map_.end() ? iter->second : kNoStateId;
  }

  bool Write(std::ostream &ostrm) const {
    WriteType(ostrm, state_map_);
    return !ostrm ? false : true;
  }

  static HashMergeStateMap<S> *Read(std::istream &istrm) {
    absl::flat_hash_map<S, S> state_map;
    ReadType(istrm, &state_map);
    return !istrm ? nullptr : new HashMergeStateMap<S>(std::move(state_map));
  }

 private:
  absl::flat_hash_map<S, S> state_map_;
};


// Uses a vector to define for state IDs less than the number of states
// in the primary Fsts. Assumes the mapping for other states is defined
// by an offset ('num_merged_states_').
// Candidate for a representation that would be suitable for GPUs.
template <class S>
class VectorMergeStateMap {
 public:
  VectorMergeStateMap(const absl::flat_hash_map<S, S> &state_map,
                      S primary_num_states)
      : primary_num_states_(primary_num_states), num_merged_states_(0) {
    S max_dest_state = kNoStateId;
    for (auto iter = state_map.begin(); iter != state_map.end(); ++iter) {
      if (iter->first < primary_num_states_) {
        ++num_merged_states_;
        if (iter->second > max_dest_state) max_dest_state = iter->second;

        while (states_.size() <= iter->first) states_.push_back(kNoStateId);
        states_[iter->first] = iter->second;
      }
    }
    CHECK_EQ(num_merged_states_, max_dest_state + 1);  // Crash OK
    for (auto iter = state_map.begin(); iter != state_map.end(); ++iter) {
      if (iter->first >= primary_num_states_) {
        CHECK_EQ(iter->first - primary_num_states_,  // Crash OK
                 iter->second - num_merged_states_);
      }
    }
  }

  VectorMergeStateMap(const std::vector<S> &states, S primary_num_states,
                      S num_merged_states)
      : states_(states), primary_num_states_(primary_num_states),
        num_merged_states_(num_merged_states) {
    CHECK_LT(num_merged_states_, primary_num_states_);  // Crash OK
  }

  VectorMergeStateMap(std::vector<S> &&states, S primary_num_states,
                      S num_merged_states)
      : states_(std::move(states)),
        primary_num_states_(primary_num_states),
        num_merged_states_(num_merged_states) {
    CHECK_LT(num_merged_states_, primary_num_states_);  // Crash OK
  }

  VectorMergeStateMap(const VectorMergeStateMap &state_map)
      : states_(state_map.states_),
        primary_num_states_(state_map.primary_num_states_),
        num_merged_states_(state_map.num_merged_states_) {}

  S Find(S s) const {
    if (s < states_.size()) {
      return states_[s];
    } else if (s < primary_num_states_) {
      return kNoStateId;
    } else {  // s >= primary_num_states_
      return s - primary_num_states_ + num_merged_states_;
    }
  }

  bool Write(std::ostream &ostrm) const {
    WriteType(ostrm, states_);
    WriteType(ostrm, primary_num_states_);
    WriteType(ostrm, num_merged_states_);
    return !ostrm ? false : true;
  }

  static VectorMergeStateMap<S> *Read(std::istream &istrm) {
    std::vector<S> states;
    S primary_num_states, num_merged_states;
    ReadType(istrm, &states);
    ReadType(istrm, &primary_num_states);
    ReadType(istrm, &num_merged_states);
    return !istrm
               ? nullptr
               : new VectorMergeStateMap<S>(
                     std::move(states), primary_num_states, num_merged_states);
  }

 private:
  std::vector<S> states_;
  S primary_num_states_;
  S num_merged_states_;

  void operator=(const VectorMergeStateMap&);  // disallow
};


// This merge state map allows am implicit (state-less/open-coded)
// representation based on a predefined function. It imposes some
// restrictions on the mapping.  It requires the existence of an state
// ID 'num_merged_states' such that the merged states are the first
// 'num_merged_states' ones in the primary Fst.
// The mapping is defined by:
//   * '0 <= s < num_merged_states' mapped to 's',
//   * 'num_merged_states <= s < primary_num_states' mapped to 'kNoStateId',
//   * 'primary_num_states <= s' mapped to
//       's - primary_num_states + num_merged_states'.
// This stateless mapping seems like a good candidate for a GPU
// implementation.
template <class S>
class FixedMergeStateMap {
 public:
  FixedMergeStateMap(S primary_num_states, S num_merged_states)
      : primary_num_states_(primary_num_states),
        num_merged_states_(num_merged_states) {}

  // The constructor from hash_map checks that the given mapping
  // is indeed representable by this class.
  FixedMergeStateMap(const absl::flat_hash_map<S, S> &state_map,
                     S primary_num_states)
      : primary_num_states_(primary_num_states), num_merged_states_(0) {
    if (!state_map.empty()) {
      S min_unmerged = primary_num_states;
      S max_merged = kNoStateId;
      S shift = kNoStateId;
      for (auto iter = state_map.begin(); iter != state_map.end(); ++iter) {
        if (iter->first < primary_num_states) {
          LOG(INFO) << iter->first << " -> " << iter->second;
          if (iter->second == kNoStateId) {
            if (iter->first < min_unmerged) min_unmerged = iter->first;
          } else {
            if (iter->first > max_merged) max_merged = iter->first;
            if (iter->first != iter->second) {
              FSTERROR() << "FixedMergeStateMap: provided state map is not"
                         << " compatible";
              return;
            }
          }
        } else {  // iter->first >= primary_num_states
          if (iter->second != kNoStateId && shift == kNoStateId) {
            shift = iter->second - (iter->first - primary_num_states);
          }
          if (iter->second == kNoStateId ||
              (iter->second - shift) != (iter->first - primary_num_states)) {
            FSTERROR() << "FixedMergeStateMap: provided state map is not"
                       << " compatible";
            return;
          }
        }
      }
      if (shift != (max_merged + 1)) {
        FSTERROR() << "FixedMergeStateMap: provided state map is not"
                   << " compatible " << shift << " " << min_unmerged << " "
                   << max_merged;
      }
      num_merged_states_ = shift;
    }
  }

  FixedMergeStateMap(const FixedMergeStateMap &state_map)
      : primary_num_states_(state_map.primary_num_states_),
        num_merged_states_(state_map.num_merged_states_) {}

  S Find(S s) const {
    if (s < num_merged_states_)
      return s;
    else if (s >= primary_num_states_)
      return num_merged_states_ + s - primary_num_states_;
    else  // num_merged_state <= s < primary_num_states
      return kNoStateId;
  }

  bool Write(std::ostream &ostrm) const {
    WriteType(ostrm, primary_num_states_);
    WriteType(ostrm, num_merged_states_);
    return !ostrm ? false : true;
  }

  static FixedMergeStateMap<S> *Read(std::istream &istrm) {
    S primary_num_states, num_merged_states;
    ReadType(istrm, &primary_num_states);
    ReadType(istrm, &num_merged_states);
    return !istrm ? nullptr :
        new FixedMergeStateMap<S>(primary_num_states, num_merged_states);
  }

 private:
  S primary_num_states_;
  S num_merged_states_;

  void operator=(const FixedMergeStateMap&);  // disallow
};

namespace internal {

// Implementation for the MergeFst class.
template <class A, class M>
class MergeFstImpl : public FstImpl<A> {
 public:
  typedef A Arc;
  typedef typename A::Label Label;
  typedef typename A::StateId StateId;
  typedef typename A::Weight Weight;
  typedef CacheState<A> State;
  typedef M StateMap;

  MergeFstImpl() : state_map_(absl::flat_hash_map<StateId, StateId>(), 0) {
    FstImpl<A>::SetType("merge");
  }

  MergeFstImpl(const ExpandedFst<A> &primary_fst,
               const ExpandedFst<A> &secondary_fst,
               const absl::flat_hash_map<StateId, StateId> &state_map)
      : primary_fst_(primary_fst.Copy()),
        secondary_fst_(secondary_fst.Copy()),
        state_map_(state_map, primary_fst.NumStates()),
        num_states_(primary_fst_->NumStates() + secondary_fst_->NumStates()),
        cache_store_(std::make_unique<HashCacheStore<State>>(CacheOptions())) {
    CHECK_EQ(secondary_fst_->NumStates(), state_map.size());  // Crash OK
    absl::flat_hash_set<StateId> sources, destinations;
    for (auto iter = state_map.begin(); iter != state_map.end(); ++iter) {
      destinations.insert(iter->first);
      sources.insert(iter->second);
      if (iter->first < primary_fst_->NumStates()) --num_states_;
    }
    CHECK_EQ(destinations.size(), state_map.size());  // Crash OK
    CHECK_EQ(sources.size(), state_map.size());       // Crash OK

    FstImpl<A>::SetType("merge");
    if (primary_fst_->InputSymbols() || secondary_fst_->InputSymbols()) {
      if (!CompatSymbols(primary_fst_->InputSymbols(),
                         secondary_fst_->InputSymbols())) {
        FSTERROR() << "MergeFstImpl: incompatible input symbol tables.";
      }
      FstImpl<A>::SetInputSymbols(primary_fst_->InputSymbols());
    }
    if (primary_fst_->OutputSymbols() || secondary_fst_->OutputSymbols()) {
      if (!CompatSymbols(primary_fst_->OutputSymbols(),
                         secondary_fst_->OutputSymbols())) {
        FSTERROR() << "MergeFstImpl: incompatible output symbol tables.";
      }
      FstImpl<A>::SetOutputSymbols(primary_fst_->OutputSymbols());
    }
    // TODO: Find better way to determine properties.
    FstImpl<A>::SetProperties(
        primary_fst_->Properties(kCopyProperties, true) &
        kAddStateProperties & kAddArcProperties & kSetFinalProperties);
  }

  MergeFstImpl(const MergeFstImpl<A, M> &impl)
      : FstImpl<A>(impl),
        primary_fst_(impl.primary_fst_->Copy(true)),
        secondary_fst_(impl.secondary_fst_->Copy(true)),
        state_map_(impl.state_map_),
        num_states_(impl.num_states_),
        cache_store_(std::make_unique<HashCacheStore<State>>(CacheOptions())) {}

  ~MergeFstImpl() override = default;

  StateId Start() const { return primary_fst_->Start(); }

  Weight Final(StateId s) const {
    Weight final =  s < primary_fst_->NumStates() ?
        primary_fst_->Final(s) : Weight::Zero();
    StateId ms = state_map_.Find(s);
    if (ms != kNoStateId)
      final = Times(final, secondary_fst_->Final(ms));
    return final;
  }

  StateId NumStates() const { return num_states_; }

  size_t NumArcs(StateId s) const {
    size_t num_arcs = s < primary_fst_->NumStates() ?
        primary_fst_->NumArcs(s) : 0;
    StateId ms = state_map_.Find(s);
    if (ms != kNoStateId)
      num_arcs += secondary_fst_->NumArcs(ms);
    return num_arcs;
  }

  size_t NumInputEpsilons(StateId s) const {
    size_t num_ieps = s < primary_fst_->NumStates() ?
        primary_fst_->NumInputEpsilons(s) : 0;
    StateId ms = state_map_.Find(s);
    if (ms != kNoStateId)
      num_ieps += secondary_fst_->NumInputEpsilons(ms);
    return num_ieps;
  }

  size_t NumOutputEpsilons(StateId s) const {
    size_t num_oeps = s < primary_fst_->NumStates() ?
        primary_fst_->NumOutputEpsilons(s) : 0;
    StateId ms = state_map_.Find(s);
    if (ms != kNoStateId)
      num_oeps += secondary_fst_->NumOutputEpsilons(ms);
    return num_oeps;
  }

  void InitStateIterator(StateIteratorData<A> *data) const {
    data->base = nullptr;
    data->nstates = num_states_;
  }

  void InitArcIterator(StateId s, ArcIteratorData<A> *data) {
    StateId ms = state_map_.Find(s);
    if (s >= primary_fst_->NumStates()) {
      secondary_fst_->InitArcIterator(ms, data);
      return;
    } else if (ms == kNoStateId) {
      primary_fst_->InitArcIterator(s, data);
      return;
    }

    State *state = cache_store_->GetMutableState(s);
    if (!(state->Flags() & kCacheArcs)) {
      state->ReserveArcs(
          primary_fst_->NumArcs(s) + secondary_fst_->NumArcs(ms));
      for (ArcIterator<ExpandedFst<A> > aiter(*primary_fst_, s);
           !aiter.Done(); aiter.Next()) {
        state->PushArc(aiter.Value());
      }
      for (ArcIterator<ExpandedFst<A> > aiter(*secondary_fst_, ms);
           !aiter.Done(); aiter.Next()) {
        state->PushArc(aiter.Value());
      }
      state->SetArcs();
      state->SetFlags(kCacheArcs, kCacheArcs);
    }
    data->base = nullptr;
    data->narcs = state->NumArcs();
    data->arcs = state->Arcs();
    data->ref_count = state->MutableRefCount();
  }

  const StateMap &GetStateMap() const { return state_map_; }
  const ExpandedFst<A> &PrimaryFst() const { return *primary_fst_; }
  const ExpandedFst<A> &SecondaryFst() const { return *secondary_fst_; }

  bool Write(std::ostream &strm, const FstWriteOptions &opts) const {
    FstHeader hdr;
    hdr.SetNumStates(NumStates());
    FstImpl<A>::WriteHeader(strm, opts, kFileVersion, &hdr);
    if (!state_map_.Write(strm)) return false;
    FstWriteOptions fopts(opts);
    fopts.write_header = true;     // Force writing component Fst headers.
    fopts.write_isymbols = false;  // Symbols written down in main header.
    fopts.write_osymbols = false;
    if (!primary_fst_->Write(strm, fopts)) return false;
    if (!secondary_fst_->Write(strm, fopts)) return false;
    return true;
  }

  static MergeFstImpl<A, M> *Read(std::istream &strm,
                                  const FstReadOptions &opts) {
    FstHeader hdr;
    std::unique_ptr<FstImpl<A> > impl(new FstImpl<A>);
    impl->SetType("merge");
    if (!impl->ReadHeader(strm, opts, kMinFileVersion, &hdr)) return nullptr;
    std::unique_ptr<M> state_map(M::Read(strm));
    if (!state_map) return nullptr;
    FstReadOptions fopts(opts);
    fopts.header = nullptr;  // Component Fst headers were written out.
    std::unique_ptr<ExpandedFst<A> > primary_fst(
        ExpandedFst<A>::Read(strm, fopts));
    if (!primary_fst) return nullptr;
    std::unique_ptr<ExpandedFst<A> > secondary_fst(
        ExpandedFst<A>::Read(strm, fopts));
    if (!secondary_fst) return nullptr;
    return new MergeFstImpl<A, M>(
        *impl, *primary_fst, *secondary_fst, hdr.NumStates(), *state_map);
  }

 private:
  MergeFstImpl(const FstImpl<A> &impl, const ExpandedFst<A> &primary_fst,
               const ExpandedFst<A> &secondary_fst, StateId num_states,
               const StateMap &state_map)
      : FstImpl<A>(impl),
        primary_fst_(primary_fst.Copy()),
        secondary_fst_(secondary_fst.Copy()),
        state_map_(state_map),
        num_states_(num_states),
        cache_store_(std::make_unique<HashCacheStore<State>>(CacheOptions())) {}

  // Current file format version
  static constexpr int kFileVersion = 1;
  // Minimum file format version supported
  static constexpr int kMinFileVersion = 1;

  std::unique_ptr<const ExpandedFst<A>> primary_fst_;
  std::unique_ptr<const ExpandedFst<A>> secondary_fst_;
  StateMap state_map_;
  StateId num_states_;
  std::unique_ptr<HashCacheStore<State>> cache_store_;
};

}  // namespace internal

// MergeFst class.
// Refer to top-of-file comment for class description.
template <class A, class M = HashMergeStateMap<typename A::StateId>>
class MergeFst : public ImplToExpandedFst<internal::MergeFstImpl<A, M>> {
 public:
  friend class ArcIterator<MergeFst<A, M> >;

  typedef A Arc;
  typedef typename A::StateId StateId;
  using Impl = internal::MergeFstImpl<A, M>;

  // To support registration.
  MergeFst() : ImplToExpandedFst<Impl>(std::make_shared<Impl>()) {}

  // Doesn't support construction from an arbitrary FST.
  explicit MergeFst(const Fst<A>&) = delete;

  // Constructor taking as argument the primary Fst, the secondary Fst
  // and the state map specified by a hash_map.
  MergeFst(const ExpandedFst<A> &primary_fst,
           const ExpandedFst<A> &secondary_fst,
           const absl::flat_hash_map<StateId, StateId> &state_map)
      : ImplToExpandedFst<Impl>(
            std::make_shared<Impl>(primary_fst, secondary_fst, state_map)) {}

  // See Fst<>::Copy() for doc
  MergeFst(const MergeFst<A, M> &fst, bool safe = false)
      : ImplToExpandedFst<Impl>(fst, safe) {}

  // Get a copy of this MergeFst. See Fst<>::Copy() for further doc.
  MergeFst<A, M> *Copy(bool safe = false) const override {
    return new MergeFst<A, M>(*this, safe);
  }

  bool Write(std::ostream &strm, const FstWriteOptions &opts) const override {
    return GetImpl()->Write(strm, opts);
  }

  bool Write(const std::string &filename) const override {
    return Fst<Arc>::WriteFile(filename);
  }

  // Read a MergeFst from an input stream; return NULL on error
  static MergeFst<A, M> *Read(std::istream &strm, const FstReadOptions &opts) {
    LOG(INFO) << "MergeFst::Read";
    Impl *impl = Impl::Read(strm, opts);
    return impl ? new MergeFst<A, M>(std::shared_ptr<Impl>(impl)) : nullptr;
  }

  // Read a MergeFst from a file; return NULL on error
  // Empty filename reads from standard input
  static MergeFst<A, M> *Read(absl::string_view filename) {
    Impl *impl = ImplToExpandedFst<Impl>::Read(filename);
    return impl ? new MergeFst<A, M>(std::shared_ptr<Impl>(impl)) : nullptr;
  }

  void InitStateIterator(StateIteratorData<A> *data) const override {
    GetImpl()->InitStateIterator(data);
  }

  void InitArcIterator(StateId s, ArcIteratorData<A> *data) const override {
    GetMutableImpl()->InitArcIterator(s, data);
  }

  MatcherBase<A> *InitMatcher(MatchType match_type) const override {
    // TODO: this won't work, needs own matcher.
    return new SortedMatcher<MergeFst<A, M> >(*this, match_type);
  }

 private:
  using ImplToExpandedFst<internal::MergeFstImpl<A, M>>::GetImpl;
  using ImplToExpandedFst<internal::MergeFstImpl<A, M>>::GetMutableImpl;

  explicit MergeFst(std::shared_ptr<Impl> impl)
      : ImplToExpandedFst<Impl>(impl) {}

  void operator=(const MergeFst<A, M> &fst);  // disallow
};


// ArcIterator specialization for MergeFst.
template <class A, class M>
class ArcIterator<MergeFst<A, M> > {
 public:
  typedef A Arc;
  typedef typename A::StateId StateId;

  ArcIterator(const MergeFst<A, M> &fst, StateId s)
      : primary_num_arcs_(0), secondary_num_arcs_(0) {
    StateId ms = fst.GetImpl()->GetStateMap().Find(s);
    if (ms != kNoStateId) {
      secondary_aiter_ = std::make_unique<ArcIterator<ExpandedFst<A>>>(
          fst.GetImpl()->SecondaryFst(), ms);
      secondary_num_arcs_ = fst.GetImpl()->SecondaryFst().NumArcs(ms);
    }
    if (s < fst.GetImpl()->PrimaryFst().NumStates()) {
      primary_aiter_ = std::make_unique<ArcIterator<ExpandedFst<A>>>(
          fst.GetImpl()->PrimaryFst(), s);
      primary_num_arcs_ = fst.GetImpl()->PrimaryFst().NumArcs(s);
    }
  }

  ~ArcIterator() = default;

  bool Done() const {
    if (primary_aiter_ && !primary_aiter_->Done()) return false;
    if (secondary_aiter_ && !secondary_aiter_->Done()) return false;
    return true;
  }

  const A& Value() const {
    if (primary_aiter_ && !primary_aiter_->Done())
      return primary_aiter_->Value();
    return secondary_aiter_->Value();
  }

  void Next() {
    if (primary_aiter_ && !primary_aiter_->Done())
      primary_aiter_->Next();
    else
      secondary_aiter_->Next();
  }

  size_t Position() const {
    if (primary_aiter_ && !primary_aiter_->Done())
      return primary_aiter_->Position();
    return secondary_aiter_->Position() + primary_num_arcs_;
  }

  void Reset() {
    if (primary_aiter_) primary_aiter_->Reset();
    if (secondary_aiter_) secondary_aiter_->Reset();
  }

  void Seek(size_t pos) {
    if (pos >= primary_num_arcs_) {
      while (primary_aiter_ && !primary_aiter_->Done()) primary_aiter_->Next();
      secondary_aiter_->Seek(pos - primary_num_arcs_);
    } else {
      primary_aiter_->Seek(pos);
    }
  }

  void SetFlags(uint32_t f, uint32_t m) {}

 private:
  std::unique_ptr<ArcIterator<ExpandedFst<A>>> primary_aiter_;
  std::unique_ptr<ArcIterator<ExpandedFst<A>>> secondary_aiter_;
  size_t primary_num_arcs_;
  size_t secondary_num_arcs_;
};

typedef MergeFst<StdArc> StdMergeFst;

}  // namespace fst

#endif  // OPENFST_LIB_MERGE_FST_H_
