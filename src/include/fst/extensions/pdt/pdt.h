// pdt.h

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
// Author: riley@google.com (Michael Riley)
//
// \file
// Common classes for PDT expansion/traversal.

#ifndef FST_EXTENSIONS_PDT_PDT_H__
#define FST_EXTENSIONS_PDT_PDT_H__

#include <tr1/unordered_map>
using std::tr1::unordered_map;
using std::tr1::unordered_multimap;
#include <map>
#include <set>

#include <fst/state-table.h>
#include <fst/fst.h>

namespace fst {

// Provides bijection between parenthesis stacks and signed integral
// stack IDs. Each stack ID is unique to each distinct stack.  The
// open-close parenthesis label pairs are passed in 'parens'.
template <typename K, typename L>
class PdtStack {
 public:
  typedef K StackId;
  typedef L Label;

  // The stacks are stored in a tree. The nodes are stored in vector
  // 'nodes_'. Each node represents the top of some stack and is
  // ID'ed by its position in the vector. Its parent node represents
  // the stack with the top 'popped' and its children are stored in
  // 'child_map_' accessed by stack_id and label. The paren_id is
  // the position in 'parens' of the parenthesis for that node.
  struct StackNode {
    StackId parent_id;
    size_t paren_id;

    StackNode(StackId p, size_t i) : parent_id(p), paren_id(i) {}
  };

  PdtStack(const vector<pair<Label, Label> > &parens)
      : parens_(parens), min_paren_(kNoLabel), max_paren_(kNoLabel) {
    for (size_t i = 0; i < parens.size(); ++i) {
      const pair<Label, Label>  &p = parens[i];
      paren_map_[p.first] = i;
      paren_map_[p.second] = i;

      if (min_paren_ == kNoLabel || p.first < min_paren_)
        min_paren_ = p.first;
      if (p.second < min_paren_)
        min_paren_ = p.second;

      if (max_paren_ == kNoLabel || p.first > max_paren_)
        max_paren_ = p.first;
      if (p.second > max_paren_)
        max_paren_ = p.second;
    }
    nodes_.push_back(StackNode(-1, -1));  // Tree root.
  }

  // Returns stack ID given the current stack ID (0 if empty) and
  // label read. 'Pushes' onto a stack if the label is an open
  // parenthesis, returning the new stack ID. 'Pops' the stack if the
  // label is a close parenthesis that matches the top of the stack,
  // returning the parent stack ID. Returns -1 if label is an
  // unmatched close parenthesis. Otherwise, returns the current stack
  // ID.
  StackId Find(StackId stack_id, Label label) {
    if (min_paren_ == kNoLabel || label < min_paren_ || label > max_paren_)
      return stack_id;                       // Non-paren.

    typename unordered_map<Label, size_t>::const_iterator pit
        = paren_map_.find(label);
    if (pit == paren_map_.end())             // Non-paren.
      return stack_id;
    ssize_t paren_id = pit->second;

    if (label == parens_[paren_id].first) {  // Open paren.
      StackId &child_id = child_map_[make_pair(stack_id, label)];
      if (child_id == 0) {                   // Child not found, push label.
        child_id = nodes_.size();
        nodes_.push_back(StackNode(stack_id, paren_id));
      }
      return child_id;
    }

    const StackNode &node = nodes_[stack_id];
    if (paren_id == node.paren_id)           // Matching close paren.
      return node.parent_id;

    return -1;                               // Non-matching close paren.
  }

 private:
  struct ChildHash {
    size_t operator()(const pair<StackId, Label> &p) const {
      return p.first + p.second * kPrime;
    }
  };

  static const size_t kPrime;

