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
// Functions and classes to determinize an FST.

#ifndef OPENFST_LIB_DETERMINIZE_H_
#define OPENFST_LIB_DETERMINIZE_H_

#include <algorithm>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <forward_list>
#include <map>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

#include "absl/base/attributes.h"
#include "absl/container/flat_hash_set.h"
#include "absl/memory/memory.h"
#include "absl/types/span.h"
#include "openfst/lib/arc-map.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/arcfilter.h"
#include "openfst/lib/bi-table.h"
#include "openfst/lib/cache.h"
#include "openfst/lib/const-fst.h"
#include "openfst/lib/factor-weight.h"
#include "openfst/lib/filter-state.h"
#include "openfst/lib/float-weight.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/impl-to-fst.h"
#include "openfst/lib/lexicographic-weight.h"
#include "openfst/lib/mutable-fst.h"
#include "openfst/lib/power-weight.h"
#include "openfst/lib/product-weight.h"
#include "openfst/lib/properties.h"
#include "openfst/lib/prune.h"
#include "openfst/lib/shortest-distance.h"
#include "openfst/lib/string-weight.h"
#include "openfst/lib/union-weight.h"
#include "openfst/lib/util.h"
#include "openfst/lib/weight.h"

namespace fst {

// Common divisors are used in determinization to compute transition weights.
// In the simplest case, it is the same as semiring Plus, but other choices
// permit more efficient determinization when the output contains strings.

// The default common divisor uses the semiring Plus.
namespace internal {
template <class Arc, class Relation>
class RelationDeterminizeFilter;
}  // namespace internal
struct PairArc;

template <class W>
struct DefaultCommonDivisor {
 public:
  using Weight = W;

  Weight operator()(const Weight& w1, const Weight& w2) const {
    return Plus(w1, w2);
  }
};

// The label common divisor for a (left) string semiring selects a single
// letter common prefix or the empty string. This is used in the
// determinization of output strings so that at most a single letter will
// appear in the output of a transtion.
template <typename Label, StringType S>
struct LabelCommonDivisor {
 public:
  using Weight = StringWeight<Label, S>;

  Weight operator()(const Weight& w1, const Weight& w2) const {
    typename Weight::Iterator iter1(w1);
    typename Weight::Iterator iter2(w2);
    if (!(StringWeight<Label, S>::Properties() & kLeftSemiring)) {
      FSTERROR() << "LabelCommonDivisor: Weight needs to be left semiring";
      return Weight::NoWeight();
    } else if (w1.Size() == 0 || w2.Size() == 0) {
      return Weight::One();
    } else if (w1 == Weight::Zero()) {
      return Weight(iter2.Value());
    } else if (w2 == Weight::Zero()) {
      return Weight(iter1.Value());
    } else if (iter1.Value() == iter2.Value()) {
      return Weight(iter1.Value());
    } else {
      return Weight::One();
    }
  }
};

// The gallic common divisor uses the label common divisor on the string
// component and the common divisor on the weight component, which defaults to
// the default common divisor.
template <class Label, class W, GallicType G,
          class CommonDivisor = DefaultCommonDivisor<W>>
class GallicCommonDivisor {
 public:
  using Weight = GallicWeight<Label, W, G>;

  Weight operator()(const Weight& w1, const Weight& w2) const {
    return Weight(label_common_divisor_(w1.Value1(), w2.Value1()),
                  weight_common_divisor_(w1.Value2(), w2.Value2()));
  }

 private:
  LabelCommonDivisor<Label, GallicStringType(G)> label_common_divisor_;
  CommonDivisor weight_common_divisor_;
};

// Specialization for general GALLIC weight.
template <class Label, class W, class CommonDivisor>
class GallicCommonDivisor<Label, W, GALLIC, CommonDivisor> {
 public:
  using Weight = GallicWeight<Label, W, GALLIC>;
  using GRWeight = GallicWeight<Label, W, GALLIC_RESTRICT>;
  using Iterator =
      UnionWeightIterator<GRWeight, GallicUnionWeightOptions<Label, W>>;

  Weight operator()(const Weight& w1, const Weight& w2) const {
    auto weight = GRWeight::Zero();
    for (Iterator iter(w1); !iter.Done(); iter.Next()) {
      weight = common_divisor_(weight, iter.Value());
    }
    for (Iterator iter(w2); !iter.Done(); iter.Next()) {
      weight = common_divisor_(weight, iter.Value());
    }
    return weight == GRWeight::Zero() ? Weight::Zero() : Weight(weight);
  }

 private:
  GallicCommonDivisor<Label, W, GALLIC_RESTRICT, CommonDivisor> common_divisor_;
};

namespace internal {

// Represents an element in a subset
template <class Arc>
struct DeterminizeElement {
  using StateId = typename Arc::StateId;
  using Weight = typename Arc::Weight;

  DeterminizeElement(StateId s, Weight weight)
      : state_id(s), weight(std::move(weight)) {}

  inline bool operator==(const DeterminizeElement& element) const {
    return state_id == element.state_id && weight == element.weight;
  }

  inline bool operator!=(const DeterminizeElement& element) const {
    return !(*this == element);
  }

  inline bool operator<(const DeterminizeElement<Arc>& element) const {
    return state_id < element.state_id;
  }

  StateId state_id;  // Input state ID.
  Weight weight;     // Residual weight.
};

// Represents a weighted subset and determinization filter state
template <typename A, typename FilterState>
struct DeterminizeStateTuple {
  using Arc = A;
  using Element = DeterminizeElement<Arc>;
  using Subset = std::forward_list<Element>;

  DeterminizeStateTuple() : filter_state(FilterState::NoState()) {}

  inline bool operator==(const DeterminizeStateTuple& tuple) const {
    return (tuple.filter_state == filter_state) && (tuple.subset == subset);
  }

  inline bool operator!=(const DeterminizeStateTuple& tuple) const {
    return (tuple.filter_state != filter_state) || (tuple.subset != subset);
  }

  Subset subset;
  FilterState filter_state;
};

// Proto-transition for determinization.
template <class StateTuple>
struct DeterminizeArc {
  using Arc = typename StateTuple::Arc;
  using Label = typename Arc::Label;
  using Weight = typename Arc::Weight;

  DeterminizeArc() = default;

  explicit DeterminizeArc(const Arc& arc)
      : label(arc.ilabel),
        dest_tuple(absl::make_unique_for_overwrite<StateTuple>()) {}

  Label label = kNoLabel;          // Arc label.
  Weight weight = Weight::Zero();  // Arc weight.
  std::unique_ptr<StateTuple>
      dest_tuple;  // Destination subset and filter state.
};

}  // namespace internal

// Determinization filters are used to compute destination state tuples based
// on the source tuple, transition, and destination element or on similar
// super-final transition information. The filter operates on a map between a
// label and the corresponding destination state tuples. It must define the map
// type LabelMap. The default filter is used for weighted determinization.
// A determinize filter for implementing weighted determinization.
template <class Arc>
class DefaultDeterminizeFilter {
 public:
  using Label = typename Arc::Label;
  using StateId = typename Arc::StateId;
  using Weight = typename Arc::Weight;

