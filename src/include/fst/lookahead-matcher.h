// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.
//
// Classes to add lookahead to FST matchers, useful for improving composition
// efficiency with certain inputs.

#ifndef FST_LIB_LOOKAHEAD_MATCHER_H_
#define FST_LIB_LOOKAHEAD_MATCHER_H_

#include <memory>
#include <string>
#include <vector>

#include <fst/add-on.h>
#include <fst/const-fst.h>
#include <fst/fst.h>
#include <fst/label-reachable.h>
#include <fst/matcher.h>


DECLARE_string(save_relabel_ipairs);
DECLARE_string(save_relabel_opairs);

namespace fst {

// Lookahead matches extend the interface of Matcher (see matcher.h) with the
// following additional methods:
//
// template <class FST>
// class LookAheadMatcher {
//  public:
//   using Arc = typename FST::Arc;
//   using Label = typename Arc::Label;
//   using StateId = typename Arc::StateId;
//   using Weight = typename Arc::Weight;
//
//  // Required constructors.
//  LookAheadMatcher(const FST &fst, MatchType match_type);
//   // If safe=true, the copy is thread-safe (except the lookahead FST is
//   // preserved). See Fst<>::Cop() for further doc.
//  LookAheadMatcher(const LookAheadMatcher &matcher, bool safe = false);
//
//  // Below are methods for looking ahead for a match to a label and more
//  // generally, to a rational set. Each returns false if there is definitely
//  // not a match and returns true if there possibly is a match.

//  // Can the label be read from the current matcher state after possibly
//  // following epsilon transitions?
//  bool LookAheadLabel(Label label) const;
//
//  // The following methods allow looking ahead for an arbitrary rational set
//  // of strings, specified by an FST and a state from which to begin the
//  matching. If the lookahead FST is a transducer, this looks on the side
//  // different from the matcher's match_type (cf. composition).
//
//  // Are there paths from a state in the lookahead FST that can be read from
//  // the curent matcher state?
//  bool LookAheadFst(const Fst<Arc> &fst, StateId state);
//
//  // Gives an estimate of the combined weight of the paths in the lookahead
//  // and matcher FSTs for the last call to LookAheadFst; a trivial
//  // implementation returns Weight::One(). Non-trivial implementations are
//  // useful for weight-pushing in composition.
//  Weight LookAheadWeight() const;
//
//  // Is there is a single non-epsilon arc found in the lookahead FST that
//  // begins the path (after possibly following any epsilons) in the last call
//  // to LookAheadFst? If so, return true and copy it to the arc argument;
//  // otherwise, return false. A trivial implementation returns false.
//  // Non-trivial implementations are useful for label-pushing in composition.
//  bool LookAheadPrefix(Arc *arc);
//
//  // Optionally pre-specifies the lookahead FST that will be passed to
//  // LookAheadFst() for possible precomputation. If copy is true, then the FST
//  argument is a copy of the FST used in the previous call to this method (to
//  avoid unnecessary updates).
//  void InitLookAheadFst(const Fst<Arc> &fst, bool copy = false);
// };

// LOOK-AHEAD FLAGS (see also kMatcherFlags in matcher.h):
//
// Matcher is a lookahead matcher when 'match_type' is MATCH_INPUT.
constexpr uint32 kInputLookAheadMatcher = 0x00000010;

// Matcher is a lookahead matcher when 'match_type' is MATCH_OUTPUT.
constexpr uint32 kOutputLookAheadMatcher = 0x00000020;

// A non-trivial implementation of LookAheadWeight() method defined and
// should be used?
constexpr uint32 kLookAheadWeight = 0x00000040;

// A non-trivial implementation of LookAheadPrefix() method defined and
// should be used?
constexpr uint32 kLookAheadPrefix = 0x00000080;

// Look-ahead of matcher FST non-epsilon arcs?
constexpr uint32 kLookAheadNonEpsilons = 0x00000100;

// Look-ahead of matcher FST epsilon arcs?
constexpr uint32 kLookAheadEpsilons = 0x00000200;

// Ignore epsilon paths for the lookahead prefix? Note this gives
// correct results in composition only with an appropriate composition
// filter since it depends on the filter blocking the ignored paths.
constexpr uint32 kLookAheadNonEpsilonPrefix = 0x00000400;

// For LabelLookAheadMatcher, save relabeling data to file
constexpr uint32 kLookAheadKeepRelabelData = 0x00000800;

// Flags used for lookahead matchers.
constexpr uint32 kLookAheadFlags = 0x00000ff0;

// LookAhead Matcher interface, templated on the Arc definition; used
// for lookahead matcher specializations that are returned by the
// InitMatcher() Fst method.
template <class Arc>
class LookAheadMatcherBase : public MatcherBase<Arc> {
 public:
  using Label = typename Arc::Label;
  using StateId = typename Arc::StateId;
  using Weight = typename Arc::Weight;

