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

// Adaptor for ArcIterator to the STL-style iterator interface.
//
// The iterator returned by ArcRange<F>::begin() and ArcRange<F>::end() is a
// random-access iterator.
//
// GetArcs(fst, state) can be used to create an ArcRange object. The type
// information is deduced from the arguments.
//
// ArcRange can be used for range-based for loops; e.g.
//   for (const auto& arc : GetArcs(fst, state))
//
// Or for using STL algorithms; e.g.
//   auto arc_range = GetArcs(fst, state);
//   std::find_if(arc_range.begin(), arc_range.end(),
//                [](const StdArc& a) { return a.olabel == 42; });

#ifndef OPENFST_LIB_ARC_RANGE_H_
#define OPENFST_LIB_ARC_RANGE_H_

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <memory>
#include <type_traits>

#include "openfst/lib/cache.h"
#include "openfst/lib/const-fst.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/vector-fst.h"

namespace fst {
namespace internal {

// Provides a random access iterator interface for an ArcIterator.
// Uses a shared ArcIterator object, see comments for ArcRange below.
template <class ArcIter_>
class ArcRangeIterator {
 public:
  using ArcIter = ArcIter_;
  // As ArcIter::Arc is not always defined, find the Arc type from the return
  // value of ArcIter::Value().
  using value_type = std::decay_t<decltype(std::declval<ArcIter>().Value())>;
  using difference_type = std::ptrdiff_t;
  using pointer = const value_type*;
  using reference = const value_type&;
  using iterator_category = std::random_access_iterator_tag;

  ArcRangeIterator(std::shared_ptr<ArcIter> arcs, int pos)
      : arcs_(arcs), pos_(pos) {}

  ArcRangeIterator(const ArcRangeIterator& other, int pos)
      : arcs_(other.arcs_), pos_(pos) {}

  ArcRangeIterator& operator++() {
    ++pos_;
    return *this;
  }

  ArcRangeIterator operator++(int /* unused */) {
    ArcRangeIterator tmp = *this;
    ++pos_;
    return tmp;
  }

  ArcRangeIterator& operator--() {
    --pos_;
    return *this;
  }

  ArcRangeIterator operator--(int /* unsed */) {
    ArcRangeIterator tmp = *this;
    --pos_;
    return tmp;
  }

  ArcRangeIterator& operator+=(difference_type d) {
    pos_ += d;
    return *this;
  }

  ArcRangeIterator& operator-=(difference_type d) {
    pos_ -= d;
    return *this;
  }

  reference operator*() const {
    arcs_->Seek(pos_);
    return arcs_->Value();
  }

  pointer operator->() const {
    arcs_->Seek(pos_);
    return &arcs_->Value();
  }

  reference operator[](difference_type d) const {
    arcs_->Seek(pos_ + d);
    return arcs_->Value();
  }

  int position() const { return pos_; }

 private:
  std::shared_ptr<ArcIter> arcs_;
  int pos_;
};

template <class T>
bool operator==(const ArcRangeIterator<T>& a, const ArcRangeIterator<T>& b) {
  return a.position() == b.position();
}

template <class T>
bool operator!=(const ArcRangeIterator<T>& a, const ArcRangeIterator<T>& b) {
  return a.position() != b.position();
}

template <class T>
ArcRangeIterator<T> operator+(const ArcRangeIterator<T>& it,
                              typename ArcRangeIterator<T>::difference_type d) {
  return ArcRangeIterator<T>(it, it.position() + d);
}

template <class T>
ArcRangeIterator<T> operator-(const ArcRangeIterator<T>& it,
                              typename ArcRangeIterator<T>::difference_type d) {
  return ArcRangeIterator<T>(it, it.position() - d);
}

template <class T>
typename ArcRangeIterator<T>::difference_type operator-(
    const ArcRangeIterator<T>& a, const ArcRangeIterator<T>& b) {
  return a.position() - b.position();
}

template <class T>
bool operator<(const ArcRangeIterator<T>& a, const ArcRangeIterator<T>& b) {
  return a.position() < b.position();
}

template <class T>
bool operator>(const ArcRangeIterator<T>& a, const ArcRangeIterator<T>& b) {
  return a.position() > b.position();
}

template <class T>
bool operator<=(const ArcRangeIterator<T>& a, const ArcRangeIterator<T>& b) {
  return a.position() <= b.position();
}

template <class T>
bool operator>=(const ArcRangeIterator<T>& a, const ArcRangeIterator<T>& b) {
  return a.position() >= b.position();
}

// Used for ArcRange specializations for ArcIterators which have just an array
// of arcs.
template <class F>
class ArcPointerRange {
 public:
  using Arc = typename F::Arc;
  using Iterator = const Arc*;

