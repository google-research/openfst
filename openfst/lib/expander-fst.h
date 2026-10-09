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
// ExpanderFst provides a simple way to create on-the-fly fsts by wrapping a
// state Expander with a caching strategy.
//
// Thread-safety properties are determined by the choice of caching store, but
// the default is an fst that is safe to access from multiple threads.
//
// Performance from a single thread is somewhat better than using a delayed fst
// based on CacheFst when using recency caching, and only slightly slower when
// using a completely thread-safe cache.
//
// Benchmark                          Time(ns)    CPU(ns) Iterations
// -----------------------------------------------------------------
// BM_ExpanderComposeFstGcLimitZero   29213805   29173640        100
// BM_BaselineComposeFstGcLimitZero   33966460   33922822        100
// BM_BaselineComposeFstCached        39861774   39807656        100
// BM_ExpanderComposeFstConcurrent    43866980   43808708        100
//
// Usage:
//   The user provides an Expander class with an Expand(StateId, State*)
//   template method:
//
//   struct Expander {
//     StateId GetStart() { return 0; }
//
//     template <class State>
//     void Expand(StateId state_id, State* state) {
//       if (state_id == 5) {
//         state->SetFinal(10);
//       } else {
//         state->AddArc(StdArc(0, 0, 0.0, state_id + 1));
//       }
//     }
//   }
//
//   And then passes a shared pointer to an Expander to create an ExpanderFst:
//
//   ExpanderFst<Expander> fst(std::make_shared<Expander>(args));
//
//   The ExpanderFst does not have to know anything about how to construct the
//   contained Expander. It is legal to share the Expander (and internally the
//   Fst may have multiple references from different Fst copies).
//
//   If the Expander is thread-safe, the ExpanderFst will be thread-safe.
//
//   The cache used is a template parameter to the ExpanderFst, this determines
//   memory use and cache safety. For example:
//
//   ConcurrentExpanderCache (default):
//     - Every state is cached, no way to shrink the cache.
//     - Concurrent access to the fst/cache is completely safe.
//   NoGcKeepOneExpanderCache:
//     - The last state is cached, reference counted states in use by
//       ArcIterators may also be cached, can be deleted if possible, but
//       no active gc.
//     - Single thread only. But, safe=true fst copies create new caches.
//       Multiple threads may use their own fst views while sharing the same
//       Expander if the expander itself is safe.
//
//   A cache instance may optionally be given to the ExpanderFst constructor to
//   supply pre-cached states. The cache copy constructor will be used to create
//   a thread-safe (non-shared) copy. If sharing is intended, this must be
//   supported by the cache implementation (ie SharedCached).

#ifndef OPENFST_LIB_EXPANDER_FST_H_
#define OPENFST_LIB_EXPANDER_FST_H_

#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>

#include "openfst/lib/expander-cache.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/impl-to-fst.h"

namespace fst {

template <class Expander, class Cache>
class ExpanderStateIterator;

namespace internal {

template <class Expander, class Cache>
class ExpanderFstImpl : public FstImpl<typename Expander::Arc> {
 public:
  using Arc = typename Expander::Arc;
  using StateId = typename Arc::StateId;
  using Weight = typename Arc::Weight;

  using State = typename Cache::State;

  explicit ExpanderFstImpl(std::shared_ptr<Expander> expander,
                           const Cache& cache)
      : expander_(std::move(expander)), cache_(cache) {
    this->SetInputSymbols(expander_->InputSymbols());
    this->SetOutputSymbols(expander_->OutputSymbols());
  }

  explicit ExpanderFstImpl(std::shared_ptr<Expander> expander)
      : ExpanderFstImpl(expander, Cache()) {}

  StateId Start() const { return expander_->Start(); }

  Weight Final(StateId s) const { return FindOrExpand(s)->Final(); }

  size_t NumArcs(StateId s) const { return FindOrExpand(s)->NumArcs(); }

  size_t NumInputEpsilons(StateId s) const {
    return FindOrExpand(s)->NumInputEpsilons();
  }

  size_t NumOutputEpsilons(StateId s) const {
    return FindOrExpand(s)->NumOutputEpsilons();
  }

  State* FindOrExpand(StateId state_id) const {
    return cache_.FindOrExpand(*expander_, state_id);
  }