  LookAheadMatcherBase()
      : weight_(Weight::One()),
        prefix_arc_(kNoLabel, kNoLabel, Weight::One(), kNoStateId) {}

  ~LookAheadMatcherBase() override {}

  bool LookAheadLabel(Label label) const { return LookAheadLabel_(label); }

  bool LookAheadFst(const Fst<Arc> &fst, StateId state) {
    return LookAheadFst_(fst, state);
  }

  Weight LookAheadWeight() const { return weight_; }

  bool LookAheadPrefix(Arc *arc) const {
    if (prefix_arc_.nextstate != kNoStateId) {
      *arc = prefix_arc_;
      return true;
    } else {
      return false;
    }
  }

  virtual void InitLookAheadFst(const Fst<Arc> &fst, bool copy = false) = 0;

 protected:
  void SetLookAheadWeight(const Weight &w) { weight_ = w; }

  void SetLookAheadPrefix(const Arc &arc) { prefix_arc_ = arc; }

  void ClearLookAheadPrefix() { prefix_arc_.nextstate = kNoStateId; }

 private:
  virtual bool LookAheadLabel_(Label label) const = 0;

  // This must set l.a. weight and prefix if non-trivial.
  virtual bool LookAheadFst_(const Fst<Arc> &fst, StateId state) = 0;