  ArcPointerRange(const F& fst, typename F::StateId state,
                  uint32_t flags = 0 /* unused */) {
    ArcIteratorData<Arc> data;
    fst.InitArcIterator(state, &data);
    begin_ = data.arcs;
    end_ = data.arcs + data.narcs;
  }

  Iterator begin() const { return begin_; }
  Iterator end() const { return end_; }

 private:
  Iterator begin_;
  Iterator end_;
};

// ArcRange using ArcRangeIterator. Used for default ArcRange specializations.
template <class F>
class ArcIterRange {
 public:
  using Iterator = ArcRangeIterator<ArcIterator<F>>;

  ArcIterRange(const F& fst, typename F::StateId state)
      : iter_(std::make_shared<ArcIterator<F>>(fst, state)),
        num_arcs_(fst.NumArcs(state)) {}

  ArcIterRange(const F& fst, typename F::StateId state, uint32_t arc_iter_flags)
      : ArcIterRange(fst, state) {
    iter_->SetFlags(arc_iter_flags, arc_iter_flags);
  }

  Iterator begin() { return Iterator(iter_, 0); }
  Iterator end() { return Iterator(iter_, num_arcs_); }

 private:
  std::shared_ptr<ArcIterator<F>> iter_;
  const size_t num_arcs_;
};

// Selects either ArcIterRange or ArcPointerRange depending on the FST type.
template <class F, class Enable = void>
struct ArcRangeSelector {
  using Type = ArcIterRange<F>;
};

// VectorFst uses the ArcPointerRange.
template <class A>
struct ArcRangeSelector<VectorFst<A>> {
  using Type = ArcPointerRange<VectorFst<A>>;
};

// ConstFst uses the ArcPointerRange.
template <class A>
struct ArcRangeSelector<ConstFst<A>> {
  using Type = ArcPointerRange<ConstFst<A>>;
};

// Uses ArcPointerRange if ArcIterator<F> is derived from CacheArcIterator.
template <class F>
struct ArcRangeSelector<
    F, std::enable_if_t<
           std::is_base_of_v<CacheArcIterator<F>, ArcIterator<F>>>> {
  using Type = ArcPointerRange<F>;
};
}  // namespace internal

// Adapts an ArcIterator to a pair of STL-style iterators.
// Provides begin() and end() for range-based loops.
// The ArcRange and its ArcRange::Iterator objects are not thread-safe. They may
// access a shared ArcIterator object.
template <class F>
class ArcRange : public internal::ArcRangeSelector<F>::Type {
  using Base = typename internal::ArcRangeSelector<F>::Type;

 public:
  using typename Base::Iterator;

  ArcRange(const F& fst, typename F::StateId state) : Base(fst, state) {}

  ArcRange(const F& fst, typename F::StateId state, uint32_t arc_iter_flags)
      : Base(fst, state, arc_iter_flags) {}
};

// Returns an ArcRange object for the arcs of the given fst and state.
template <class F>
ArcRange<F> GetArcs(const F& fst, typename F::StateId state) {
  return ArcRange<F>(fst, state);
}

}  // namespace fst

#endif  // OPENFST_LIB_ARC_RANGE_H_
