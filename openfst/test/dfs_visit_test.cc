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

#include <cstdint>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/arcfilter.h"
#include "openfst/lib/cc-visitors.h"
#include "openfst/lib/dfs-visit.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/properties.h"
#include "openfst/lib/vector-fst.h"

namespace fst {
namespace {

using ::testing::ElementsAre;

using Arc = StdArc;
using StateId = Arc::StateId;
using Weight = Arc::Weight;

class OrderRecordingVisitor {
 public:
  OrderRecordingVisitor() = default;

  void InitVisit(const Fst<Arc>&) const {}
  bool InitState(StateId s, StateId) {
    discovered_.push_back(s);
    return true;
  }
  constexpr bool TreeArc(StateId, const Arc&) const { return true; }
  constexpr bool BackArc(StateId, const Arc&) const { return true; }
  constexpr bool ForwardOrCrossArc(StateId, const Arc&) const { return true; }
  void FinishState(StateId s, StateId, const Arc*) { finished_.push_back(s); }
  void FinishVisit() const {}

  const std::vector<StateId>& Discovered() const { return discovered_; }
  const std::vector<StateId>& Finished() const { return finished_; }

 private:
  std::vector<StateId> discovered_;
  std::vector<StateId> finished_;
};

class AbortingVisitor {
 public:
  explicit AbortingVisitor(StateId abort_at)
      : abort_at_(abort_at), aborted_(false) {}

  void InitVisit(const Fst<Arc>&) const {}
  bool InitState(StateId s, StateId) {
    if (s == abort_at_) {
      aborted_ = true;
      return false;
    }
    return true;
  }
  constexpr bool TreeArc(StateId, const Arc&) const { return true; }
  constexpr bool BackArc(StateId, const Arc&) const { return true; }
  constexpr bool ForwardOrCrossArc(StateId, const Arc&) const { return true; }
  void FinishState(StateId, StateId, const Arc*) const {}
  void FinishVisit() const {}

  bool Aborted() const { return aborted_; }

 private:
  const StateId abort_at_;
  bool aborted_;
};

TEST(DfsVisitTest, SimpleDagTraversal) {
  VectorFst<Arc> fst;
  const auto s0 = fst.AddState();
  const auto s1 = fst.AddState();
  const auto s2 = fst.AddState();
  fst.SetStart(s0);
  fst.SetFinal(s2, Weight::One());
  fst.AddArc(s0, Arc(1, 1, Weight::One(), s1));
  fst.AddArc(s1, Arc(2, 2, Weight::One(), s2));

  OrderRecordingVisitor visitor;
  DfsVisit(fst, &visitor, AnyArcFilter<Arc>());

  EXPECT_THAT(visitor.Discovered(), ElementsAre(0, 1, 2));
  EXPECT_THAT(visitor.Finished(), ElementsAre(2, 1, 0));
}

TEST(DfsVisitTest, EarlyAbort) {
  VectorFst<Arc> fst;
  const auto s0 = fst.AddState();
  const auto s1 = fst.AddState();
  fst.SetStart(s0);
  fst.AddArc(s0, Arc(1, 1, Weight::One(), s1));

  AbortingVisitor visitor(1);
  DfsVisit(fst, &visitor, AnyArcFilter<Arc>());
  EXPECT_TRUE(visitor.Aborted());
}

TEST(DfsVisitTest, SccVisitorAcyclic) {
  VectorFst<Arc> fst;
  const auto s0 = fst.AddState();
  const auto s1 = fst.AddState();
  fst.SetStart(s0);
  fst.SetFinal(s1, Weight::One());
  fst.AddArc(s0, Arc(1, 1, Weight::One(), s1));

  std::vector<StateId> scc;
  std::vector<bool> access;
  std::vector<bool> coaccess;
  uint64_t props = 0;
  SccVisitor<Arc> scc_visitor(&scc, &access, &coaccess, &props);
  DfsVisit(fst, &scc_visitor, AnyArcFilter<Arc>());

  EXPECT_THAT(scc, ElementsAre(0, 1));
  EXPECT_THAT(access, ElementsAre(true, true));
  EXPECT_THAT(coaccess, ElementsAre(true, true));
  EXPECT_FALSE(props & kCyclic);
}

TEST(DfsVisitTest, SccVisitorCyclic) {
  VectorFst<Arc> fst;
  const auto s0 = fst.AddState();
  const auto s1 = fst.AddState();
  const auto s2 = fst.AddState();
  fst.SetStart(s0);
  fst.SetFinal(s2, Weight::One());
  fst.AddArc(s0, Arc(1, 1, Weight::One(), s1));
  fst.AddArc(s1, Arc(2, 2, Weight::One(), s2));
  // Cycle between 1 and 2.
  fst.AddArc(s2, Arc(3, 3, Weight::One(), s1));

  std::vector<StateId> scc;
  std::vector<bool> access;
  std::vector<bool> coaccess;
  uint64_t props = 0;
  SccVisitor<Arc> scc_visitor(&scc, &access, &coaccess, &props);
  DfsVisit(fst, &scc_visitor, AnyArcFilter<Arc>());

  EXPECT_EQ(scc.size(), 3);
  EXPECT_EQ(scc[1], scc[2]);
  EXPECT_NE(scc[0], scc[1]);
  EXPECT_TRUE(props & kCyclic);
}

}  // namespace
}  // namespace fst