  Weight weight_;                             // Look-ahead weight.
  Arc prefix_arc_;                            // Look-ahead prefix arc
};

// Don't really lookahead, just declare future looks good regardless.
template <class M>
class TrivialLookAheadMatcher
    : public LookAheadMatcherBase<typename M::FST::Arc> {
 public:
  using FST = typename M::FST;
  using Arc = typename FST::Arc;
  using Label = typename Arc::Label;
  using StateId = typename Arc::StateId;
  using Weight = typename Arc::Weight;

  TrivialLookAheadMatcher(const FST &fst, MatchType match_type)
      : matcher_(fst, match_type) {}

  TrivialLookAheadMatcher(const TrivialLookAheadMatcher<M> &lmatcher,
                          bool safe = false)
      : matcher_(lmatcher.matcher_, safe) {}

  TrivialLookAheadMatcher<M> *Copy(bool safe = false) const override {
    return new TrivialLookAheadMatcher<M>(*this, safe);
  }

  MatchType Type(bool test) const override { return matcher_.Type(test); }

  void SetState(StateId state) { return matcher_.SetState(state); }

  bool Find(Label label) { return matcher_.Find(label); }

  bool Done() const { return matcher_.Done(); }

  const Arc &Value() const { return matcher_.Value(); }

  void Next() { matcher_.Next(); }

  Weight Final(StateId state) const { return matcher_.Final(state); }

  ssize_t Priority(StateId state) { return matcher_.Priority(state); }

  const FST &GetFst() const override { return matcher_.GetFst(); }

  uint64 Properties(uint64 props) const override {
    return matcher_.Properties(props);
  }

  uint32 Flags() const override {
    return matcher_.Flags() | kInputLookAheadMatcher | kOutputLookAheadMatcher;
  }

  // Look-ahead methods.
  bool LookAheadLabel(Label label) const { return true; }

  bool LookAheadFst(const Fst<Arc> &fst, StateId state) { return true; }

  Weight LookAheadWeight() const { return Weight::One(); }

  bool LookAheadPrefix(Arc *arc) const { return false; }

  void InitLookAheadFst(const Fst<Arc> &fst, bool copy = false) override {}

 private:
  // This allows base class virtual access to non-virtual derived-class members
  // of the same name. It makes the derived class more efficient to use but
  // unsafe to further derive.
  void SetState_(StateId state) override { SetState(state); }

  bool Find_(Label label) override { return Find(label); }

  bool Done_() const override { return Done(); }

  const Arc &Value_() const override { return Value(); }

  void Next_() override { Next(); }

  Weight Final_(StateId state) const override { return Final(state); }

  ssize_t Priority_(StateId state) override { return Priority(state); }

  bool LookAheadLabel_(Label l) const override { return LookAheadLabel(l); }

  bool LookAheadFst_(const Fst<Arc> &fst, StateId state) override {
    return LookAheadFst(fst, state);
  }

  Weight LookAheadWeight_() const { return LookAheadWeight(); }

  bool LookAheadPrefix_(Arc *arc) const { return LookAheadPrefix(arc); }

  M matcher_;
};

// Look-ahead of one transition. Template argument F accepts flags to
// control behavior.
template <class M, uint32 F = kLookAheadNonEpsilons | kLookAheadEpsilons |
                              kLookAheadWeight | kLookAheadPrefix>
class ArcLookAheadMatcher : public LookAheadMatcherBase<typename M::FST::Arc> {
 public:
  using FST = typename M::FST;
  using Arc = typename FST::Arc;
  using Label = typename Arc::Label;
  using StateId = typename Arc::StateId;
  using Weight = typename Arc::Weight;
  using MatcherData = NullAddOn;

  using LookAheadMatcherBase<Arc>::LookAheadWeight;
  using LookAheadMatcherBase<Arc>::SetLookAheadPrefix;
  using LookAheadMatcherBase<Arc>::SetLookAheadWeight;
  using LookAheadMatcherBase<Arc>::ClearLookAheadPrefix;

  ArcLookAheadMatcher(
      const FST &fst, MatchType match_type,
      std::shared_ptr<MatcherData> data = std::shared_ptr<MatcherData>())
      : matcher_(fst, match_type),
        fst_(matcher_.GetFst()),
        lfst_(nullptr),
        state_(kNoStateId) {}

  ArcLookAheadMatcher(const ArcLookAheadMatcher<M, F> &lmatcher,
                      bool safe = false)
      : matcher_(lmatcher.matcher_, safe),
        fst_(matcher_.GetFst()),
        lfst_(lmatcher.lfst_),
        state_(kNoStateId) {}

  // General matcher methods.
  ArcLookAheadMatcher<M, F> *Copy(bool safe = false) const override {
    return new ArcLookAheadMatcher<M, F>(*this, safe);
  }

  MatchType Type(bool test) const override { return matcher_.Type(test); }

  void SetState(StateId state) {
    state_ = state;
    matcher_.SetState(state);
  }

  bool Find(Label label) { return matcher_.Find(label); }

  bool Done() const { return matcher_.Done(); }

  const Arc &Value() const { return matcher_.Value(); }

  void Next() { matcher_.Next(); }

  Weight Final(StateId state) const { return matcher_.Final(state); }

  ssize_t Priority(StateId state) { return matcher_.Priority(state); }

  const FST &GetFst() const override { return fst_; }

  uint64 Properties(uint64 props) const override {
    return matcher_.Properties(props);
  }

  uint32 Flags() const override {
    return matcher_.Flags() | kInputLookAheadMatcher | kOutputLookAheadMatcher |
           F;
  }

