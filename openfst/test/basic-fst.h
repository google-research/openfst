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
// Similar to VectorFst, but with simplified property maintainence.

#ifndef OPENFST_TEST_BASIC_FST_H_
#define OPENFST_TEST_BASIC_FST_H_

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <istream>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

#include "absl/log/log.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/mutable-fst.h"
#include "openfst/lib/properties.h"
#include "openfst/lib/util.h"
#include "openfst/lib/vector-fst.h"

namespace fst {

template <class A>
class BasicFst;

namespace internal {

// This is a VectorFstBaseImpl container that holds VectorState's.
template <class A>
class BasicFstImpl : public VectorFstBaseImpl<VectorState<A>> {
 public:
  using FstImpl<A>::SetInputSymbols;
  using FstImpl<A>::SetOutputSymbols;
  using FstImpl<A>::SetType;
  using FstImpl<A>::SetProperties;
  using FstImpl<A>::Properties;
  using FstImpl<A>::WriteHeader;

  using VectorFstBaseImpl<VectorState<A>>::Start;
  using VectorFstBaseImpl<VectorState<A>>::NumStates;
  using VectorFstBaseImpl<VectorState<A>>::ReserveArcs;
  using VectorFstBaseImpl<VectorState<A>>::GetState;

  friend class MutableArcIterator<BasicFst<A>>;

  using BaseImpl = VectorFstBaseImpl<VectorState<A>>;
  using Weight = typename A::Weight;
  using StateId = typename A::StateId;

  BasicFstImpl() {
    SetType("basic");
    SetProperties(kNullProperties | kStaticProperties);
  }
  explicit BasicFstImpl(const Fst<A>& fst);

  static BasicFstImpl<A>* Read(std::istream& strm, const FstReadOptions& opts);

  bool Write(std::ostream& strm, const FstWriteOptions& opts) const;

  void SetStart(StateId s) {
    BaseImpl::SetStart(s);
    SetProperties(Properties() & kSetStartProperties);
  }

  void SetFinal(StateId s, Weight w) {
    BaseImpl::SetFinal(s, w);
    SetProperties(Properties() & kSetFinalProperties);
  }

  StateId AddState() {
    StateId s = BaseImpl::AddState();
    SetProperties(Properties() & kAddStateProperties);
    return s;
  }

  void AddArc(StateId s, const A& arc) {
    BaseImpl::AddArc(s, arc);
    SetProperties(Properties() & kAddArcProperties);
  }

  void DeleteStates(absl::Span<const StateId> dstates) {
    BaseImpl::DeleteStates(dstates);
    SetProperties(Properties() & kDeleteStatesProperties);
  }

  void DeleteStates() {
    BaseImpl::DeleteStates();
    SetProperties(kNullProperties | kStaticProperties);
  }

  void DeleteArcs(StateId s, size_t n) {
    BaseImpl::DeleteArcs(s, n);
    SetProperties(Properties() & kDeleteArcsProperties);
  }

  void DeleteArcs(StateId s) {
    BaseImpl::DeleteArcs(s);
    SetProperties(Properties() & kDeleteArcsProperties);
  }