  using FilterState = CharFilterState;
  using Element = internal::DeterminizeElement<Arc>;
  using StateTuple = internal::DeterminizeStateTuple<Arc, FilterState>;
  using LabelMap = std::map<Label, internal::DeterminizeArc<StateTuple>>;

  // This is needed e.g. to go into the gallic domain for transducers.
  template <class A>
  struct rebind {
    using Other = DefaultDeterminizeFilter<A>;
  };

  explicit DefaultDeterminizeFilter(const Fst<Arc>& fst) {}

  // This is needed (e.g.) to go into the gallic domain for transducers.
  template <class Filter>
  DefaultDeterminizeFilter(const Fst<Arc>& fst,
                           std::unique_ptr<Filter> filter) {}

  // Copy constructor; the FST can be passed if it has been deep-copied.
  DefaultDeterminizeFilter(const DefaultDeterminizeFilter& filter,
                           const Fst<Arc>* fst = nullptr) {}

  FilterState Start() const { return FilterState(0); }

  // Does no work.
  void SetState(StateId s, const StateTuple& tuple) {}

  // Filters transition, possibly modifying label map. Returns true if arc is
  // added to the label map.
  bool FilterArc(const Arc& arc, const Element& src_element,
                 Element&& dest_element, LabelMap* label_map) const {
    // Adds element to unique state tuple for arc label.
    auto& det_arc = (*label_map)[arc.ilabel];
    if (det_arc.label == kNoLabel) {
      det_arc = internal::DeterminizeArc<StateTuple>(arc);
      det_arc.dest_tuple->filter_state = FilterState(0);
    }
    det_arc.dest_tuple->subset.push_front(std::move(dest_element));
    return true;
  }

  // Filters super-final transition, returning new final weight.
  Weight FilterFinal(Weight weight, const Element& element) const {
    return weight;
  }

  static uint64_t Properties(uint64_t props) { return props; }
};

// Determinization state table interface:
//
// template <class Arc, class FilterState>
// class DeterminizeStateTable {
//  public:
//   using StateId = typename Arc::StateId;
//   using StateTuple = internal::DeterminizeStateTuple<Arc, FilterState>;
//
//   // Required sub-class. This is needed (e.g.) to go into the gallic domain.
//   template <class B, class G>
//   struct rebind {
//     using Other = DeterminizeStateTable<B, G>;
//   }
//
//   // Required constuctor.
//   DeterminizeStateTable();
//
//   // Required copy constructor that does not copy state.
//   DeterminizeStateTable(const DeterminizeStateTable<Arc, FilterState>
//   &table);
//
//   // Looks up state ID by state tuple; if it doesn't exist, then adds it.
//   // FindState takes ownership of the state tuple argument so that it
//   // doesn't have to copy it if it creates a new state.
//   StateId FindState(std::unique_ptr<StateTuple> tuple);
//
//   // Looks up state tuple by ID.
//   const StateTuple *Tuple(StateId id) const;
// };

// The default determinization state table based on the compact hash bi-table.
template <class Arc, class FilterState>
class DefaultDeterminizeStateTable {
 public:
  using Label = typename Arc::Label;
  using StateId = typename Arc::StateId;
  using Weight = typename Arc::Weight;

  using StateTuple = internal::DeterminizeStateTuple<Arc, FilterState>;
  using Element = typename StateTuple::Element;
  using Subset = typename StateTuple::Subset;

  template <class B, class G>
  struct rebind {
    using Other = DefaultDeterminizeStateTable<B, G>;
  };

  explicit DefaultDeterminizeStateTable(size_t table_size = 0)
      : table_size_(table_size), tuples_(table_size_) {}

  DefaultDeterminizeStateTable(const DefaultDeterminizeStateTable& table)
      : table_size_(table.table_size_), tuples_(table_size_) {}

  ~DefaultDeterminizeStateTable() {
    for (StateId s = 0; s < tuples_.Size(); ++s) delete tuples_.FindEntry(s);
  }

  // Finds the state corresponding to a state tuple. Only creates a new state if
  // the tuple is not found. FindState takes ownership of the tuple argument so
  // that it doesn't have to copy it if it creates a new state.
  StateId FindState(std::unique_ptr<StateTuple> tuple) {
    StateTuple* raw_tuple = tuple.release();
    const StateId ns = tuples_.Size();
    // TODO: Make CompactHashBiTable support move semantics so we
    // can store a `std::unique_ptr` in `tuples_`.
    const auto s = tuples_.FindId(raw_tuple);
    if (s != ns) delete raw_tuple;  // Tuple found.
    return s;
  }

  const StateTuple* Tuple(StateId s) { return tuples_.FindEntry(s); }

 private:
  // Comparison object for StateTuples.
  class StateTupleEqual {
   public:
    bool operator()(const StateTuple* tuple1, const StateTuple* tuple2) const {
      return *tuple1 == *tuple2;
    }
  };

  // Hash function for StateTuples.
  class StateTupleKey {
   public:
    size_t operator()(const StateTuple* tuple) const {
      size_t h = tuple->filter_state.Hash();
      for (auto& element : tuple->subset) {
        const size_t h1 = element.state_id;
        static constexpr auto lshift = 5;
        static constexpr auto rshift = CHAR_BIT * sizeof(size_t) - 5;
        h ^= h << 1 ^ h1 << lshift ^ h1 >> rshift ^ element.weight.Hash();
      }
      return h;
    }
  };

  size_t table_size_;
  CompactHashBiTable<StateId, StateTuple*, StateTupleKey, StateTupleEqual,
                     HS_STL>
      tuples_;

  DefaultDeterminizeStateTable& operator=(const DefaultDeterminizeStateTable&) =
      delete;
};

// Determinization type.
enum DeterminizeType {
  // Input transducer is known to be functional (or error).
  DETERMINIZE_FUNCTIONAL,  // Input transducer is functional (error if not).
  // Input transducer is not known to be functional.
  DETERMINIZE_NONFUNCTIONAL,
  // Input transducer is not known to be functional but only keep the min of
  // of ambiguous outputs.
  DETERMINIZE_DISAMBIGUATE
};

// Options for finite-state transducer determinization templated on the arc
// type, common divisor, the determinization filter and the state table.
// DeterminizeFst takes ownership of the determinization filter and state table,
// if provided.
template <class Arc,
          class CommonDivisor = DefaultCommonDivisor<typename Arc::Weight>,
          class Filter = DefaultDeterminizeFilter<Arc>,
          class StateTable =
              DefaultDeterminizeStateTable<Arc, typename Filter::FilterState>>
struct DeterminizeFstOptions : public CacheOptions {
  using Label = typename Arc::Label;

  float delta;                // Quantization delta for subset weights.
  Label subsequential_label;  // Label used for residual final output
                              // when producing subsequential transducers.
  DeterminizeType type;       // Determinization type.
  bool increment_subsequential_label;  // When creating several subsequential
                                       // arcs at a given state, make their
                                       // label distinct by incrementing.
  Filter* filter;                      // Determinization filter;
                                       // DeterminizeFst takes ownership.
  StateTable* state_table;             // Determinization state table;
                                       // DeterminizeFst takes ownership.