  const MatcherData *GetData() const { return nullptr; }

  std::shared_ptr<MatcherData> GetSharedData() const {
    return std::shared_ptr<MatcherData>();
  }

  // Look-ahead methods.
  bool LookAheadLabel(Label label) const { return matcher_.Find(label); }

  // Checks if there is a matching (possibly super-final) transition
  // at (state_, state).
  bool LookAheadFst(const Fst<Arc> &fst, StateId state);

  void InitLookAheadFst(const Fst<Arc> &fst, bool copy = false) override {
    lfst_ = &fst;
  }

 private:
  // This allows base class virtual access to non-virtual derived-
  // class members of the same name. It makes the derived class more
  // efficient to use but unsafe to further derive.
  void SetState_(StateId state) override { SetState(state); }

  bool Find_(Label label) override { return Find(label); }

  bool Done_() const override { return Done(); }

  const Arc &Value_() const override { return Value(); }

  void Next_() override { Next(); }

  Weight Final_(StateId state) const override { return Final(state); }

  ssize_t Priority_(StateId state) override { return Priority(state); }

  bool LookAheadLabel_(Label label) const override {
    return LookAheadLabel(label);
  }

  bool LookAheadFst_(const Fst<Arc> &fst, StateId state) override {
    return LookAheadFst(fst, state);
  }