  vector<pair<Label, Label> > parens_;
  vector<StackNode> nodes_;
  unordered_map<Label, size_t> paren_map_;
  unordered_map<pair<StackId, Label>,
           StackId, ChildHash> child_map_;   // Child of stack node wrt label
  Label min_paren_;                          // For faster paren. check
  Label max_paren_;                          // For faster paren. check
};

template <typename T, typename L>
const size_t PdtStack<T, L>::kPrime = 7853;


// State tuple for PDT expansion
template <typename S, typename K>
struct PdtStateTuple {
  typedef S StateId;
  typedef K StackId;

  StateId state_id;
  StackId stack_id;

  PdtStateTuple()
      : state_id(kNoStateId), stack_id(-1) {}

  PdtStateTuple(StateId fs, StackId ss)
      : state_id(fs), stack_id(ss) {}
};

// Equality of PDT state tuples.
template <typename S, typename K>
inline bool operator==(const PdtStateTuple<S, K>& x,
                       const PdtStateTuple<S, K>& y) {
  if (&x == &y)
    return true;
  return x.state_id == y.state_id && x.stack_id == y.stack_id;
}


// Hash function object for PDT state tuples
template <class T>
class PdtStateHash {
 public:
  size_t operator()(const T &tuple) const {
    return tuple.state_id + tuple.stack_id * kPrime;
  }

 private:
  static const size_t kPrime;
};

template <typename T>
const size_t PdtStateHash<T>::kPrime = 7853;


// Tuple to PDT state bijection.
template <class S, class K>
class PdtStateTable
    : public CompactHashStateTable<PdtStateTuple<S, K>,
                                   PdtStateHash<PdtStateTuple<S, K> > > {
 public:
  typedef S StateId;
  typedef K StackId;

  PdtStateTable() {}

  PdtStateTable(const PdtStateTable<S, K> &table) {}

 private:
  void operator=(const PdtStateTable<S, K> &table);  // disallow
};


// Store balancing parenthesis data for a PDT.
template <class Arc>
class PdtBalanceData {
 public:
  typedef typename Arc::StateId StateId;
  typedef typename Arc::Label Label;
  typedef ssize_t StateNodeId;
  typedef PdtStateTuple<StateId, StateNodeId> StateNode;

  // Open paren key
  struct ParenKey {
    Label paren_id;            // ID of open paren
    StateId state_id;          // destination state of open paren

    ParenKey() : paren_id(kNoLabel), state_id(kNoStateId) {}

    ParenKey(Label p, StateId s) : paren_id(p), state_id(s) {}

    bool operator==(const ParenKey &p) const {
      if (&p == this)
        return true;
      return p.paren_id == this->paren_id && p.state_id == this->state_id;
    }

    bool operator!=(const ParenKey &p) const { return !(p == *this); }
  };

  // Hash for paren multimap
  struct ParenHash {
    size_t operator()(const ParenKey &p) const {
      return p.paren_id + p.state_id * kPrime0;
    }
  };

  // Hash set for open parens
  typedef unordered_set<ParenKey, ParenHash> OpenParenSet;

  // Maps from open paren destination state to parenthesis ID.
  typedef unordered_multimap<StateId, Label> OpenParenMap;

  // Maps from open paren key to source states of matching close parens
  typedef unordered_multimap<ParenKey, StateId, ParenHash> CloseParenMap;

  // Maps from open paren key to state node ID
  typedef unordered_map<ParenKey, StateNodeId, ParenHash> StateNodeMap;

  ~PdtBalanceData() {
    VLOG(1) << "# of balanced close paren states: " << node_map_.size();
  }

  void Clear() {
    open_paren_map_.clear();
    close_paren_map_.clear();
  }

  // Adds an open parenthesis with destination state 'open_dest'.
  void OpenInsert(Label paren_id, StateId open_dest) {
    ParenKey key(paren_id, open_dest);
    if (!open_paren_set_.count(key)) {
      open_paren_set_.insert(key);
      open_paren_map_.insert(make_pair(open_dest, paren_id));
    }
  }