  explicit DeterminizeFstOptions(const CacheOptions& opts, float delta = kDelta,
                                 Label subsequential_label = 0,
                                 DeterminizeType type = DETERMINIZE_FUNCTIONAL,
                                 bool increment_subsequential_label = false,
                                 Filter* filter = nullptr,
                                 StateTable* state_table = nullptr)
      : CacheOptions(opts),
        delta(delta),
        subsequential_label(subsequential_label),
        type(type),
        increment_subsequential_label(increment_subsequential_label),
        filter(filter),
        state_table(state_table) {}

  explicit DeterminizeFstOptions(float delta = kDelta,
                                 Label subsequential_label = 0,
                                 DeterminizeType type = DETERMINIZE_FUNCTIONAL,
                                 bool increment_subsequential_label = false,
                                 Filter* filter = nullptr,
                                 StateTable* state_table = nullptr)
      : delta(delta),
        subsequential_label(subsequential_label),
        type(type),
        increment_subsequential_label(increment_subsequential_label),
        filter(filter),
        state_table(state_table) {}
};

namespace internal {

// Implementation of delayed DeterminizeFst. This base class is
// common to the variants that implement acceptor and transducer
// determinization.
template <class Arc>
class DeterminizeFstImplBase : public CacheImpl<Arc> {
 public:
  using Label = typename Arc::Label;
  using StateId = typename Arc::StateId;
  using Weight = typename Arc::Weight;

  using Store = DefaultCacheStore<Arc>;
  using State = typename Store::State;

  using FstImpl<Arc>::SetType;
  using FstImpl<Arc>::SetProperties;
  using FstImpl<Arc>::Properties;
  using FstImpl<Arc>::SetInputSymbols;
  using FstImpl<Arc>::SetOutputSymbols;

  using CacheBaseImpl<CacheState<Arc>>::HasStart;
  using CacheBaseImpl<CacheState<Arc>>::HasFinal;
  using CacheBaseImpl<CacheState<Arc>>::HasArcs;
  using CacheBaseImpl<CacheState<Arc>>::SetFinal;
  using CacheBaseImpl<CacheState<Arc>>::SetStart;

  template <class CommonDivisor, class Filter, class StateTable>
  DeterminizeFstImplBase(
      const Fst<Arc>& fst,
      const DeterminizeFstOptions<Arc, CommonDivisor, Filter, StateTable>& opts)
      : CacheImpl<Arc>(opts), fst_(fst.Copy()) {
    SetType("determinize");
    const auto iprops = fst.Properties(kFstProperties, false);
    const auto dprops =
        DeterminizeProperties(iprops, opts.subsequential_label != 0,
                              opts.type == DETERMINIZE_NONFUNCTIONAL
                                  ? opts.increment_subsequential_label
                                  : true);
    SetProperties(Filter::Properties(dprops), kCopyProperties);
    SetInputSymbols(fst.InputSymbols());
    SetOutputSymbols(fst.OutputSymbols());
  }

  DeterminizeFstImplBase(const DeterminizeFstImplBase& impl)
      : CacheImpl<Arc>(impl), fst_(impl.fst_->Copy(true)) {
    SetType("determinize");
    SetProperties(impl.Properties(), kCopyProperties);
    SetInputSymbols(impl.InputSymbols());
    SetOutputSymbols(impl.OutputSymbols());
  }

  virtual DeterminizeFstImplBase* Copy() const = 0;

  StateId Start() {
    if (!HasStart()) {
      const auto start = ComputeStart();
      if (start != kNoStateId) SetStart(start);
    }
    return CacheImpl<Arc>::Start();
  }

  Weight Final(StateId s) {
    if (!HasFinal(s)) SetFinal(s, ComputeFinal(s));
    return CacheImpl<Arc>::Final(s);
  }

  virtual void Expand(StateId s) = 0;

  size_t NumArcs(StateId s) {
    if (!HasArcs(s)) Expand(s);
    return CacheImpl<Arc>::NumArcs(s);
  }

  size_t NumInputEpsilons(StateId s) {
    if (!HasArcs(s)) Expand(s);
    return CacheImpl<Arc>::NumInputEpsilons(s);
  }

  size_t NumOutputEpsilons(StateId s) {
    if (!HasArcs(s)) Expand(s);
    return CacheImpl<Arc>::NumOutputEpsilons(s);
  }

  void InitArcIterator(StateId s, ArcIteratorData<Arc>* data) {
    if (!HasArcs(s)) Expand(s);
    CacheImpl<Arc>::InitArcIterator(s, data);
  }

  virtual StateId ComputeStart() = 0;

  virtual Weight ComputeFinal(StateId s) = 0;

  const Fst<Arc>& GetFst() const { return *fst_; }

 private:
  std::unique_ptr<const Fst<Arc>> fst_;  // Input FST.
};

// Compact state table for DeterminizeFsaImpl when using
// DefaultDeterminizeFilter and DefaultDeterminizeStateTable with no
// caller-supplied state_table (so every state has the constant filter state
// CharFilterState(0)). Stores all accepted subsets contiguously in `elements_`
// and indexes state IDs in `set_` using cached per-state hashes, avoiding
// StateTuple and std::forward_list allocations on both hits and misses as well
// as subset re-walks when `set_` grows.
template <class Arc>
class FlatDeterminizeStateTable {
 public:
  using StateId = typename Arc::StateId;
  using Element = DeterminizeElement<Arc>;

  FlatDeterminizeStateTable() : set_(0, StateHash(this), StateEqual(this)) {}

  // Neither copyable nor movable: the hash and equality functors of `set_`
  // point back to this object. Copies of DeterminizeFsaImpl start with an
  // empty table instead.
  FlatDeterminizeStateTable(const FlatDeterminizeStateTable&) = delete;
  FlatDeterminizeStateTable& operator=(const FlatDeterminizeStateTable&) =
      delete;

  // Returns the subset of state `s`, ordered as it was passed to
  // `FindState()`. The view points into `elements_` and is invalidated if
  // `FindState()` inserts a new state.
  absl::Span<const Element> Subset(StateId s) const
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    const StateEntry& entry = states_[s];
    return absl::MakeConstSpan(elements_.data() + entry.offset, entry.size);
  }

  // Looks up the state ID for the subset `items`, which must be sorted by
  // strictly ascending state ID (hashing and comparison depend on the order).
  // On a hit, returns the existing state ID and leaves `items` unchanged. On a
  // miss, assigns the next sequential state ID (0, 1, ...) and moves each
  // item's weight into `elements_`. `Item` may be `Element` or a transition
  // struct with `.nextstate` and `.weight` fields.
  template <class Item>
  StateId FindState(absl::Span<Item> items) {
    const absl::Span<const Item> const_items = absl::MakeConstSpan(items);
    const size_t hash = HashItems(const_items);
    const Candidate<Item> candidate{const_items, hash};
    if (const auto it = set_.find(candidate); it != set_.end()) {
      return *it;
    }
    const StateId new_id = static_cast<StateId>(states_.size());
    const size_t offset = elements_.size();
    for (Item& item : items) {
      elements_.emplace_back(GetStateId(item), std::move(item.weight));
    }
    states_.push_back(StateEntry{offset, items.size(), hash});
    set_.insert(new_id);
    return new_id;
  }

