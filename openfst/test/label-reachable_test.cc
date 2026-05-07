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
// Unit test for the LabelReachable class.

#include "openfst/lib/label-reachable.h"

#include <cstdint>
#include <ios>
#include <memory>
#include <random>
#include <string>
#include <vector>

#include "openfst/compat/file_path.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/flags/flag.h"
#include "absl/log/log.h"
#include "openfst/compat/seed_sequences.h"
#include "absl/random/random.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/file-util.h"
#include "openfst/lib/float-weight.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/interval-set.h"
#include "openfst/lib/statesort.h"
#include "openfst/lib/vector-fst.h"

namespace fst {
namespace {

using Arc = StdArc;
using StateId = Arc::StateId;
using Weight = Arc::Weight;
using Label = Arc::Label;

std::vector<IntervalSet<Label>> GetTestIntervalSets() {
  return {IntervalSet<Label>{},
          {{0, 2}, {4, 7}, {9, 10}},
          {{1, 2}, {3, 5}, {6, 10}},
          {{0, 2}, {3, 10}},
          {{1, 2}, {4, 5}, {6, 7}, {9, 10}}};
}

TEST(StateSort, Identity) {
  using IS = IntervalSet<Label>;
  using I = IntervalSet<Label>::Interval;
  std::vector<IS> interval_sets = GetTestIntervalSets();
  StateSort(&interval_sets, std::vector<int>({0, 1, 2, 3, 4}));
  EXPECT_THAT(interval_sets,
              ::testing::ElementsAre(
                  IS({}), IS({I(0, 2), I(4, 7), I(9, 10)}),
                  IS({I(1, 2), I(3, 5), I(6, 10)}), IS({I(0, 2), I(3, 10)}),
                  IS({I(1, 2), I(4, 5), I(6, 7), I(9, 10)})));
}

TEST(StateSort, Rotate) {
  using IS = IntervalSet<Label>;
  using I = IntervalSet<Label>::Interval;
  std::vector<IS> interval_sets = GetTestIntervalSets();
  StateSort(&interval_sets, std::vector<int>({2, 3, 4, 0, 1}));
  EXPECT_THAT(interval_sets,
              ::testing::ElementsAre(IS({I(0, 2), I(3, 10)}),
                                     IS({I(1, 2), I(4, 5), I(6, 7), I(9, 10)}),
                                     IS({}), IS({I(0, 2), I(4, 7), I(9, 10)}),
                                     IS({I(1, 2), I(3, 5), I(6, 10)})));
}

TEST(StateSort, TwoCycles) {
  using IS = IntervalSet<Label>;
  using I = IntervalSet<Label>::Interval;
  std::vector<IS> interval_sets = GetTestIntervalSets();
  // (0 2) (1 4 3)
  StateSort(&interval_sets, std::vector<int>({2, 4, 0, 1, 3}));
  EXPECT_THAT(interval_sets,
              ::testing::ElementsAre(IS({I(1, 2), I(3, 5), I(6, 10)}),
                                     IS({I(0, 2), I(3, 10)}), IS({}),
                                     IS({I(1, 2), I(4, 5), I(6, 7), I(9, 10)}),
                                     IS({I(0, 2), I(4, 7), I(9, 10)})));
}

Label MaxLabel(const Fst<Arc>& fst, bool max_input) {
  Label max_label = kNoLabel;
  for (StateIterator<Fst<Arc>> siter(fst); !siter.Done(); siter.Next()) {
    StateId s = siter.Value();
    for (ArcIterator<Fst<Arc>> aiter(fst, s); !aiter.Done(); aiter.Next()) {
      const Arc& arc = aiter.Value();
      Label label = max_input ? arc.ilabel : arc.olabel;
      if (label > max_label) max_label = label;
    }
  }
  return max_label;
}

class TestLabelReachable {
 public:
  TestLabelReachable(const Fst<Arc>& fst, StateId s, bool reach_input) {
    Visit(fst, s, reach_input);
  }

  bool Reach(Label l) const {
    if (l == kNoLabel) return reach_final_;
    return l >= 0 && l < reach_.size() && reach_[l];
  }

  bool ReachFinal() const { return reach_final_; }

  const std::vector<Label>& ReachVector() { return label_vec_; }

 private:
  void SetReachable(Label l) {
    if (l == kNoLabel) {
      reach_final_ = true;
      return;
    }
    if (l >= reach_.size()) reach_.resize(l + 1, false);
    reach_[l] = true;
  }

  void Visit(const Fst<Arc>& fst, StateId s, bool reach_input) {
    if (visited_.size() <= s) visited_.resize(s + 1, false);
    if (visited_[s]) return;
    visited_[s] = true;

    if (fst.Final(s) != Weight::Zero())
      SetReachable(kNoLabel);  // Mark as final
    for (ArcIterator<Fst<Arc>> aiter(fst, s); !aiter.Done(); aiter.Next()) {
      const Arc& arc = aiter.Value();
      Label label = reach_input ? arc.ilabel : arc.olabel;
      if (label == 0) {
        Visit(fst, arc.nextstate, reach_input);
      } else {
        if (!Reach(label)) label_vec_.push_back(label);
        SetReachable(label);
      }
    }
  }

  std::vector<bool> reach_;
  bool reach_final_ = false;

