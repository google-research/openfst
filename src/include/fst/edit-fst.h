
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
//
// Copyright 2005-2010 Google, Inc.
// Author: dbikel@google.com (Dan Bikel)
//
// An \ref Fst implementation that allows non-destructive edit operations on an
// existing fst.

#ifndef FST_LIB_EDIT_FST_H_
#define FST_LIB_EDIT_FST_H_

#include <vector>
using std::vector;
#include <fst/cache.h>

namespace fst {

// This class enables non-destructive edit operations on a wrapped ExpandedFst.
// The implementation uses copy-on-write semantics at the node level: if a user
// has an underlying fst on which he or she wants to perform a relatively small
// number of edits (read: mutations), then this implementation will copy the
// edited node to an internal MutableFst and perform any edits in situ on that
// copied node. This class supports all the methods of MutableFst except for
// DeleteStates(const vector<StateId> &); thus, new nodes may also be added, and
// one may add transitions from existing nodes of the wrapped fst to new nodes.
//
// template parameters:
//   A the type of arc to use
//   MutableFstT the type of mutable fst to use internally for edited states
template <typename A, typename MutableFstT = VectorFst<A> >
class EditFstImpl : public FstImpl<A> {
 public:
  using FstImpl<A>::SetProperties;
  using FstImpl<A>::SetInputSymbols;
  using FstImpl<A>::SetOutputSymbols;
  using FstImpl<A>::WriteHeader;

  typedef A Arc;
  typedef typename Arc::Weight Weight;
  typedef typename Arc::StateId StateId;

  // Constructs an editable fst implementation with no states.  Effectively,
  // this initially-empty fst will in every way mimic the behavior of
  // a VectorFst--more precisely, a VectorFstImpl instance--but with slightly
  // slower performance (by a constant factor), due to the fact that
  // this class maintains a mapping between external state id's and
  // their internal equivalents.
  EditFstImpl() : num_new_states_(0) {
    FstImpl<A>::SetType("edit");
    wrapped_ = new MutableFstT();
    InheritPropertiesFromWrapped();
    SetEmptyAndDeleteKeysForInternalMaps();
  }

  // Wraps the specified ExpandedFst. This constructor requires that the
  // specified Fst is an ExpandedFst instance. This requirement is only enforced
  // at runtime. (See below for the reason.)
  //
  // This library uses the pointer-to-implementation or "PIMPL" design pattern.
  // In particular, to make it convenient to bind an implementation class to its
  // interface, there are a pair of template "binder" classes, one for immutable
  // and one for mutable fst's (ImplToFst and ImplToMutableFst, respectively).
  // As it happens, the API for the ImplToMutableFst<I,F> class requires that
  // the implementation class--the template parameter "I"--have a constructor
  // taking a const Fst<A> reference.  Accordingly, the constructor here must
  // perform a static_cast to the ExpandedFst<A> type required by EditFst and
  // therefore EditFstImpl.
  explicit EditFstImpl(const Fst<A> &wrapped)
      : wrapped_(static_cast<ExpandedFst<A> *>(wrapped.Copy())),
        num_new_states_(0) {
    FstImpl<A>::SetType("edit");

    // have edits_ inherit all properties from wrapped_
    edits_.SetProperties(wrapped_->Properties(kFstProperties, false),
                         kFstProperties);

    InheritPropertiesFromWrapped();
    SetEmptyAndDeleteKeysForInternalMaps();
  }

  // A copy constructor for this implementation class, used to implement
  // the Copy() method of the Fst interface.
  EditFstImpl(const EditFstImpl &impl)
      : wrapped_(static_cast<ExpandedFst<A> *>(impl.wrapped_->Copy())),
        edits_(*(impl.edits_.Copy(true))),
        external_to_internal_ids_(impl.external_to_internal_ids_),
        edited_final_weights_(impl.edited_final_weights_),
        num_new_states_(impl.num_new_states_) {
    InheritPropertiesFromWrapped();
    SetEmptyAndDeleteKeysForInternalMaps();
  }

  ~EditFstImpl() {
    delete wrapped_;
  }