 private:
  // Location in `elements_` and cached hash of a stored subset. Holds an
  // offset rather than a pointer, so it stays valid when `elements_` grows.
  struct StateEntry {
    size_t offset;
    size_t size;
    size_t hash;
  };

  // Lookup key for a subset that may not be stored yet, with its hash.
  template <class Item>
  struct Candidate {
    absl::Span<const Item> items;
    size_t hash;
  };

  template <class Item>
  static StateId GetStateId(const Item& item) {
    if constexpr (std::is_same_v<std::remove_cv_t<Item>, Element>) {
      return item.state_id;
    } else {
      return item.nextstate;
    }
  }

  template <class Item>
  static size_t HashItems(absl::Span<const Item> items) {
    uint64_t h = items.size();
    for (const Item& item : items) {
      const uint64_t s = static_cast<uint64_t>(GetStateId(item));
      const uint64_t w = static_cast<uint64_t>(item.weight.Hash());
      // Multiplicative mix avoids StateTupleKey's shift-XOR cancellation
      // (where {s, 2*s} collides with {s} if the weights hash to zero, as 0.0f
      // does) and populates the upper 7 bits used by absl::flat_hash_set's H2
      // control byte.
      uint64_t k = (s * uint64_t{0x9e3779b97f4a7c15}) ^ w;
      k ^= k >> 30;
      h = (h ^ k) * uint64_t{0xbf58476d1ce4e5b9};
    }
    h ^= h >> 27;
    h *= uint64_t{0x94d049bb133111eb};
    h ^= h >> 31;
    return static_cast<size_t>(h);
  }

  struct StateHash {
    using is_transparent = void;

    explicit StateHash(const FlatDeterminizeStateTable* owner) : owner(owner) {}

    size_t operator()(StateId id) const { return owner->states_[id].hash; }

    template <class Item>
    size_t operator()(const Candidate<Item>& c) const {
      return c.hash;
    }

    const FlatDeterminizeStateTable* owner;
  };

  struct StateEqual {
    using is_transparent = void;

    explicit StateEqual(const FlatDeterminizeStateTable* owner)
        : owner(owner) {}

    // Only called when FindState() inserts a new ID, which it does after
    // finding no stored subset equal to the new one, so distinct IDs never
    // compare equal.
    bool operator()(StateId a, StateId b) const { return a == b; }

    template <class Item>
    bool operator()(StateId id, const Candidate<Item>& c) const {
      const StateEntry& entry = owner->states_[id];
      // Comparing the cached hashes skips most element-wise comparisons and
      // keeps equality consistent with the hash, as absl::flat_hash_set
      // requires, for weights that compare equal but hash differently (e.g.
      // 0.0f and -0.0f).
      if (entry.size != c.items.size() || entry.hash != c.hash) {
        return false;
      }
      const absl::Span<const Element> stored = owner->Subset(id);
      return std::equal(stored.begin(), stored.end(), c.items.begin(),
                        [](const Element& elem, const Item& item) {
                          return elem.state_id == GetStateId(item) &&
                                 elem.weight == item.weight;
                        });
    }

    template <class Item>
    bool operator()(const Candidate<Item>& c, StateId id) const {
      return (*this)(id, c);
    }

    const FlatDeterminizeStateTable* owner;
  };

  std::vector<Element> elements_;   // Elements of all subsets, back to back.
  std::vector<StateEntry> states_;  // Indexed by state ID.
  // IDs of all stored states, hashed and compared by subset content.
  absl::flat_hash_set<StateId, StateHash, StateEqual> set_;
};

// Implementation of delayed determinization for weighted acceptors.
template <class Arc, class CommonDivisor, class Filter, class StateTable>
class DeterminizeFsaImpl : public DeterminizeFstImplBase<Arc> {
 public:
  using Label = typename Arc::Label;
  using StateId = typename Arc::StateId;
  using Weight = typename Arc::Weight;

  using FilterState = typename Filter::FilterState;
  using StateTuple = internal::DeterminizeStateTuple<Arc, FilterState>;
  using Element = typename StateTuple::Element;
  using LabelMap = typename Filter::LabelMap;

  using FstImpl<Arc>::SetProperties;
  using DeterminizeFstImplBase<Arc>::GetFst;
  using DeterminizeFstImplBase<Arc>::SetArcs;

  DeterminizeFsaImpl(
      const Fst<Arc>& fst, const std::vector<Weight>* in_dist,
      std::vector<Weight>* out_dist,
      const DeterminizeFstOptions<Arc, CommonDivisor, Filter, StateTable>& opts)
      : DeterminizeFstImplBase<Arc>(fst, opts),
        delta_(opts.delta),
        in_dist_(in_dist),
        out_dist_(out_dist),
        filter_(opts.filter ? opts.filter : new Filter(fst)),
        state_table_(opts.state_table) {
    if (!state_table_ && !kIsDefaultFilterAndStateTable) {
      state_table_ = std::make_unique<StateTable>();
    }
    if (!fst.Properties(kAcceptor, true)) {
      FSTERROR() << "DeterminizeFst: Argument not an acceptor";
      SetProperties(kError, kError);
    }
    if (!(Weight::Properties() & kLeftSemiring)) {
      FSTERROR() << "DeterminizeFst: Weight must be left distributive: "
                 << Weight::Type();
      SetProperties(kError, kError);
    }
    if (out_dist_) out_dist_->clear();
  }

  DeterminizeFsaImpl(const DeterminizeFsaImpl& impl)
      : DeterminizeFstImplBase<Arc>(impl),
        delta_(impl.delta_),
        in_dist_(nullptr),
        out_dist_(nullptr),
        filter_(new Filter(*impl.filter_, &GetFst())),
        state_table_(impl.state_table_ ? new StateTable(*impl.state_table_)
                                       : nullptr) {
    if (impl.out_dist_) {
      FSTERROR() << "DeterminizeFsaImpl: Cannot copy with out_dist vector";
      SetProperties(kError, kError);
    }
  }

  DeterminizeFsaImpl* Copy() const override {
    return new DeterminizeFsaImpl(*this);
  }

  uint64_t Properties() const override { return Properties(kFstProperties); }

  // Sets error if found, and returns other FST impl properties.
  uint64_t Properties(uint64_t mask) const override {
    if ((mask & kError) && (GetFst().Properties(kError, false))) {
      SetProperties(kError, kError);
    }
    return FstImpl<Arc>::Properties(mask);
  }

  StateId ComputeStart() override {
    const auto s = GetFst().Start();
    if (s == kNoStateId) return kNoStateId;
    if constexpr (kIsDefaultFilterAndStateTable) {
      if (state_table_ == nullptr) {
        Element start_element(s, Weight::One());
        return FindDefaultState(absl::MakeSpan(&start_element, 1));
      }
    }
    auto tuple = absl::make_unique_for_overwrite<StateTuple>();
    tuple->subset.emplace_front(s, Weight::One());
    tuple->filter_state = filter_->Start();
    return FindState(std::move(tuple));
  }