  StateId NumStates() const { return expander_->NumStates(); }

  Cache* GetCache() const { return &cache_; }

  Expander* GetExpander() const { return expander_.get(); }

 private:
  mutable std::shared_ptr<Expander> expander_;
  mutable Cache cache_;
};

template <class Expander, class Cache>
class ExpanderStateIterator final
    : public StateIteratorBase<typename Expander::Arc> {
 public:
  using StateId = typename Expander::StateId;

  explicit ExpanderStateIterator(const ExpanderFstImpl<Expander, Cache>* impl)
      : impl_(impl) {}

  bool Done() const override { return state_ >= impl_->NumStates(); }

  StateId Value() const override { return state_; }

  void Next() override {
    if (state_ == min_unexpanded_state_) {
      impl_->FindOrExpand(state_);
      min_unexpanded_state_ = state_ + 1;
    }
    ++state_;
  }

  void Reset() override { state_ = 0; }

 private:
  const ExpanderFstImpl<Expander, Cache>* impl_;
  StateId state_ = 0;
  StateId min_unexpanded_state_ = 0;
};

}  // namespace internal

template <class Expander, class Cache = DefaultExpanderCache<Expander>>
struct ExpanderFst
    : public ImplToFst<internal::ExpanderFstImpl<Expander, Cache>> {
  using Base = ImplToFst<internal::ExpanderFstImpl<Expander, Cache>>;
  using Arc = typename Expander::Arc;
  using StateId = typename Arc::StateId;
  using Weight = typename Arc::Weight;

  using typename Base::Impl;
  using State = typename Impl::State;

  explicit ExpanderFst(std::shared_ptr<Expander> expander)
      : Base(std::make_shared<Impl>(expander)) {}

  explicit ExpanderFst(std::shared_ptr<Expander> expander, const Cache& cache)
      : Base(std::make_shared<Impl>(expander, cache)) {}

  ExpanderFst(const ExpanderFst& fst, bool safe = false) : Base(fst, safe) {}

  ExpanderFst* Copy(bool safe = false) const override {
    return new ExpanderFst(*this, safe);
  }

  void InitStateIterator(StateIteratorData<Arc>* data) const override {
    data->base =
        std::make_unique<internal::ExpanderStateIterator<Expander, Cache>>(
            this->GetImpl());
  }

  void InitArcIterator(StateId s, ArcIteratorData<Arc>* data) const override {
    auto* state = this->GetImpl()->FindOrExpand(s);
    data->base = nullptr;
    data->arcs = state->Arcs();
    data->narcs = state->NumArcs();
    data->ref_count = state->MutableRefCount();
    if (data->ref_count != nullptr) ++(*data->ref_count);
  }

  Cache* GetCache() const { return this->GetImpl()->GetCache(); }

  Expander* GetExpander() const { return this->GetImpl()->GetExpander(); }

  friend class ArcIterator<ExpanderFst<Expander, Cache>>;
};

template <class Expander, class Cache>
class ArcIterator<ExpanderFst<Expander, Cache>> {
 public:
  using FST = ExpanderFst<Expander, Cache>;

  using Arc = typename FST::Arc;
  using StateId = typename Arc::StateId;

  ArcIterator(const FST& fst, StateId s) {
    const auto* state = fst.GetMutableImpl()->FindOrExpand(s);
    arcs_ = state->Arcs();
    narcs_ = state->NumArcs();
    ref_count_ = state->MutableRefCount();
    if (ref_count_ != nullptr) ++(*ref_count_);
  }

  ~ArcIterator() {
    if (ref_count_ != nullptr) --(*ref_count_);
  }

  bool Done() const { return i_ >= narcs_; }

  const Arc& Value() const { return arcs_[i_]; }

  void Next() { ++i_; }

  size_t Position() const { return i_; }

  void Reset() { i_ = 0; }

  void Seek(size_t a) { i_ = a; }

  constexpr uint8_t Flags() const { return 0; }

  void SetFlags(uint8_t, uint8_t) {}

 private:
  const Arc* arcs_ = nullptr;
  int32_t narcs_ = 0;
  int32_t i_ = 0;
  int* ref_count_ = nullptr;
};

}  // namespace fst

#endif  // OPENFST_LIB_EXPANDER_FST_H_