 private:
  // Properties always true of this Fst class
  static constexpr uint64_t kStaticProperties = kExpanded | kMutable;
  // Current file format version
  static constexpr int kFileVersion = 1;
  // Minimum file format version supported
  static constexpr int kMinFileVersion = 1;
};

template <class A>
BasicFstImpl<A>::BasicFstImpl(const Fst<A>& fst) {
  SetType("basic");
  SetProperties(fst.Properties(kCopyProperties, false) | kStaticProperties);
  SetInputSymbols(fst.InputSymbols());
  SetOutputSymbols(fst.OutputSymbols());
  BaseImpl::SetStart(fst.Start());

  for (StateIterator<Fst<A>> siter(fst); !siter.Done(); siter.Next()) {
    StateId s = siter.Value();
    BaseImpl::AddState();
    BaseImpl::SetFinal(s, fst.Final(s));
    ReserveArcs(s, fst.NumArcs(s));
    for (ArcIterator<Fst<A>> aiter(fst, s); !aiter.Done(); aiter.Next()) {
      const A& arc = aiter.Value();
      BaseImpl::AddArc(s, arc);
    }
  }
}

template <class A>
BasicFstImpl<A>* BasicFstImpl<A>::Read(std::istream& strm,
                                       const FstReadOptions& opts) {
  BasicFstImpl<A>* impl = new BasicFstImpl;
  FstHeader hdr;
  if (!impl->ReadHeader(strm, opts, kMinFileVersion, &hdr)) return nullptr;
  impl->BaseImpl::SetStart(hdr.Start());
  impl->ReserveStates(hdr.NumStates());

  for (StateId s = 0; s < hdr.NumStates(); ++s) {
    impl->BaseImpl::AddState();
    VectorState<A>* state = impl->GetState(s);
    Weight final;
    final.Read(strm);
    state->SetFinal(final);
    int64_t narcs;
    ReadType(strm, &narcs);
    if (!strm) {
      LOG(ERROR) << "BasicFst::Read: read failed: " << opts.source;
      return nullptr;
    }
    impl->ReserveArcs(s, narcs);
    for (size_t j = 0; j < narcs; ++j) {
      A arc;
      ReadType(strm, &arc.ilabel);
      ReadType(strm, &arc.olabel);
      arc.weight.Read(strm);
      ReadType(strm, &arc.nextstate);
      if (!strm) {
        LOG(ERROR) << "BasicFst::Read: read failed: " << opts.source;
        return nullptr;
      }
      impl->BaseImpl::AddArc(s, arc);
    }
  }
  return impl;
}

template <class A>
bool BasicFstImpl<A>::Write(std::ostream& strm,
                            const FstWriteOptions& opts) const {
  FstHeader hdr;
  hdr.SetStart(Start());
  hdr.SetNumStates(NumStates());
  WriteHeader(strm, opts, kFileVersion, &hdr);

  for (StateId s = 0; s < NumStates(); ++s) {
    const VectorState<A>* state = GetState(s);
    state->Final().Write(strm);
    int64_t narcs = state->NumArcs();
    WriteType(strm, narcs);
    for (size_t a = 0; a < narcs; ++a) {
      const A& arc = state->GetArc(a);
      WriteType(strm, arc.ilabel);
      WriteType(strm, arc.olabel);
      arc.weight.Write(strm);
      WriteType(strm, arc.nextstate);
    }
  }

  strm.flush();
  if (!strm) {
    LOG(ERROR) << "BasicFst::Write: write failed: " << opts.source;
    return false;
  }
  return true;
}

}  // namespace internal

// Simple concrete, mutable FST. This class attaches interface to
// implementation and handles reference counting, delegating most
// methods to ImplToMutableFst. Supports additional operations:
// ReserveStates and ReserveArcs (cf. STL vectors).
template <class A>
class BasicFst : public ImplToMutableFst<internal::BasicFstImpl<A>> {
  using Base = ImplToMutableFst<internal::BasicFstImpl<A>>;

 public:
  friend class StateIterator<BasicFst<A>>;
  friend class ArcIterator<BasicFst<A>>;
  friend class MutableArcIterator<BasicFst<A>>;

  using Arc = A;
  using StateId = typename A::StateId;
  using typename Base::Impl;

  BasicFst() : Base(std::make_shared<Impl>()) {}

  explicit BasicFst(const Fst<A>& fst) : Base(std::make_shared<Impl>(fst)) {}

  BasicFst(const BasicFst<A>& fst) : Base(fst) {}

  // Get a copy of this BasicFst. See Fst<>::Copy() for further doc.
  BasicFst<A>* Copy(bool safe = false) const override {
    return new BasicFst<A>(*this);
  }

  virtual BasicFst<A>& operator=(const BasicFst<A>& fst) {
    SetImpl(fst.GetSharedImpl());
    return *this;
  }

  BasicFst<A>& operator=(const Fst<A>& fst) override {
    if (this != &fst) SetImpl(std::make_shared<Impl>(fst));
    return *this;
  }

  static BasicFst<A>* Read(std::istream& strm, const FstReadOptions& opts) {
    auto* impl = Impl::Read(strm, opts);
    return impl ? new BasicFst<A>(std::shared_ptr<Impl>(impl)) : nullptr;
  }

  // Read a BasicFst from a file; return NULL on error
  static BasicFst<A>* Read(absl::string_view filename) {
    auto* impl = Base::Read(filename);
    return impl ? new BasicFst<A>(std::shared_ptr<Impl>(impl)) : 0;
  }

