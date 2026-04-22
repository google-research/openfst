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
// Unit test for the StateReachable class.

#include "openfst/lib/state-reachable.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <random>
#include <string>
#include <vector>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "absl/container/flat_hash_set.h"
#include "absl/flags/flag.h"
#include "absl/log/log.h"
#include "openfst/compat/seed_sequences.h"
#include "absl/random/random.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/cc-visitors.h"
#include "openfst/lib/dfs-visit.h"
#include "openfst/lib/float-weight.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/vector-fst.h"

namespace fst {
namespace {

using Arc = StdArc;
using StateId = Arc::StateId;
using Weight = Arc::Weight;
using Label = Arc::Label;

class TestStateReachable {
 public:
  TestStateReachable(const fst::Fst<Arc>& fst, StateId s) { Visit(fst, s); }

  bool Reach(StateId s) { return state_set_.count(s) > 0; }

 private:
  void Visit(const fst::Fst<Arc>& fst, StateId s) {
    while (visited_.size() <= s) visited_.push_back(false);
    if (visited_[s]) return;
    visited_[s] = true;

    if (fst.Final(s) != Weight::Zero()) state_set_.insert(s);

    for (ArcIterator<fst::Fst<Arc>> aiter(fst, s); !aiter.Done();
         aiter.Next()) {
      const Arc& arc = aiter.Value();
      Visit(fst, arc.nextstate);
    }
  }

  absl::flat_hash_set<Label> state_set_;
  std::vector<bool> visited_;
};

class ReachableTest : public testing::Test {
 protected:
  void SetUp() override {
    afst_.resize(7);
    for (auto i = 0; i < 7; ++i) {
      afst_[i].reset(VectorFst<Arc>::Read(JoinPath(
          std::string("."),
          "openfst/test/testdata/state-reachable",
          "a" + std::to_string(i) + ".fst")));
    }
  }

  void TestReachableState(absl::BitGen& bit_gen, const VectorFst<Arc>& fst,
                          int i, StateReachable<Arc>* state_reachable) {
    if (fst.NumStates() == 0) return;
    for (auto j = 0; j < kNumRandomStates; ++j) {
      const StateId s = absl::Uniform(bit_gen, 0, fst.NumStates());
      TestStateReachable test_reachable(fst, s);
      state_reachable->SetState(s);
      for (StateId d = 0; d < fst.NumStates(); ++d) {
        if (fst.Final(d) == Weight::Zero()) continue;
        const bool reach1 = test_reachable.Reach(d);
        const bool reach2 = state_reachable->Reach(d);
        if (reach1 && !reach2) {
          LOG(ERROR) << "Reachability of state " << d << " from state " << s
                     << " of Fst a" << i << " failed";
        } else if (!reach1 && reach2) {
          LOG(ERROR) << "Non-reachability of label " << d << " from state " << s
                     << " of Fst a" << i << " failed";
        }
        ASSERT_EQ(reach1, reach2);
      }
    }
  }

  void GetSccs(const fst::Fst<Arc>& fst) {
    // Gets SCCs
    uint64_t props = 0;
    SccVisitor<Arc> scc_visitor(&scc_, nullptr, nullptr, &props);
    DfsVisit(fst, &scc_visitor);

    // Gets the number of states per SCC.
    nscc_.clear();
    for (StateId s = 0; s < scc_.size(); ++s) {
      StateId c = scc_[s];
      while (c >= nscc_.size()) nscc_.push_back(0);
      ++nscc_[c];
    }
  }

  std::vector<std::unique_ptr<const VectorFst<Arc>>> afst_;
  std::vector<StateId> scc_;
  std::vector<size_t> nscc_;
  static constexpr int kNumRandomStates = 1000;  // Number of states to examine.
};

TEST_F(ReachableTest, ReachStateTest) {
  absl::BitGen bit_gen(fst::MakeTaggedSeedSeq(
      "REACH_STATE_TEST"));
  for (int i = 0; i < afst_.size(); ++i) {
    VectorFst<Arc> fst(*afst_[i]);
    GetSccs(fst);
    for (StateId s = 0; s < fst.NumStates(); ++s) {
      if (absl::Bernoulli(bit_gen, .5) && nscc_[scc_[s]] == 1) {
        fst.SetFinal(s);
      } else {
        fst.SetFinal(s, Weight::Zero());
      }
    }
    StateReachable<Arc> state_reachable(fst);
    TestReachableState(bit_gen, fst, i, &state_reachable);
  }
}

}  // namespace
}  // namespace fst