  // Adds a matching closing parenthesis with source state
  // 'close_source' that balances an open_parenthesis with destination
  // state 'open_dest' if OpenInsert() previously called
  // (o.w. CloseInsert() does nothing).
  void CloseInsert(Label paren_id, StateId open_dest, StateId close_source) {
    ParenKey key(paren_id, open_dest);
    if (open_paren_set_.count(key))
      close_paren_map_.insert(make_pair(key, close_source));
  }

  // Find close paren source states matching an open parenthesis.
  // Methods that follow, iterate through those matching states.
  // Should be called only after FinishInsert(open_dest).
  bool Find(Label paren_id, StateId open_dest) {
    ParenKey close_key(paren_id, open_dest);
    StateNodeId node_id = node_map_[close_key];
    if (node_id == kNoStateNodeId) {
      node_.state_id = kNoStateId;
      return false;
    } else {
      node_ = node_table_.Tuple(node_id);
      return true;
    }
  }

  bool Done() const { return node_.state_id == kNoStateId; }

  StateId Value() const { return node_.state_id; }

  void Next() {
    if (node_.stack_id == kNoStateNodeId)
      node_.state_id = kNoStateId;
    else
      node_ = node_table_.Tuple(node_.stack_id);
  }

  // Call when all open and close parenthesis insertions wrt open
  // parentheses entering 'open_dest' are finished. Must be called
  // before Find(open_dest). Stores close paren source states in a tree.
  void FinishInsert(StateId open_dest) {
    vector<StateId> close_sources;
    for (typename OpenParenMap::iterator oit = open_paren_map_.find(open_dest);
         oit != open_paren_map_.end() && oit->first == open_dest;) {
      Label paren_id = oit->second;
      close_sources.clear();
      ParenKey okey(paren_id, open_dest);
      open_paren_set_.erase(open_paren_set_.find(okey));
      for (typename CloseParenMap::iterator cit = close_paren_map_.find(okey);
           cit != close_paren_map_.end() && cit->first == okey;) {
        close_sources.push_back(cit->second);
        close_paren_map_.erase(cit++);
      }
      sort(close_sources.begin(), close_sources.end());
      typename vector<StateId>::iterator unique_end =
          unique(close_sources.begin(), close_sources.end());
      StateNodeId node_id = kNoStateNodeId;
      for (typename vector<StateId>::iterator sit = close_sources.begin();
           sit != unique_end;
           ++sit) {
        StateNode node(*sit, node_id);
        node_id = node_table_.FindState(node);
      }
      node_map_[okey] = node_id;
      open_paren_map_.erase(oit++);
    }
  }

 private:
  static const size_t kPrime0;
  static const StateNodeId kNoStateNodeId;

  OpenParenSet open_paren_set_;                      // open par.at dest?

  OpenParenMap open_paren_map_;                      // open parens per state
  ParenKey open_dest_;                               // cur open dest. state
  typename OpenParenMap::const_iterator open_iter_;  // cur open parens/state

  CloseParenMap close_paren_map_;                    // close states/open
                                                     //  paren and state

  StateNodeMap node_map_;                            // paren, state to node ID
  PdtStateTable<StateId, StateNodeId> node_table_;
  StateNode node_;                                   // current state node

};

template<class Arc>
const size_t PdtBalanceData<Arc>::kPrime0 = 7853;

template<class Arc> const typename PdtBalanceData<Arc>::StateNodeId
PdtBalanceData<Arc>::kNoStateNodeId = -1;


// // Find balancing parentheses in a PDT. The PDT is assumed to
// // be 'strongly regular' (i.e. is expandable as an FST).
// template <class Arc>
// class PdtBalance {
//  public:
//   typedef typename Arc::StateId StateId;

//   PdtBalance(const Fst<Arc> &fst) {}

//   const PdtBalanceData<Arc> Data() { return data_; }

//  private:
//   PdtBalanceData<Arc> data_;
// };

}  // namespace fst

#endif  // FST_EXTENSIONS_PDT_PDT_H__