  mutable M matcher_;
  const FST &fst_;        // Matcher FST.
  const Fst<Arc> *lfst_;  // Look-ahead FST.
  StateId state_;         // Matcher state.
};

template <class M, uint32 F>
bool ArcLookAheadMatcher<M, F>::LookAheadFst(const Fst<Arc> &fst,
                                             StateId state) {
  if (&fst != lfst_) InitLookAheadFst(fst);
  bool result = false;
  ssize_t nprefix = 0;
  if (F & kLookAheadWeight) SetLookAheadWeight(Weight::Zero());
  if (F & kLookAheadPrefix) ClearLookAheadPrefix();
  if (fst_.Final(state_) != Weight::Zero() &&
      lfst_->Final(state) != Weight::Zero()) {
    if (!(F & (kLookAheadWeight | kLookAheadPrefix))) return true;
    ++nprefix;
    if (F & kLookAheadWeight) {
      SetLookAheadWeight(Plus(LookAheadWeight(),
                              Times(fst_.Final(state_), lfst_->Final(state))));
    }
    result = true;
  }
  if (matcher_.Find(kNoLabel)) {
    if (!(F & (kLookAheadWeight | kLookAheadPrefix))) return true;
    ++nprefix;
    if (F & kLookAheadWeight) {
      for (; !matcher_.Done(); matcher_.Next()) {
        SetLookAheadWeight(Plus(LookAheadWeight(), matcher_.Value().weight));
      }
    }
    result = true;
  }
  for (ArcIterator<Fst<Arc>> aiter(*lfst_, state); !aiter.Done();
       aiter.Next()) {
    const auto &arc = aiter.Value();
    Label label = kNoLabel;
    switch (matcher_.Type(false)) {
      case MATCH_INPUT:
        label = arc.olabel;
        break;
      case MATCH_OUTPUT:
        label = arc.ilabel;
        break;
      default:
        FSTERROR() << "ArcLookAheadMatcher::LookAheadFst: Bad match type";
        return true;
    }
    if (label == 0) {
      if (!(F & (kLookAheadWeight | kLookAheadPrefix))) return true;
      if (!(F & kLookAheadNonEpsilonPrefix)) ++nprefix;
      if (F & kLookAheadWeight) {
        SetLookAheadWeight(Plus(LookAheadWeight(), arc.weight));
      }
      result = true;
    } else if (matcher_.Find(label)) {
      if (!(F & (kLookAheadWeight | kLookAheadPrefix))) return true;
      for (; !matcher_.Done(); matcher_.Next()) {
        ++nprefix;
        if (F & kLookAheadWeight) {
          SetLookAheadWeight(Plus(LookAheadWeight(),
                                  Times(arc.weight, matcher_.Value().weight)));
        }
        if ((F & kLookAheadPrefix) && nprefix == 1) SetLookAheadPrefix(arc);
      }
      result = true;
    }
  }
  if (F & kLookAheadPrefix) {
    if (nprefix == 1) {
      SetLookAheadWeight(Weight::One());  // Avoids double counting.
    } else {
      ClearLookAheadPrefix();
    }
  }
  return result;
}

// Template argument F accepts flags to control behavior. It must include
// precisely one of kInputLookAheadMatcher or kOutputLookAheadMatcher.
template <class M,
          uint32 F = kLookAheadEpsilons | kLookAheadWeight | kLookAheadPrefix |
                     kLookAheadNonEpsilonPrefix | kLookAheadKeepRelabelData,
          class Accumulator = DefaultAccumulator<typename M::Arc>,
          class Reachable = LabelReachable<typename M::Arc, Accumulator>>
class LabelLookAheadMatcher
    : public LookAheadMatcherBase<typename M::FST::Arc> {
 public:
  using FST = typename M::FST;
  using Arc = typename FST::Arc;
  using Label = typename Arc::Label;
  using StateId = typename Arc::StateId;
  using Weight = typename Arc::Weight;
  using MatcherData = typename Reachable::Data;

  using LookAheadMatcherBase<Arc>::LookAheadWeight;
  using LookAheadMatcherBase<Arc>::SetLookAheadPrefix;
  using LookAheadMatcherBase<Arc>::SetLookAheadWeight;
  using LookAheadMatcherBase<Arc>::ClearLookAheadPrefix;

  LabelLookAheadMatcher(
      const FST &fst, MatchType match_type,
      std::shared_ptr<MatcherData> data = std::shared_ptr<MatcherData>(),
      Accumulator *accumulator = nullptr)
      : matcher_(fst, match_type),
        lfst_(nullptr),
        state_(kNoStateId),
        error_(false) {
    if (!(F & (kInputLookAheadMatcher | kOutputLookAheadMatcher))) {
      FSTERROR() << "LabelLookaheadMatcher: Bad matcher flags: " << F;
      error_ = true;
    }
    const bool reach_input = match_type == MATCH_INPUT;
    if (data) {
      if (reach_input == data->ReachInput()) {
        label_reachable_.reset(new Reachable(data, accumulator));
      }
    } else if ((reach_input && (F & kInputLookAheadMatcher)) ||
               (!reach_input && (F & kOutputLookAheadMatcher))) {
      label_reachable_.reset(new Reachable(fst, reach_input, accumulator,
                                           F & kLookAheadKeepRelabelData));
    }
  }

  LabelLookAheadMatcher(
      const LabelLookAheadMatcher<M, F, Accumulator, Reachable> &lmatcher,
      bool safe = false)
      : matcher_(lmatcher.matcher_, safe),
        lfst_(lmatcher.lfst_),
        label_reachable_(lmatcher.label_reachable_
                             ? new Reachable(*lmatcher.label_reachable_, safe)
                             : nullptr),
        state_(kNoStateId),
        error_(lmatcher.error_) {}

  // General matcher methods.

  LabelLookAheadMatcher<M, F, Accumulator, Reachable> *Copy(
      bool safe = false) const override {
    return new LabelLookAheadMatcher<M, F, Accumulator, Reachable>(*this, safe);
  }

  MatchType Type(bool test) const override { return matcher_.Type(test); }

  void SetState(StateId state) {
    if (state == state_) return;
    state_ = state;
    match_set_state_ = false;
    reach_set_state_ = false;
  }

  bool Find(Label label) {
    if (!match_set_state_) {
      matcher_.SetState(state_);
      match_set_state_ = true;
    }
    return matcher_.Find(label);
  }

  bool Done() const { return matcher_.Done(); }

  const Arc &Value() const { return matcher_.Value(); }

  void Next() { matcher_.Next(); }

  ssize_t Priority(StateId state) { return matcher_.Priority(state); }

  Weight Final(StateId state) const { return matcher_.Final(state); }

  const FST &GetFst() const override { return matcher_.GetFst(); }

  uint64 Properties(uint64 inprops) const override {
    auto outprops = matcher_.Properties(inprops);
    if (error_ || (label_reachable_ && label_reachable_->Error())) {
      outprops |= kError;
    }
    return outprops;
  }

  uint32 Flags() const override {
    if (label_reachable_ && label_reachable_->GetData()->ReachInput()) {
      return matcher_.Flags() | F | kInputLookAheadMatcher;
    } else if (label_reachable_ && !label_reachable_->GetData()->ReachInput()) {
      return matcher_.Flags() | F | kOutputLookAheadMatcher;
    } else {
      return matcher_.Flags();
    }
  }

  const MatcherData *GetData() const {
    return label_reachable_ ? label_reachable_->GetData() : nullptr;
  };

  std::shared_ptr<MatcherData> GetSharedData() const {
    return label_reachable_ ? label_reachable_->GetSharedData()
                            : std::shared_ptr<MatcherData>();
  }

  bool LookAheadLabel(Label label) const {
    if (label == 0) return true;
    if (label_reachable_) {
      if (!reach_set_state_) {
        label_reachable_->SetState(state_);
        reach_set_state_ = true;
      }
      return label_reachable_->Reach(label);
    } else {
      return true;
    }
  }

  // Checks if there is a matching (possibly super-final) transition at
  // (state_, state).
  template <class LFST>
  bool LookAheadFst(const LFST &fst, StateId state);

  void InitLookAheadFst(const Fst<Arc> &fst, bool copy = false) override {
    lfst_ = &fst;
    if (label_reachable_) {
      const bool reach_input = Type(false) == MATCH_OUTPUT;
      label_reachable_->ReachInit(fst, reach_input, copy);
    }
  }

  template <class LFST>
  void InitLookAheadFst(const LFST &fst, bool copy = false) {
    lfst_ = static_cast<const Fst<Arc> *>(&fst);
    if (label_reachable_) {
      const bool reach_input = Type(false) == MATCH_OUTPUT;
      label_reachable_->ReachInit(fst, reach_input, copy);
    }
  }

 private:
  // This allows base class virtual access to non-virtual derived-
  // class members of the same name. It makes the derived class more
  // efficient to use but unsafe to further derive.
  void SetState_(StateId state) override { SetState(state); }

  bool Find_(Label label) override { return Find(label); }

  bool Done_() const override { return Done(); }

  const Arc &Value_() const override { return Value(); }

  void Next_() override { Next(); }

  Weight Final_(StateId state) const override { return Final(state); }

  ssize_t Priority_(StateId state) override { return Priority(state); }

  bool LookAheadLabel_(Label label) const override {
    return LookAheadLabel(label);
  }

  bool LookAheadFst_(const Fst<Arc> &fst, StateId state) override {
    return LookAheadFst(fst, state);
  }

  mutable M matcher_;
  const Fst<Arc> *lfst_;                        // Look-ahead FST.
  std::unique_ptr<Reachable> label_reachable_;  // Label reachability info.
  StateId state_;                               // Matcher state.
  bool match_set_state_;                        // matcher_.SetState called?
  mutable bool reach_set_state_;                // reachable_.SetState called?
  bool error_;
};

template <class M, uint32 F, class Accumulator, class Reachable>
template <class LFST>
inline bool LabelLookAheadMatcher<M, F, Accumulator, Reachable>::LookAheadFst(
    const LFST &fst, StateId state) {
  if (static_cast<const Fst<Arc> *>(&fst) != lfst_) InitLookAheadFst(fst);
  SetLookAheadWeight(Weight::One());
  ClearLookAheadPrefix();
  if (!label_reachable_) return true;
  label_reachable_->SetState(state_, state);
  reach_set_state_ = true;
  bool compute_weight = F & kLookAheadWeight;
  bool compute_prefix = F & kLookAheadPrefix;
  ArcIterator<LFST> aiter(fst, state);
  aiter.SetFlags(kArcNoCache, kArcNoCache);  // Makes caching optional.
  const bool reach_arc = label_reachable_->Reach(
      &aiter, 0, internal::NumArcs(*lfst_, state), compute_weight);
  const auto lfinal = internal::Final(*lfst_, state);
  const bool reach_final =
      lfinal != Weight::Zero() && label_reachable_->ReachFinal();
  if (reach_arc) {
    const auto begin = label_reachable_->ReachBegin();
    const auto end = label_reachable_->ReachEnd();
    if (compute_prefix && end - begin == 1 && !reach_final) {
      aiter.Seek(begin);
      SetLookAheadPrefix(aiter.Value());
      compute_weight = false;
    } else if (compute_weight) {
      SetLookAheadWeight(label_reachable_->ReachWeight());
    }
  }
  if (reach_final && compute_weight) {
    SetLookAheadWeight(reach_arc ? Plus(LookAheadWeight(), lfinal) : lfinal);
  }
  return reach_arc || reach_final;
}

// Label-lookahead relabeling class.
template <class Arc, class Data = LabelReachableData<typename Arc::Label>>
class LabelLookAheadRelabeler {
 public:
  using Label = typename Arc::Label;
  using Reachable = LabelReachable<Arc, DefaultAccumulator<Arc>, Data>;