  // const Fst/ExpandedFst operations, declared in the Fst and ExpandedFst
  // interfaces
  StateId Start() const {
    StateId edited_start = edits_.Start();
    return edited_start == kNoStateId ? wrapped_->Start() : edited_start;
  }

  Weight Final(StateId s) const {
    FinalWeightIterator final_weight_it = GetFinalWeightIterator(s);
    if (final_weight_it == NotInFinalWeightMap()) {
      IdMapIterator it = GetEditedIdMapIterator(s);
      return it == NotInEditedMap() ?
             wrapped_->Final(s) : edits_.Final(it->second);
    }
    else {
      return final_weight_it->second;
    }
  }

  size_t NumArcs(StateId s) const {
    IdMapIterator it = GetEditedIdMapIterator(s);
    return it == NotInEditedMap() ?
           wrapped_->NumArcs(s) : edits_.NumArcs(it->second);
  }

  size_t NumInputEpsilons(StateId s) const {
    IdMapIterator it = GetEditedIdMapIterator(s);
    return it == NotInEditedMap() ?
           wrapped_->NumInputEpsilons(s) : edits_.NumInputEpsilons(it->second);
  }

  size_t NumOutputEpsilons(StateId s) const {
    IdMapIterator it = GetEditedIdMapIterator(s);
    return it == NotInEditedMap() ?
           wrapped_->NumOutputEpsilons(s) :
           edits_.NumOutputEpsilons(it->second);
  }

  StateId NumStates() const {
    return wrapped_->NumStates() + num_new_states_;
  }

  static EditFstImpl<A, MutableFstT> *Read(istream &strm,
                                           const FstReadOptions &opts);

  bool Write(ostream &strm, const FstWriteOptions &opts) const {
    FstHeader hdr;
    hdr.SetStart(Start());
    hdr.SetNumStates(NumStates());
    FstWriteOptions header_opts(opts);
    header_opts.write_isymbols = false;  // Let contained FST hold any symbols.
    header_opts.write_osymbols = false;
    WriteHeader(strm, header_opts, kFileVersion, &hdr);

    // First, serialize wrapped fst to stream.
    FstWriteOptions wrapped_opts(opts);
    wrapped_opts.write_header = true;  // Force writing contained header.
    wrapped_->Write(strm, wrapped_opts);
    // Next, serialize all private data members of this class.
    FstWriteOptions edits_opts(opts);
    edits_opts.write_header = true;  // Force writing contained header.
    edits_.Write(strm, edits_opts);
    WriteType(strm, external_to_internal_ids_);
    WriteType(strm, edited_final_weights_);
    WriteType(strm, num_new_states_);

    strm.flush();
    if (!strm) {
      LOG(ERROR) << "EditFst::Write: write failed: " << opts.source;
      return false;
    }
    return true;
  }
  // end const Fst operations

  // non-const MutableFst operations

  // Sets the start state for this fst.
  void SetStart(StateId s);

  // Sets the final state for this fst.
  void SetFinal(StateId s, Weight w);

  // Adds a new state to this fst, initially with no arcs.
  StateId AddState();

  // Adds the specified arc to the specified state of this fst.
  void AddArc(StateId s, const Arc &arc);

  void DeleteStates(const vector<StateId>& dstates) {
    LOG(ERROR) << ": EditFstImpl::DeleteStates(const std::vector<StateId>&): "
               << " not implemented";
  }

  // Deletes all states in this fst.
  void DeleteStates();

  // Removes all but the first n outgoing arcs of the specified state.
  void DeleteArcs(StateId s, size_t n) {
    edits_.DeleteArcs(GetEditableInternalId(s), n);
    SetProperties(DeleteArcsProperties(FstImpl<A>::Properties()));
  }

  // Removes all outgoing arcs from the specified state.
  void DeleteArcs(StateId s) {
    edits_.DeleteArcs(GetEditableInternalId(s));
    SetProperties(DeleteArcsProperties(FstImpl<A>::Properties()));
  }

  // end non-const MutableFst operations