  Weight ComputeFinal(StateId s) override {
    if constexpr (kIsDefaultFilterAndStateTable) {
      if (state_table_ == nullptr) {
        // Same computation as below, where DefaultDeterminizeFilter's
        // SetState() does nothing and FilterFinal() returns its argument. The
        // elements of Subset(s) are in the same order as `tuple->subset`.
        auto final_weight = Weight::Zero();
        for (const Element& element : flat_state_table_.Subset(s)) {
          final_weight =
              Plus(final_weight,
                   Times(element.weight, GetFst().Final(element.state_id)));
          if (!final_weight.Member()) SetProperties(kError, kError);
        }
        return final_weight;
      }
    }
    const auto* tuple = state_table_->Tuple(s);
    filter_->SetState(s, *tuple);
    auto final_weight = Weight::Zero();
    for (const auto& element : tuple->subset) {
      final_weight =
          Plus(final_weight,
               Times(element.weight, GetFst().Final(element.state_id)));
      final_weight = filter_->FilterFinal(final_weight, element);
      if (!final_weight.Member()) SetProperties(kError, kError);
    }
    return final_weight;
  }

  StateId FindState(std::unique_ptr<StateTuple> tuple) {
    const auto& subset = tuple->subset;
    const auto s = state_table_->FindState(std::move(tuple));
    if (in_dist_ && out_dist_->size() <= s) {
      out_dist_->push_back(ComputeDistance(subset));
    }
    return s;
  }

  // Computes distance from a state to the final states in the DFA given the
  // distances in the NFA.
  template <class SubsetRange>
  Weight ComputeDistance(const SubsetRange& subset) {
    auto outd = Weight::Zero();
    for (const auto& element : subset) {
      const auto ind =
          (element.state_id < in_dist_->size() ? (*in_dist_)[element.state_id]
                                               : Weight::Zero());
      outd = Plus(outd, Times(element.weight, ind));
    }
    return outd;
  }

  // Computes the outgoing transitions from a state, creating new destination
  // states as needed.
  void Expand(StateId s) override {
    if constexpr (kIsDefaultFilterAndStateTable) {
      if (state_table_ == nullptr) {
        ExpandDefault(s);
        return;
      }
    }
    LabelMap label_map;
    GetLabelMap(s, &label_map);
    for (auto& [unused_label, arc] : label_map) {
      AddArc(s, std::move(arc));
    }
    SetArcs(s);
  }

 private:
  // True if `Filter` and `StateTable` are exactly the default types. All
  // states then share the filter state CharFilterState(0), and the filter's
  // SetState() and FilterFinal() have no effect. If, in addition, the caller
  // passed no state table (`state_table_` is null), states are kept in
  // `flat_state_table_` and expanded by ExpandDefault(); a caller-supplied
  // table is still populated through the generic path.
  static constexpr bool kIsDefaultFilterAndStateTable =
      std::is_same_v<Filter, DefaultDeterminizeFilter<Arc>> &&
      std::is_same_v<StateTable,
                     DefaultDeterminizeStateTable<Arc, FilterState>>;

  using DetArc = internal::DeterminizeArc<StateTuple>;

  // An outgoing transition of the source subset, as gathered by
  // ExpandDefault().
  struct RawTransition {
    RawTransition(uint32_t sequence, StateId nextstate, Weight weight,
                  Label ilabel)
        : sequence(sequence),
          nextstate(nextstate),
          weight(std::move(weight)),
          ilabel(ilabel) {}

    // Position in gathering order. Wraps after 2^32 transitions from a single
    // source subset, which only affects the order in which transitions beyond
    // that point are combined.
    uint32_t sequence;
    StateId nextstate;
    // Residual weight of the source element times the arc weight.
    Weight weight;
    Label ilabel;
  };

  // Counterpart of FindState() for `flat_state_table_`. The output distance of
  // a new state is computed from the stored subset, since the table moves the
  // weights out of `subset` when it adds a state.
  template <class Item>
  StateId FindDefaultState(absl::Span<Item> subset) {
    const StateId s = flat_state_table_.FindState(subset);
    if (in_dist_ && out_dist_->size() <= static_cast<size_t>(s)) {
      out_dist_->push_back(ComputeDistance(flat_state_table_.Subset(s)));
    }
    return s;
  }

  // Fast path for DefaultDeterminizeFilter and DefaultDeterminizeStateTable
  // when no caller-supplied state_table is passed. Gathers the outgoing
  // transitions of the source subset into a reusable flat buffer, sorts them,
  // normalizes each run of equal labels in place, and looks up destination
  // subsets directly in `flat_state_table_` without building a LabelMap or
  // allocating StateTuple / std::forward_list nodes. Gives the same arcs,
  // weights, and destination subsets as the generic path, except for arcs
  // labeled kNoLabel: the generic path uses that label to mark an uninitialized
  // DeterminizeArc and so keeps only the last such transition, whereas this
  // path keeps all of them.
  void ExpandDefault(StateId s) {
    scratch_transitions_.clear();
    // Subset(s) is only read here: the FindDefaultState() calls below may
    // invalidate it.
    for (const Element& src_element : flat_state_table_.Subset(s)) {
      for (ArcIterator<Fst<Arc>> aiter(GetFst(), src_element.state_id);
           !aiter.Done(); aiter.Next()) {
        const auto& arc = aiter.Value();
        scratch_transitions_.emplace_back(
            static_cast<uint32_t>(scratch_transitions_.size()), arc.nextstate,
            Times(src_element.weight, arc.weight), arc.ilabel);
      }
    }
    if (scratch_transitions_.size() > 1) {
      // Orders transitions by label, then by destination state, and breaks ties
      // by descending `sequence` (last gathered first). That matches the
      // generic path, where FilterArc() prepends each element and NormArc()
      // sorts stably by state; the combine order is observable because Plus()
      // is not associative for floating-point weights, and Plus() for
      // GALLIC_MIN returns its second argument on a tie. No two transitions
      // compare equal, so the result does not depend on how std::sort orders
      // equal elements.
      std::sort(scratch_transitions_.begin(), scratch_transitions_.end(),
                [](const RawTransition& a, const RawTransition& b) {
                  if (a.ilabel != b.ilabel) return a.ilabel < b.ilabel;
                  if (a.nextstate != b.nextstate) {
                    return a.nextstate < b.nextstate;
                  }
                  // Larger `sequence` was gathered later; place it first to
                  // match FilterArc()'s push_front() + stable sort().
                  return a.sequence > b.sequence;
                });
    }
    const auto scratch_end = scratch_transitions_.end();
    // Each iteration handles one run [run_begin, run_end) of transitions with
    // the same label. While the run is scanned, [run_begin, unique_end) holds
    // the destination subset built so far, in ascending `nextstate` order with
    // the weights of duplicate destinations summed; the slots in
    // [unique_end, run_end) are not read again.
    for (auto run_begin = scratch_transitions_.begin(), run_end = run_begin;
         run_begin != scratch_end; run_begin = run_end) {
      const Label label = run_begin->ilabel;
      // Starts from Zero() as NormArc() does: common_divisor_(Zero(), w) need
      // not equal w (LabelCommonDivisor returns only the first label of w).
      Weight arc_weight = common_divisor_(Weight::Zero(), run_begin->weight);
      auto unique_end = run_begin + 1;
      for (run_end = run_begin + 1;
           run_end != scratch_end && run_end->ilabel == label; ++run_end) {
        arc_weight = common_divisor_(arc_weight, run_end->weight);
        if (run_end->nextstate == (unique_end - 1)->nextstate) {
          Weight& prev_weight = (unique_end - 1)->weight;
          prev_weight = Plus(prev_weight, run_end->weight);
          if (!prev_weight.Member()) SetProperties(kError, kError);
        } else {
          if (unique_end != run_end) {
            *unique_end = std::move(*run_end);
          }
          ++unique_end;
        }
      }
      for (auto it = run_begin; it != unique_end; ++it) {
        Weight norm_weight = Divide(it->weight, arc_weight, DIVIDE_LEFT);
        it->weight = norm_weight.Quantize(delta_);
      }
      const absl::Span<RawTransition> dest_subset = absl::MakeSpan(
          &*run_begin, static_cast<size_t>(unique_end - run_begin));
      CacheImpl<Arc>::EmplaceArc(s, label, label, std::move(arc_weight),
                                 FindDefaultState(dest_subset));
    }
    // Release any heap-backed weights (e.g. GallicWeight) between calls while
    // preserving vector capacity.
    scratch_transitions_.clear();
    SetArcs(s);
  }