  // Relabels matcher FST (initialization function object).
  template <typename Impl>
  explicit LabelLookAheadRelabeler(std::shared_ptr<Impl> *impl);

  // Relabels arbitrary FST. Class LFST should be a label-lookahead FST.
  template <class LFST>
  static void Relabel(MutableFst<Arc> *fst, const LFST &mfst,
                      bool relabel_input) {
    const auto *data = mfst.GetAddOn();
    Reachable reachable(data->First() ? data->SharedFirst()
                                      : data->SharedSecond());
    reachable.Relabel(fst, relabel_input);
  }

  // Returns relabeling pairs (cf. relabel.h::Relabel()). Class LFST should be a
  // label-lookahead FST. If avoid_collisions is true, extra pairs are added to
  // ensure no collisions when relabeling automata that have labels unseen here.
  template <class LFST>
  static void RelabelPairs(const LFST &mfst,
                           std::vector<std::pair<Label, Label>> *pairs,
                           bool avoid_collisions = false) {
    const auto *data = mfst.GetAddOn();
    Reachable reachable(data->First() ? data->SharedFirst()
                                      : data->SharedSecond());
    reachable.RelabelPairs(pairs, avoid_collisions);
  }
};

template <class Arc, class Data>
template <typename Impl>
inline LabelLookAheadRelabeler<Arc, Data>::LabelLookAheadRelabeler(
    std::shared_ptr<Impl> *impl) {
  Fst<Arc> &fst = (*impl)->GetFst();
  auto data = (*impl)->GetSharedAddOn();
  const auto name = (*impl)->Type();
  const bool is_mutable = fst.Properties(kMutable, false);
  std::unique_ptr<MutableFst<Arc>> mfst;
  if (is_mutable) {
    mfst.reset(static_cast<MutableFst<Arc> *>(&fst));
  } else {
    mfst.reset(new VectorFst<Arc>(fst));
  }
  if (data->First()) {  // reach_input.
    Reachable reachable(data->SharedFirst());
    reachable.Relabel(mfst.get(), true);
    if (!FLAGS_save_relabel_ipairs.empty()) {
      std::vector<std::pair<Label, Label>> pairs;
      reachable.RelabelPairs(&pairs, true);
      WriteLabelPairs(FLAGS_save_relabel_ipairs, pairs);
    }
  } else {
    Reachable reachable(data->SharedSecond());
    reachable.Relabel(mfst.get(), false);
    if (!FLAGS_save_relabel_opairs.empty()) {
      std::vector<std::pair<Label, Label>> pairs;
      reachable.RelabelPairs(&pairs, true);
      WriteLabelPairs(FLAGS_save_relabel_opairs, pairs);
    }
  }
  if (!is_mutable) {
    *impl = std::make_shared<Impl>(*mfst, name);
    (*impl)->SetAddOn(data);
  }
}

// Generic lookahead matcher, templated on the FST definition (a wrapper around
// a pointer to specific one).
template <class F>
class LookAheadMatcher {
 public:
  using FST = F;
  using Arc = typename FST::Arc;
  using Label = typename Arc::Label;
  using StateId = typename Arc::StateId;
  using Weight = typename Arc::Weight;
  using LBase = LookAheadMatcherBase<Arc>;