  bool Write(std::ostream& strm, const FstWriteOptions& opts) const override {
    return GetImpl()->Write(strm, opts);
  }

  bool Write(const std::string& filename) const override {
    return Fst<A>::WriteFile(filename);
  }

  void InitStateIterator(StateIteratorData<A>* data) const override {
    GetImpl()->InitStateIterator(data);
  }

  void InitArcIterator(StateId s, ArcIteratorData<A>* data) const override {
    GetImpl()->InitArcIterator(s, data);
  }

  inline void InitMutableArcIterator(StateId s,
                                     MutableArcIteratorData<A>*) override;

 private:
  explicit BasicFst(std::shared_ptr<Impl> impl) : Base(impl) {}

  using Base::GetImpl;
  using Base::MutateCheck;
  using Base::ReserveArcs;
  using Base::ReserveStates;
  using Base::SetImpl;
};

// Specialization for BasicFst; see generic version in fst.h
// for sample usage (but use the BasicFst type!). This version
// should inline.
template <class A>
class StateIterator<BasicFst<A>> {
 public:
  using StateId = typename A::StateId;

  explicit StateIterator(const BasicFst<A>& fst)
      : nstates_(fst.GetImpl()->NumStates()), s_(0) {}

  bool Done() const { return s_ >= nstates_; }

  StateId Value() const { return s_; }

  void Next() { ++s_; }

  void Reset() { s_ = 0; }

 private:
  StateId nstates_;
  StateId s_;
};

// Specialization for BasicFst; see generic version in fst.h
// for sample usage (but use the BasicFst type!). This version
// should inline.
template <class A>
class ArcIterator<BasicFst<A>> {
 public:
  using StateId = typename A::StateId;

  ArcIterator(const BasicFst<A>& fst, StateId s)
      : arcs_(fst.GetImpl()->GetState(s)->Arcs()),
        narcs_(fst.GetImpl()->GetState(s)->NumArcs()),
        i_(0) {}

  bool Done() const { return i_ >= narcs_; }

  const A& Value() const { return arcs_[i_]; }

  void Next() { ++i_; }

  size_t Position() const { return i_; }

  void Reset() { i_ = 0; }

  void Seek(size_t a) { i_ = a; }

  uint8_t Flags() const { return kArcValueFlags; }

  void SetFlags(uint8_t, uint8_t) {}

 private:
  const A* arcs_;  // Borrowed reference.
  size_t narcs_;
  size_t i_;
};

// Specialization for BasicFst; see generic version in fst.h
// for sample usage (but use the BasicFst type!). This version
// should inline.
template <class A>
class MutableArcIterator<BasicFst<A>> : public MutableArcIteratorBase<A> {
 public:
  using StateId = typename A::StateId;
  using Weight = typename A::Weight;

  MutableArcIterator(BasicFst<A>* fst, StateId s) : i_(0) {
    fst->MutateCheck();
    state_ = fst->GetMutableImpl()->GetState(s);
    properties_ = &fst->GetImpl()->properties_;
  }

  bool Done() const final { return i_ >= state_->NumArcs(); }

  const A& Value() const final { return state_->GetArc(i_); }

  void Next() final { ++i_; }

  size_t Position() const final { return i_; }

  void Reset() final { i_ = 0; }

  void Seek(size_t a) final { i_ = a; }

  void SetValue(const A& arc) final {
    state_->SetArc(arc, i_);
    *properties_ &= kSetArcProperties;
  }

  constexpr uint8_t Flags() const final { return kArcValueFlags; }

  void SetFlags(uint8_t, uint8_t) final {}

 private:
  VectorState<A>* state_;              // Borrowed reference.
  std::atomic<uint64_t>* properties_;  // Borrowed reference.
  size_t i_;
};

// Provide information needed for the generic mutable arc iterator
template <class A>
inline void BasicFst<A>::InitMutableArcIterator(
    StateId s, MutableArcIteratorData<A>* data) {
  data->base = std::make_unique<MutableArcIterator<BasicFst<A>>>(this, s);
}

// A useful alias when using StdArc.
using StdBasicFst = BasicFst<StdArc>;

}  // namespace fst

#endif  // OPENFST_TEST_BASIC_FST_H_