  // Constructs proto-determinization transition, including destination subset,
  // per label.
  void GetLabelMap(StateId s, LabelMap* label_map) {
    const auto* src_tuple = state_table_->Tuple(s);
    filter_->SetState(s, *src_tuple);
    for (const auto& src_element : src_tuple->subset) {
      for (ArcIterator<Fst<Arc>> aiter(GetFst(), src_element.state_id);
           !aiter.Done(); aiter.Next()) {
        const auto& arc = aiter.Value();
        Element dest_element(arc.nextstate,
                             Times(src_element.weight, arc.weight));
        filter_->FilterArc(arc, src_element, std::move(dest_element),
                           label_map);
      }
    }
    for (auto& [unused_label, arc] : *label_map) {
      NormArc(&arc);
    }
  }

  // Sorts subsets and removes duplicate elements, normalizing transition and
  // subset weights.
  void NormArc(DetArc* det_arc) {
    auto& dest_subset = det_arc->dest_tuple->subset;
    dest_subset.sort();
    auto piter = dest_subset.begin();
    for (auto diter = dest_subset.begin(); diter != dest_subset.end();) {
      auto& dest_element = *diter;
      auto& prev_element = *piter;
      // Computes arc weight.
      det_arc->weight = common_divisor_(det_arc->weight, dest_element.weight);
      if (piter != diter && dest_element.state_id == prev_element.state_id) {
        // Found duplicate state: sums state weight and deletes duplicate.
        prev_element.weight = Plus(prev_element.weight, dest_element.weight);
        if (!prev_element.weight.Member()) SetProperties(kError, kError);
        ++diter;
        dest_subset.erase_after(piter);
      } else {
        piter = diter;
        ++diter;
      }
    }
    // Divides out label weight from destination subset elements, quantizing to
    // ensure comparisons are effective.
    for (auto& dest_element : dest_subset) {
      dest_element.weight =
          Divide(dest_element.weight, det_arc->weight, DIVIDE_LEFT);
      dest_element.weight = dest_element.weight.Quantize(delta_);
    }
  }

  // Adds an arc from state S to the destination state associated with state
  // tuple in det_arc as created by GetLabelMap.
  void AddArc(StateId s, DetArc&& det_arc) {
    CacheImpl<Arc>::EmplaceArc(s, det_arc.label, det_arc.label,
                               std::move(det_arc.weight),
                               FindState(std::move(det_arc.dest_tuple)));
  }

  float delta_;                         // Quantization delta for weights.
  const std::vector<Weight>* in_dist_;  // Distance to final NFA states.
  std::vector<Weight>* out_dist_;       // Distance to final DFA states.

  static const CommonDivisor common_divisor_;
  std::unique_ptr<Filter> filter_;
  // Null when `kIsDefaultFilterAndStateTable` is true and no caller-supplied
  // `opts.state_table` was passed; `flat_state_table_` is used instead.
  std::unique_ptr<StateTable> state_table_;
  FlatDeterminizeStateTable<Arc> flat_state_table_;
  // Scratch buffer of ExpandDefault(). Empty between calls, but keeps its
  // capacity for the lifetime of this object.
  std::vector<RawTransition> scratch_transitions_;
};

template <class Arc, class CommonDivisor, class Filter, class StateTable>
const CommonDivisor DeterminizeFsaImpl<Arc, CommonDivisor, Filter,
                                       StateTable>::common_divisor_{};

// Implementation of delayed determinization for transducers. Transducer
// determinization is implemented by mapping the input to the Gallic semiring as
// an acceptor whose weights contain the output strings and using acceptor
// determinization above to determinize that acceptor.
template <class Arc, GallicType G, class CommonDivisor, class Filter,
          class StateTable>
class DeterminizeFstImpl : public DeterminizeFstImplBase<Arc> {
 public:
  using Label = typename Arc::Label;
  using StateId = typename Arc::StateId;
  using Weight = typename Arc::Weight;

  using ToMapper = ToGallicMapper<Arc, G>;
  using ToArc = typename ToMapper::ToArc;
  using ToFst = ArcMapFst<Arc, ToArc, ToMapper>;
  using FromMapper = FromGallicMapper<Arc, G>;
  using FromFst = ArcMapFst<ToArc, Arc, FromMapper>;

  using ToCommonDivisor = GallicCommonDivisor<Label, Weight, G, CommonDivisor>;
  using ToFilter = typename Filter::template rebind<ToArc>::Other;
  using ToFilterState = typename ToFilter::FilterState;
  using ToStateTable =
      typename StateTable::template rebind<ToArc, ToFilterState>::Other;
  using FactorIterator = GallicFactor<Label, Weight, G>;

  using FstImpl<Arc>::SetProperties;
  using DeterminizeFstImplBase<Arc>::GetFst;
  using CacheBaseImpl<CacheState<Arc>>::GetCacheGc;
  using CacheBaseImpl<CacheState<Arc>>::GetCacheLimit;

  DeterminizeFstImpl(
      const Fst<Arc>& fst,
      const DeterminizeFstOptions<Arc, CommonDivisor, Filter, StateTable>& opts)
      : DeterminizeFstImplBase<Arc>(fst, opts),
        delta_(opts.delta),
        subsequential_label_(opts.subsequential_label),
        increment_subsequential_label_(opts.increment_subsequential_label) {
    // Takes ownership of filter and state_table.
    std::unique_ptr<Filter> filter(opts.filter);
    std::unique_ptr<StateTable> state_table(opts.state_table);
    if (state_table) {
      FSTERROR() << "DeterminizeFst: "
                 << "A state table can not be passed with transducer input";
      SetProperties(kError, kError);
      return;
    }
    Init(GetFst(), std::move(filter));
  }

  DeterminizeFstImpl(const DeterminizeFstImpl& impl)
      : DeterminizeFstImplBase<Arc>(impl),
        delta_(impl.delta_),
        subsequential_label_(impl.subsequential_label_),
        increment_subsequential_label_(impl.increment_subsequential_label_),
        from_fst_(impl.from_fst_ ? impl.from_fst_->Copy(true) : nullptr) {}