  std::vector<Label> label_vec_;
  std::vector<bool> visited_;
};

class ReachableTest : public testing::Test {
 protected:
  void SetUp() override {
    afst_.resize(7);
    for (auto i = 0; i < 7; ++i) {
      afst_[i].reset(VectorFst<Arc>::Read(JoinPath(
          std::string("."),
          "openfst/test/testdata/label-reachable",
          "a" + std::to_string(i) + ".fst")));
    }
  }

  void TestReachableLabel(const VectorFst<Arc>& fst, int i,
                          LabelReachable<Arc>* label_reachable) {
    const Label max_label = MaxLabel(fst, false);
    if (fst.NumStates() == 0) return;
    for (auto j = 0; j < kNumRandomStates; ++j) {
      const StateId s = absl::Uniform(rand_, 0, fst.NumStates());
      TestLabelReachable test_reachable(fst, s, false);
      label_reachable->SetState(s);
      for (auto l = 1; l <= max_label; ++l) {
        const bool reach1 = test_reachable.Reach(l);
        const Label r = label_reachable->Relabel(l);
        const bool reach2 = label_reachable->Reach(r);
        if (reach1 && !reach2) {
          LOG(ERROR) << "Reachability of label " << l << " from state " << s
                     << " of Fst a" << i << " failed";
        } else if (!reach1 && reach2) {
          LOG(ERROR) << "Non-reachability of label " << l << " from state " << s
                     << " of Fst a" << i << " failed";
        }
        ASSERT_EQ(reach1, reach2);
      }
    }
  }

  void TestReachableFinal(const VectorFst<Arc>& fst, int i,
                          LabelReachable<Arc>* label_reachable) {
    if (fst.NumStates() == 0) return;
    for (auto j = 0; j < kNumRandomStates; ++j) {
      const StateId s = absl::Uniform(rand_, 0, fst.NumStates());
      TestLabelReachable test_reachable(fst, s, false);
      label_reachable->SetState(s);
      const bool reach1 = test_reachable.ReachFinal();
      const bool reach2 = label_reachable->ReachFinal();
      if (reach1 && !reach2) {
        LOG(ERROR) << "Final reachability from state " << s << " of Fst a" << i
                   << " failed";
      } else if (!reach1 && reach2) {
        LOG(ERROR) << "Final non-reachability from state " << s << " of Fst a"
                   << i << " failed";
        ASSERT_EQ(reach1, reach2);
      }
    }
  }

  absl::BitGen rand_{fst::MakeTaggedSeedSeq("REACHABLE_TEST")};
  std::vector<std::unique_ptr<VectorFst<Arc>>> afst_;
  static constexpr int kNumRandomStates = 1000;  // Number of states to examine.
};

TEST_F(ReachableTest, ReachLabelTest) {
  for (auto i = 0; i < afst_.size(); ++i) {
    VectorFst<Arc>& fst = *afst_[i];
    LabelReachable<Arc> label_reachable(fst, false);
    TestReachableLabel(fst, i, &label_reachable);
  }
}

TEST_F(ReachableTest, StateSortLabelReachableTest) {
  for (auto i = 0; i < afst_.size(); ++i) {
    const VectorFst<Arc>& fst = *afst_[i];
    const auto num_states = fst.NumStates();
    std::vector<StateId> state_order(num_states);
    for (StateId s = 0; s < num_states; ++s) {
      state_order.at(s) = (num_states - s + 5) % num_states;
    }
    // Copy the fst so we can modify it.
    VectorFst<Arc> reordered_fst = fst;
    StateSort(&reordered_fst, state_order);
    // Build LabelReachable on original FST, then sort that.
    LabelReachable<Arc> label_reachable(fst, false);
    StateSort(label_reachable.GetSharedData().get(), state_order);
    TestReachableLabel(reordered_fst, i, &label_reachable);
  }
}

TEST_F(ReachableTest, ReachCopyTest) {
  for (auto i = 0; i < afst_.size(); ++i) {
    VectorFst<Arc>& fst = *afst_[i];
    auto label_reachable = std::make_unique<LabelReachable<Arc>>(fst, false);
    LabelReachable<Arc> copy_label_reachable(*label_reachable);
    label_reachable = nullptr;
    TestReachableLabel(fst, i, &copy_label_reachable);
  }
}

TEST_F(ReachableTest, ReachIOTest) {
  const std::string filename =
      JoinPath(::testing::TempDir(), "iotest.data");
  for (auto i = 0; i < afst_.size(); ++i) {
    VectorFst<Arc>& fst = *afst_[i];
    {
      LabelReachable<Arc> label_reachable(fst, false);
      file::FileOutStream ostrm(filename,
                                std::ios_base::out | std::ios_base::binary);
      ASSERT_TRUE(ostrm);
      ASSERT_TRUE(label_reachable.GetData()->Write(ostrm, FstWriteOptions()));
    }
    file::FileInStream istrm(filename,
                             std::ios_base::in | std::ios_base::binary);
    ASSERT_TRUE(istrm);
    auto data = std::shared_ptr<LabelReachableData<Label>>(
        LabelReachableData<Label>::Read(istrm, FstReadOptions()));
    ASSERT_TRUE(data);
    LabelReachable<Arc> nlabel_reachable(data);
    TestReachableLabel(fst, i, &nlabel_reachable);
  }
}

TEST_F(ReachableTest, ReachFinalTest) {
  for (auto i = 0; i < afst_.size(); ++i) {
    VectorFst<Arc>& fst = *afst_[i];
    LabelReachable<Arc> label_reachable(fst, false);
    TestReachableFinal(fst, i, &label_reachable);
  }
}

}  // namespace
}  // namespace fst