  // Provides information for the generic state iterator.
  void InitStateIterator(StateIteratorData<Arc> *data) const {
    data->base = 0;
    data->nstates = NumStates();
  }

  // Provides information for the generic arc iterator.
  void InitArcIterator(StateId s, ArcIteratorData<Arc> *data) const {
    IdMapIterator id_map_it = GetEditedIdMapIterator(s);
    if (id_map_it == NotInEditedMap()) {
      wrapped_->InitArcIterator(s, data);
    } else {
      edits_.InitArcIterator(id_map_it->second, data);
    }
  }

  // Provides information for the generic mutable arc iterator.
  void InitMutableArcIterator(StateId s, MutableArcIteratorData<A> *data) {
    data->base =
        new MutableArcIterator<MutableFstT>(&edits_, GetEditableInternalId(s));
  }

 private:
  typedef typename unordered_map<StateId, StateId>::const_iterator
    IdMapIterator;
  typedef typename unordered_map<StateId, Weight>::const_iterator
    FinalWeightIterator;
  // Properties always true of this Fst class
  static const uint64 kStaticProperties = kExpanded | kMutable;
  // Current file format version
  static const int kFileVersion = 2;
  // Minimum file format version supported
  static const int kMinFileVersion = 2;

  // Causes this fst to inherit all the properties from its wrapped fst, except
  // for the two properties that always apply to EditFst instances: kExpanded
  // and kMutable.
  void InheritPropertiesFromWrapped() {
    SetProperties(wrapped_->Properties(kCopyProperties, false) |
                  kStaticProperties);
    SetInputSymbols(wrapped_->InputSymbols());
    SetOutputSymbols(wrapped_->OutputSymbols());
  }

  void SetEmptyAndDeleteKeysForInternalMaps() {
  }

  // Returns the iterator of the map from external to internal state id's
  // of edits_ for the specified external state id.
  IdMapIterator GetEditedIdMapIterator(StateId s) const {
    return external_to_internal_ids_.find(s);
  }
  IdMapIterator NotInEditedMap() const {
    return external_to_internal_ids_.end();
  }

  FinalWeightIterator GetFinalWeightIterator(StateId s) const {
    return edited_final_weights_.find(s);
  }
  FinalWeightIterator NotInFinalWeightMap() const {
    return edited_final_weights_.end();
  }

  // Returns the internal state id of the specified external id if the state has
  // already been made editable, or else copies the state from wrapped_
  // to edits_ and returns the state id of the newly editable state in edits_.
  //
  // \return makes the specified state editable if it isn't already and returns
  //         its state id in edits_
  StateId GetEditableInternalId(StateId s) {
    IdMapIterator id_map_it = GetEditedIdMapIterator(s);
    if (id_map_it == NotInEditedMap()) {
      StateId new_internal_id = edits_.AddState();
      external_to_internal_ids_[s] = new_internal_id;
      for (ArcIterator< Fst<A> > arc_iterator(*wrapped_, s);
          !arc_iterator.Done();
          arc_iterator.Next()) {
        edits_.AddArc(new_internal_id, arc_iterator.Value());
      }
      // copy the final weight
      FinalWeightIterator final_weight_it = GetFinalWeightIterator(s);
      if (final_weight_it == NotInFinalWeightMap()) {
        edits_.SetFinal(new_internal_id, wrapped_->Final(s));
      }
      else {
        edits_.SetFinal(new_internal_id, final_weight_it->second);
        edited_final_weights_.erase(s);
      }
      return new_internal_id;
    } else {
      return id_map_it->second;
    }
  }