  DeterminizeFstImpl* Copy() const override {
    return new DeterminizeFstImpl(*this);
  }

  uint64_t Properties() const override { return Properties(kFstProperties); }

  // Sets error if found, and returns other FST impl properties.
  uint64_t Properties(uint64_t mask) const override {
    if ((mask & kError) &&
        (GetFst().Properties(kError, false) ||
         (from_fst_ && from_fst_->Properties(kError, false)))) {
      SetProperties(kError, kError);
    }
    return FstImpl<Arc>::Properties(mask);
  }

  StateId ComputeStart() override {
    return from_fst_ ? from_fst_->Start() : kNoStateId;
  }

  Weight ComputeFinal(StateId s) override {
    return from_fst_ ? from_fst_->Final(s) : Weight::Zero();
  }

  void Expand(StateId s) override {
    if (from_fst_) {
      for (ArcIterator<FromFst> aiter(*from_fst_, s); !aiter.Done();
           aiter.Next()) {
        CacheImpl<Arc>::PushArc(s, aiter.Value());
      }
    }
    CacheImpl<Arc>::SetArcs(s);
  }

 private:
  // Initialization of transducer determinization implementation, which is
  // defined after DeterminizeFst since it calls it.
  void Init(const Fst<Arc>& fst, std::unique_ptr<Filter> filter);

  float delta_;
  Label subsequential_label_;
  bool increment_subsequential_label_;
  std::unique_ptr<FromFst> from_fst_;
};

}  // namespace internal

// Determinizes a weighted transducer. This version is a delayed
// FST. The result will be an equivalent FST that has the property
// that no state has two transitions with the same input label.
// For this algorithm, epsilon transitions are treated as regular
// symbols (cf. RmEpsilon).
//
// The transducer must be functional. The weights must be (weakly) left
// divisible (valid for TropicalWeight and LogWeight for instance) and be
// zero-sum-free if for all a, b: (Plus(a, b) == 0) => a = b = 0.
//
// Complexity:
//
//   Determinizable: exponential (polynomial in the size of the output).
//   Non-determinizable: does not terminate.
//
// The determinizable automata include all unweighted and all acyclic input.
//
// For more information, see:
//
// Mohri, M. 1997. Finite-state transducers in language and speech processing.
// Computational Linguistics 23(2): 269-311.
//
// This class attaches interface to implementation and handles reference
// counting, delegating most methods to ImplToFst.
template <class A>
class DeterminizeFst : public ImplToFst<internal::DeterminizeFstImplBase<A>> {
  using Base = ImplToFst<internal::DeterminizeFstImplBase<A>>;

 public:
  using Arc = A;
  using Label = typename Arc::Label;
  using StateId = typename Arc::StateId;
  using Weight = typename Arc::Weight;

  using Store = DefaultCacheStore<Arc>;
  using State = typename Store::State;
  using typename Base::Impl;

  friend class ArcIterator<DeterminizeFst<Arc>>;
  friend class StateIterator<DeterminizeFst<Arc>>;

  template <class B, GallicType G, class CommonDivisor, class Filter,
            class StateTable>
  friend class DeterminizeFstImpl;

  explicit DeterminizeFst(const Fst<A>& fst) : Base(CreateImpl(fst)) {}

  template <class CommonDivisor, class Filter, class StateTable>
  explicit DeterminizeFst(
      const Fst<Arc>& fst,
      const DeterminizeFstOptions<Arc, CommonDivisor, Filter, StateTable>&
          opts =
              DeterminizeFstOptions<Arc, CommonDivisor, Filter, StateTable>())
      : Base(CreateImpl(fst, opts)) {}

  // This acceptor-only version additionally computes the distance to final
  // states in the output if provided with those distances for the input; this
  // is useful for e.g., computing the k-shortest unique paths.
  template <class CommonDivisor, class Filter, class StateTable>
  DeterminizeFst(
      const Fst<Arc>& fst, const std::vector<Weight>* in_dist,
      std::vector<Weight>* out_dist,
      const DeterminizeFstOptions<Arc, CommonDivisor, Filter, StateTable>&
          opts =
              DeterminizeFstOptions<Arc, CommonDivisor, Filter, StateTable>())
      : Base(std::make_shared<internal::DeterminizeFsaImpl<Arc, CommonDivisor,
                                                           Filter, StateTable>>(
            fst, in_dist, out_dist, opts)) {
    if (!fst.Properties(kAcceptor, true)) {
      FSTERROR() << "DeterminizeFst: "
                 << "Distance to final states computed for acceptors only";
      GetMutableImpl()->SetProperties(kError, kError);
    }
  }

  // See Fst<>::Copy() for doc.
  DeterminizeFst(const DeterminizeFst& fst, bool safe = false)
      : Base(safe ? std::shared_ptr<Impl>(fst.GetImpl()->Copy())
                  : fst.GetSharedImpl()) {}

  // Get a copy of this DeterminizeFst. See Fst<>::Copy() for further doc.
  DeterminizeFst* Copy(bool safe = false) const override {
    return new DeterminizeFst(*this, safe);
  }

  inline void InitStateIterator(StateIteratorData<Arc>* data) const override;

  void InitArcIterator(StateId s, ArcIteratorData<Arc>* data) const override {
    GetMutableImpl()->InitArcIterator(s, data);
  }

 private:
  using Base::GetImpl;
  using Base::GetMutableImpl;

  static std::shared_ptr<Impl> CreateImpl(const Fst<Arc>& fst) {
    using D = DefaultCommonDivisor<Weight>;
    using F = DefaultDeterminizeFilter<Arc>;
    using T = DefaultDeterminizeStateTable<Arc, typename F::FilterState>;
    const DeterminizeFstOptions<Arc, D, F, T> opts;
    return CreateImpl(fst, opts);
  }

  template <class CommonDivisor, class Filter, class StateTable>
  static std::shared_ptr<Impl> CreateImpl(
      const Fst<Arc>& fst,
      const DeterminizeFstOptions<Arc, CommonDivisor, Filter, StateTable>&
          opts) {
    if (fst.Properties(kAcceptor, true)) {
      // Calls implementation for acceptors.
      return std::make_shared<
          internal::DeterminizeFsaImpl<Arc, CommonDivisor, Filter, StateTable>>(
          fst, nullptr, nullptr, opts);
    } else if (opts.type == DETERMINIZE_DISAMBIGUATE) {
      if constexpr (IsPath<Weight>::value) {
        // Calls disambiguating implementation for non-functional transducers.
        return std::make_shared<internal::DeterminizeFstImpl<
            Arc, GALLIC_MIN, CommonDivisor, Filter, StateTable>>(fst, opts);
      } else {
        FSTERROR() << "DeterminizeFst: Weight needs to have the path "
                   << "property to disambiguate output: " << Weight::Type();
        // Return an error Impl.
        const ConstFst<Arc> empty_fst;
        auto rv = std::make_shared<internal::DeterminizeFstImpl<
            Arc, GALLIC, CommonDivisor, Filter, StateTable>>(empty_fst, opts);
        rv->SetProperties(kError, kError);
        return rv;
      }
    } else if (opts.type == DETERMINIZE_FUNCTIONAL) {
      // Calls implementation for functional transducers.
      return std::make_shared<internal::DeterminizeFstImpl<
          Arc, GALLIC_RESTRICT, CommonDivisor, Filter, StateTable>>(fst, opts);
    } else {  // opts.type == DETERMINIZE_NONFUNCTIONAL
      // Calls implementation for non functional transducers;
      return std::make_shared<internal::DeterminizeFstImpl<
          Arc, GALLIC, CommonDivisor, Filter, StateTable>>(fst, opts);
    }
  }

