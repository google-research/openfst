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

#include "openfst/lib/merge-fst.h"

#include <memory>
#include <vector>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "absl/container/flat_hash_map.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/equal.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/lib/verify.h"

namespace fst {
namespace {

class MergeFstTest : public ::testing::Test {
 protected:
  typedef StdArc::StateId StateId;
  typedef StdArc::Weight Weight;

  void SetUp() override {
    // Define state map.
    state_map_[0] = 0;
    state_map_[3] = 1;
    ref_  = { 0, kNoStateId, kNoStateId, 1};

    // Define "primary" Fst.
    for (int i = 0; i <= 2; ++i) primary_fst_.AddState();
    primary_fst_.SetStart(0);
    primary_fst_.AddArc(0, StdArc(1, 4, Weight(1.0F), 1));
    primary_fst_.AddArc(1, StdArc(2, 5, Weight(2.0F), 1));
    primary_fst_.AddArc(1, StdArc(3, 6, Weight(3.0F), 2));
    primary_fst_.SetFinal(2, Weight(4.0F));

    // Define "secondary" Fst.
    for (int i = 0; i <= 1; ++i) secondary_fst_.AddState();
    secondary_fst_.AddArc(0, StdArc(2, 7, Weight(1.0F), 3));
    secondary_fst_.AddArc(1, StdArc(3, 4, Weight(3.0F), 2));
    secondary_fst_.AddArc(1, StdArc(1, 8, Weight(2.0F), 3));

    // Define reference "merged" Fst.
    ref_fst_ = primary_fst_;
    ref_fst_.AddState();
    ref_fst_.AddArc(0, StdArc(2, 7, Weight(1.0F), 3));
    ref_fst_.AddArc(3, StdArc(3, 4, Weight(3.0F), 2));
    ref_fst_.AddArc(3, StdArc(1, 8, Weight(2.0F), 3));
  }

  void TearDown() override {
  }

  template <class M>
  void TestMergeFstType() {
    typedef MergeFst<StdArc, M> MergeFstType;
    // Tests Merge Fst construction.
    MergeFstType merg_fst(primary_fst_, secondary_fst_, state_map_);
    EXPECT_TRUE(Verify(merg_fst));
    ASSERT_TRUE(Equal(merg_fst, ref_fst_));

    // Tests Read/Write methods.
    merg_fst.Write(::testing::TempDir() + "/merge.fst");
    const auto* fst =
        MergeFstType::Read(JoinPath(::testing::TempDir(), "merge.fst"));
    ASSERT_TRUE(fst != nullptr) << "Failed to read merged FST";
    std::unique_ptr<const StdFst> read_fst(fst);
    EXPECT_TRUE(Verify(*read_fst));
    ASSERT_TRUE(Equal(*read_fst, ref_fst_));

    // Tests Copy() method.
    for (int i = 0; i < 2; ++i) {
      std::unique_ptr<const MergeFstType> copy_fst(merg_fst.Copy(i));
      ASSERT_TRUE(copy_fst.get() != nullptr);
      EXPECT_TRUE(Verify(*copy_fst));
      ASSERT_TRUE(Equal(*copy_fst, ref_fst_));
    }

    // Tests arc iterators.
    for (StdArc::StateId s = 0; s < ref_fst_.NumStates(); ++s) {
      ArcIterator<StdFst> aiter1(merg_fst, s);
      ArcIterator<MergeFstType> aiter2(merg_fst, s);
      for (; !aiter1.Done() || !aiter2.Done(); aiter1.Next(), aiter2.Next()) {
        ASSERT_TRUE(!aiter1.Done() && !aiter2.Done());
        EXPECT_EQ(aiter1.Value().ilabel,    aiter2.Value().ilabel);
        EXPECT_EQ(aiter1.Value().olabel,    aiter2.Value().olabel);
        EXPECT_EQ(aiter1.Value().weight,    aiter2.Value().weight);
        EXPECT_EQ(aiter1.Value().nextstate, aiter2.Value().nextstate);
        EXPECT_EQ(aiter1.Position(),        aiter2.Position());
      }
      EXPECT_TRUE(aiter1.Done());
      EXPECT_TRUE(aiter2.Done());
      for (int i = 0; i < ref_fst_.NumArcs(s); ++i) {
        aiter1.Seek(i);
        aiter2.Seek(i);
        EXPECT_EQ(aiter1.Position(), i);
        EXPECT_EQ(aiter2.Position(), i);
        EXPECT_EQ(aiter1.Value().ilabel,    aiter2.Value().ilabel);
        EXPECT_EQ(aiter1.Value().olabel,    aiter2.Value().olabel);
        EXPECT_EQ(aiter1.Value().weight,    aiter2.Value().weight);
        EXPECT_EQ(aiter1.Value().nextstate, aiter2.Value().nextstate);
      }
    }
  }

  absl::flat_hash_map<StateId, StateId> state_map_;
  std::vector<StateId> ref_;
  StdVectorFst primary_fst_;
  StdVectorFst secondary_fst_;
  StdVectorFst ref_fst_;
};

TEST_F(MergeFstTest, MergeStateMap) {
  HashMergeStateMap<StateId> hash_mst(state_map_);
  VectorMergeStateMap<StateId> vect_mst(state_map_, 3);
  FixedMergeStateMap<StateId> fixd_mst(3, 1);
  FixedMergeStateMap<StateId> fixe_mst(state_map_, 3);
  for (StdArc::StateId s = 0; s < 4; ++s) {
    EXPECT_EQ(hash_mst.Find(s), ref_[s]);
    EXPECT_EQ(vect_mst.Find(s), ref_[s]);
    EXPECT_EQ(fixd_mst.Find(s), ref_[s]);
    EXPECT_EQ(fixe_mst.Find(s), ref_[s]);
  }
}

TEST_F(MergeFstTest, HashMergeFst) {
  TestMergeFstType<HashMergeStateMap<StateId> >();
}

TEST_F(MergeFstTest, VectorMergeFst) {
  TestMergeFstType<VectorMergeStateMap<StateId> >();
}

TEST_F(MergeFstTest, FixedMergeFst) {
  TestMergeFstType<FixedMergeStateMap<StateId> >();
}

}  // namespace
}  // namespace fst