  LookAheadMatcher(const FST &fst, MatchType match_type)
      : base_(fst.InitMatcher(match_type)) {
    if (!base_) base_.reset(new SortedMatcher<FST>(fst, match_type));
    lookahead_ = false;
  }

  // Takes ownership of base.
  explicit LookAheadMatcher(MatcherBase<Arc> *base)
      : base_(base), lookahead_(false) {}

  LookAheadMatcher(const LookAheadMatcher<FST> &matcher, bool safe = false)
      : base_(matcher.base_->Copy(safe)) {
    lookahead_ = matcher.lookahead_;
  }

  LookAheadMatcher<FST> *Copy(bool safe = false) const {
    return new LookAheadMatcher<FST>(*this, safe);
  }

  MatchType Type(bool test) const { return base_->Type(test); }

  void SetState(StateId state) { base_->SetState(state); }

  bool Find(Label label) { return base_->Find(label); }

  bool Done() const { return base_->Done(); }

  const Arc &Value() const { return base_->Value(); }

  void Next() { base_->Next(); }

  Weight Final(StateId state) const { return base_->Final(state); }

  ssize_t Priority(StateId state) { return base_->Priority(state); }

  const FST &GetFst() const {
    return static_cast<const FST &>(base_->GetFst());
  }

  uint64 Properties(uint64 props) const { return base_->Properties(props); }