  DeterminizeFst& operator=(const DeterminizeFst&) = delete;
};

namespace internal {

// Initialization of transducer determinization implementation, which is defined
// after DeterminizeFst since it calls it.
template <class A, GallicType G, class D, class F, class T>
void DeterminizeFstImpl<A, G, D, F, T>::Init(const Fst<A>& fst,
                                             std::unique_ptr<F> filter) {
  // Mapper to an acceptor.
  const ToFst to_fst(fst);
  auto* to_filter = filter ? new ToFilter(to_fst, std::move(filter)) : nullptr;
  // This recursive call terminates since it is to a (non-recursive)
  // different constructor.
  const CacheOptions copts(GetCacheGc(), GetCacheLimit());
  const DeterminizeFstOptions<ToArc, ToCommonDivisor, ToFilter, ToStateTable>
      dopts(copts, delta_, 0, DETERMINIZE_FUNCTIONAL, false, to_filter);
  // Uses acceptor-only constructor to avoid template recursion.
  const DeterminizeFst<ToArc> det_fsa(to_fst, nullptr, nullptr, dopts);
  // Mapper back to transducer.
  const FactorWeightOptions<ToArc> fopts(
      CacheOptions(true, 0), delta_, kFactorFinalWeights, subsequential_label_,
      subsequential_label_, increment_subsequential_label_,
      increment_subsequential_label_);
  const FactorWeightFst<ToArc, FactorIterator> factored_fst(det_fsa, fopts);
  from_fst_ =
      std::make_unique<FromFst>(factored_fst, FromMapper(subsequential_label_));
}

}  // namespace internal

// Specialization for DeterminizeFst.
template <class Arc>
class StateIterator<DeterminizeFst<Arc>>
    : public CacheStateIterator<DeterminizeFst<Arc>> {
 public:
  explicit StateIterator(const DeterminizeFst<Arc>& fst)
      : CacheStateIterator<DeterminizeFst<Arc>>(fst, fst.GetMutableImpl()) {}
};

// Specialization for DeterminizeFst.
template <class Arc>
class ArcIterator<DeterminizeFst<Arc>>
    : public CacheArcIterator<DeterminizeFst<Arc>> {
 public:
  using StateId = typename Arc::StateId;

  ArcIterator(const DeterminizeFst<Arc>& fst, StateId s)
      : CacheArcIterator<DeterminizeFst<Arc>>(fst.GetMutableImpl(), s) {
    if (!fst.GetImpl()->HasArcs(s)) fst.GetMutableImpl()->Expand(s);
  }
};

template <class Arc>
inline void DeterminizeFst<Arc>::InitStateIterator(
    StateIteratorData<Arc>* data) const {
  data->base = std::make_unique<StateIterator<DeterminizeFst<Arc>>>(*this);
}

// Useful aliases when using StdArc.
using StdDeterminizeFst = DeterminizeFst<StdArc>;

template <class Arc>
struct DeterminizeOptions {
  using Label = typename Arc::Label;
  using StateId = typename Arc::StateId;
  using Weight = typename Arc::Weight;

  float delta;                // Quantization delta for subset weights; also
                              // bounds pruning weight comparison tolerance.
  Weight weight_threshold;    // Pruning weight threshold.
  StateId state_threshold;    // Pruning state threshold.
  Label subsequential_label;  // Label used for residual final output.
  DeterminizeType type;
  bool increment_subsequential_label;  // When creating several subsequential
                                       // arcs at a given state, make their
                                       // label distinct by incrementation?

  explicit DeterminizeOptions(float delta = kDelta,
                              Weight weight_threshold = Weight::Zero(),
                              StateId state_threshold = kNoStateId,
                              Label subsequential_label = 0,
                              DeterminizeType type = DETERMINIZE_FUNCTIONAL,
                              bool increment_subsequential_label = false)
      : delta(delta),
        weight_threshold(std::move(weight_threshold)),
        state_threshold(state_threshold),
        subsequential_label(subsequential_label),
        type(type),
        increment_subsequential_label(increment_subsequential_label) {}
};

// Determinizes a weighted transducer. This version writes the
// determinized Fst to an output MutableFst. The result will be an
// equivalent FST that has the property that no state has two
// transitions with the same input label. For this algorithm, epsilon
// transitions are treated as regular symbols (cf. RmEpsilon).
//
// The transducer must be functional. The weights must be (weakly)
// left divisible (valid for TropicalWeight and LogWeight).
//
// Complexity:
//
//   Determinizable: exponential (polynomial in the size of the output)
//   Non-determinizable: does not terminate
//
// The determinizable automata include all unweighted and all acyclic input.
template <class Arc>
void Determinize(
    const Fst<Arc>& ifst, MutableFst<Arc>* ofst,
    const DeterminizeOptions<Arc>& opts = DeterminizeOptions<Arc>()) {
  using Weight = typename Arc::Weight;
  DeterminizeFstOptions<Arc> nopts;
  nopts.delta = opts.delta;
  nopts.subsequential_label = opts.subsequential_label;
  nopts.type = opts.type;
  nopts.increment_subsequential_label = opts.increment_subsequential_label;
  nopts.gc_limit = 0;  // Caches only the last state for fastest copy.
  if (opts.weight_threshold != Weight::Zero() ||
      opts.state_threshold != kNoStateId) {
    if constexpr (IsPath<Weight>::value) {
      if (ifst.Properties(kAcceptor, false)) {
        std::vector<Weight> idistance;
        std::vector<Weight> odistance;
        // Cap relaxation delta at kShortestDelta so coarse subset-quantization
        // opts.delta does not accumulate multi-hop error in idistance.
        ShortestDistance(ifst, &idistance, true,
                         std::min(opts.delta, kShortestDelta));
        DeterminizeFst<Arc> dfst(ifst, &idistance, &odistance, nopts);
        PruneOptions<Arc, AnyArcFilter<Arc>> popts(
            opts.weight_threshold, opts.state_threshold, AnyArcFilter<Arc>(),
            &odistance, opts.delta);
        Prune(dfst, ofst, popts);
      } else {
        *ofst = DeterminizeFst<Arc>(ifst, nopts);
        // *ofst already has quantized weights; cap at kDelta so coarse
        // opts.delta does not widen the beam or coarsen ShortestDistance.
        Prune(ofst, opts.weight_threshold, opts.state_threshold,
              std::min(opts.delta, kDelta));
      }
    } else {
      FSTERROR() << "Determinize: Weight needs to have the path "
                 << "property to use pruning options: " << Weight::Type();
      ofst->SetProperties(kError, kError);
    }
  } else {
    *ofst = DeterminizeFst<Arc>(ifst, nopts);
  }
}

}  // namespace fst

#endif  // OPENFST_LIB_DETERMINIZE_H_