  // The fst that this fst wraps.  The purpose of this class is to enable
  // non-destructive edits on this wrapped fst.
  const ExpandedFst<A> *wrapped_;
  // A mutable fst (by default, a VectorFst) to contain new states, and/or
  // copies of states from wrapped_ that have been modified in some way.
  MutableFstT edits_;
  // A mapping from external state id's to the internal id's of states that
  // appear in edits_.
  unordered_map<StateId, StateId> external_to_internal_ids_;
  // A mapping from external state id's to final state weights assigned to
  // those states.  The states in this map are *only* those whose final weight
  // has been modified; if any other part of the state has been modified,
  // the entire state is copied to edits_, and all modifications reside there.
  unordered_map<StateId, Weight> edited_final_weights_;
  // The number of new states added to this mutable fst impl, which is <= the
  // number of states in edits_.
  StateId num_new_states_;
};

template <class A, class M> const uint64 EditFstImpl<A, M>::kStaticProperties;

// EditFstImpl IMPLEMENTATION STARTS HERE

template<typename A, typename MutableFstT>
inline void EditFstImpl<A, MutableFstT>::SetStart(StateId s) {
  edits_.SetStart(s);
  SetProperties(SetStartProperties(FstImpl<A>::Properties()));
}

template<typename A, typename MutableFstT>
inline void EditFstImpl<A, MutableFstT>::SetFinal(StateId s, Weight w) {
  Weight old_weight = Final(s);
  IdMapIterator it = GetEditedIdMapIterator(s);
  // if we haven't already edited state s, don't add it to edited_ (which can
  // be expensive if s has many transitions); just use the
  // edited_final_weights_ map
  if (it == NotInEditedMap()) {
    edited_final_weights_[s] = w;
  }
  else {
    edits_.SetFinal(GetEditableInternalId(s), w);
  }
  SetProperties(SetFinalProperties(FstImpl<A>::Properties(), old_weight, w));
}

template<typename A, typename MutableFstT>
inline typename A::StateId EditFstImpl<A, MutableFstT>::AddState() {
  StateId internal_state_id = edits_.AddState();
  StateId external_state_id = NumStates();
  external_to_internal_ids_[external_state_id] = internal_state_id;
  num_new_states_++;
  SetProperties(AddStateProperties(FstImpl<A>::Properties()));
  return external_state_id;
}

template<typename A, typename MutableFstT>
inline void EditFstImpl<A, MutableFstT>::AddArc(StateId s, const Arc &arc) {
  StateId internal_id = GetEditableInternalId(s);

  size_t num_arcs = edits_.NumArcs(internal_id);
  ArcIterator<MutableFstT> arc_it(edits_, internal_id);
  const A *prev_arc = NULL;
  if (num_arcs > 0) {
    // grab the final arc associated with this state in edits_
    arc_it.Seek(num_arcs - 1);
    prev_arc = &(arc_it.Value());
  }
  SetProperties(AddArcProperties(FstImpl<A>::Properties(), s, arc, prev_arc));

  edits_.AddArc(internal_id, arc);
}

template<typename A, typename MutableFstT>
inline void EditFstImpl<A, MutableFstT>::DeleteStates() {
  edits_.DeleteStates();
  num_new_states_ = 0;
  external_to_internal_ids_.clear();
  delete wrapped_;
  // we are deleting all states, so just forget about pointer to wrapped_
  // and do what default constructor does: set wrapped_ to a new VectorFst
  wrapped_ = new MutableFstT();
  uint64 newProps = DeleteAllStatesProperties(FstImpl<A>::Properties(),
                                              kStaticProperties);
  FstImpl<A>::SetProperties(newProps);
}

template <typename A, typename MutableFstT>
EditFstImpl<A, MutableFstT> *
EditFstImpl<A, MutableFstT>::Read(istream &strm,
                                  const FstReadOptions &opts) {
  EditFstImpl<A, MutableFstT> *impl = new EditFstImpl();
  FstHeader hdr;
  if (!impl->ReadHeader(strm, opts, kMinFileVersion, &hdr)) {
    return 0;
  }
  impl->SetStart(hdr.Start());

  // first, read in wrapped fst
  FstReadOptions wrapped_opts(opts);
  wrapped_opts.header = 0;  // Contained header was written out, so read it in.
  Fst<A> *wrapped_fst = Fst<A>::Read(strm, wrapped_opts);
  if (!wrapped_fst) {
    return 0;
  }
  impl->wrapped_ = static_cast<ExpandedFst<A> *>(wrapped_fst);
  // next read in MutabelFstT machine that stores edits
  FstReadOptions edits_opts(opts);
  edits_opts.header = 0;  // Contained header was written out, so read it in.

  // Because our internal representation of edited states is a solid object
  // of type MutableFstT (defaults to VectorFst<A>) and not a pointer,
  // and because the static Read method allocates a new object on the heap,
  // we need to call Read, check if there was a failure, use
  // MutableFstT::operator= to assign the object (not the pointer) to the
  // edits_ data member (which will increase the ref count by 1 on the impl)
  // and, finally, delete the heap-allocated object.
  MutableFstT *edits = MutableFstT::Read(strm, edits_opts);
  if (!edits) {
    delete wrapped_fst;
    return 0;
  }
  impl->edits_ = *edits;
  delete edits;
  // finally, read in rest of private data members
  ReadType(strm, &impl->external_to_internal_ids_);
  ReadType(strm, &impl->edited_final_weights_);
  ReadType(strm, &impl->num_new_states_);
  if (!strm) {
    LOG(ERROR) << "EditFst::Read: read failed: " << opts.source;
    return 0;
  }

  impl->SetEmptyAndDeleteKeysForInternalMaps();

  return impl;
}

// END EditFstImpl IMPLEMENTATION

// Concrete, editable FST.  This class attaches interface to implementation.
template <class A>
class EditFst : public ImplToMutableFst< EditFstImpl<A> > {
 public:
  friend class MutableArcIterator< EditFst<A> >;