  uint32 Flags() const { return base_->Flags(); }

  bool LookAheadLabel(Label label) const {
    if (LookAheadCheck()) {
      return static_cast<LBase *>(base_.get())->LookAheadLabel(label);
    } else {
      return true;
    }
  }

  bool LookAheadFst(const Fst<Arc> &fst, StateId state) {
    if (LookAheadCheck()) {
      return static_cast<LBase *>(base_.get())->LookAheadFst(fst, state);
    } else {
      return true;
    }
  }

  Weight LookAheadWeight() const {
    if (LookAheadCheck()) {
      return static_cast<LBase *>(base_.get())->LookAheadWeight();
    } else {
      return Weight::One();
    }
  }

  bool LookAheadPrefix(Arc *arc) const {
    if (LookAheadCheck()) {
      return static_cast<LBase *>(base_.get())->LookAheadPrefix(arc);
    } else {
      return false;
    }
  }

  void InitLookAheadFst(const Fst<Arc> &fst, bool copy = false) {
    if (LookAheadCheck()) {
      static_cast<LBase *>(base_.get())->InitLookAheadFst(fst, copy);
    }
  }

 private:
  bool LookAheadCheck() const {
    if (!lookahead_) {
      lookahead_ =
          base_->Flags() & (kInputLookAheadMatcher | kOutputLookAheadMatcher);
      if (!lookahead_) {
        FSTERROR() << "LookAheadMatcher: No look-ahead matcher defined";
      }
    }
    return lookahead_;
  }

  std::unique_ptr<MatcherBase<Arc>> base_;
  mutable bool lookahead_;

  LookAheadMatcher &operator=(const LookAheadMatcher &) = delete;
};

}  // namespace fst

#endif  // FST_LIB_LOOKAHEAD_MATCHER_H_