  typedef A Arc;
  typedef typename A::StateId StateId;
  typedef EditFstImpl<A> Impl;

  EditFst() : ImplToMutableFst<Impl>(new Impl()) {}

  explicit EditFst(const Fst<A> &fst) :
      ImplToMutableFst<Impl>(new Impl(fst)) {}

  // See Fst<>::Copy() for doc.
  EditFst(const EditFst<A> &fst, bool safe = false)
      : ImplToMutableFst<Impl>(fst, safe) {}

  virtual ~EditFst() {}

  // Get a copy of this EditFst. See Fst<>::Copy() for further doc.
  virtual EditFst<A> *Copy(bool safe = false) const {
    return new EditFst<A>(*this, safe);
  }

  EditFst<A> &operator=(const EditFst<A> &fst) {
    SetImpl(fst.GetImpl(), false);
    return *this;
  }

  virtual EditFst<A> &operator=(const Fst<A> &fst) {
    if (this != &fst) {
      SetImpl(new Impl(fst));
    }
    return *this;
  }

  // Read an EditFst from an input stream; return NULL on error.
  static EditFst<A> *Read(istream &strm, const FstReadOptions &opts) {
    Impl* impl = Impl::Read(strm, opts);
    return impl ? new EditFst<A>(impl) : 0;
  }

  // Read an EditFst from a file; return NULL on error.
  // Empty filename reads from standard input.
  static EditFst<A> *Read(const string &filename) {
    Impl* impl = ImplToExpandedFst<Impl, MutableFst<A> >::Read(filename);
    return impl ? new EditFst<A>(impl) : 0;
  }

  virtual void InitStateIterator(StateIteratorData<Arc> *data) const {
    GetImpl()->InitStateIterator(data);
  }

  virtual void InitArcIterator(StateId s, ArcIteratorData<Arc> *data) const {
    GetImpl()->InitArcIterator(s, data);
  }

  virtual
  void InitMutableArcIterator(StateId s, MutableArcIteratorData<A> *data) {
    GetImpl()->InitMutableArcIterator(s, data);
  }
 private:
  explicit EditFst(Impl *impl) : ImplToMutableFst<Impl>(impl) {}

  // Makes visible to friends.
  Impl *GetImpl() const { return ImplToFst< Impl, MutableFst<A> >::GetImpl(); }

  void SetImpl(Impl *impl, bool own_impl = true) {
    ImplToFst< Impl, MutableFst<A> >::SetImpl(impl, own_impl);
  }
};

}  // namespace fst

#endif  // FST_LIB_EDIT_FST_H_
