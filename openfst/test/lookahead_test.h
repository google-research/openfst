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
// Unit test for lookahead matchers and filters.

#ifndef OPENFST_TEST_LOOKAHEAD_TEST_H_
#define OPENFST_TEST_LOOKAHEAD_TEST_H_

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "absl/log/log.h"
#include "absl/memory/memory.h"
#include "openfst/compat/compat_memory.h"
#include "openfst/lib/accumulator.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/arcsort.h"
#include "openfst/lib/compose.h"
#include "openfst/lib/equal.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/lookahead-matcher.h"
#include "openfst/lib/matcher-fst.h"
#include "openfst/lib/matcher.h"
#include "openfst/lib/randequivalent.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/lib/verify.h"

namespace fst {

inline constexpr uint64_t kSeed = 8844;

template <typename F>
class LookAheadTest : public ::testing::Test {
 protected:
  using Arc = typename F::Arc;
  using ArcLookAheadFst = MatcherFst<F, ArcLookAheadMatcher<SortedMatcher<F>>,
                                     arc_lookahead_fst_type>;

  using OLabelLookAheadFst =
      MatcherFst<F,
                 LabelLookAheadMatcher<SortedMatcher<F>, olabel_lookahead_flags,
                                       CacheLogAccumulator<StdArc>>,
                 olabel_lookahead_fst_type, LabelLookAheadRelabeler<Arc>>;

  void SetUp() override {
    for (auto i = 0; i < 6; ++i) {
      afst_.emplace_back(VectorFst<Arc>::Read(JoinPath(
          std::string("."),
          "openfst/test/testdata/lookahead/a" +
              std::to_string(i) + ".fst")));
      ArcSort(afst_.back().get(), OLabelCompare<Arc>());
    }

    for (auto i = 0; i < 6; ++i) {
      bfst_.emplace_back(VectorFst<Arc>::Read(JoinPath(
          std::string("."),
          "openfst/test/testdata/lookahead/b" +
              std::to_string(i) + ".fst")));
    }
    tmp_name_ = JoinPath(::testing::TempDir(), "tmp.fst");
  }

  std::vector<std::unique_ptr<VectorFst<Arc>>> afst_;
  std::vector<std::unique_ptr<VectorFst<Arc>>> bfst_;
  std::string tmp_name_;

  bool IsEqual(const Fst<Arc>& fst1, const Fst<Arc>& fst2, int i, int j,
               const std::string& type, bool equal, bool copy) {
    bool same = equal ? Equal(fst1, fst2)
                      : RandEquivalent(fst1, fst2, /*npath=*/100,
                                       /*delta=*/.05, /*seed=*/kSeed);
    if (!same) {
      LOG(WARNING) << "Lookahead composition test of Fst a" << i << " and Fst b"
                   << j << " failed with type = " << type << " copy = " << copy;
    }
    return same;
  }

  void ComposeTest(const Fst<Arc>& xfst, const Fst<Arc>& yfst, int i, int j,
                   const std::string& type, bool equal) {
    Fst<Arc>& afst = *afst_[i];
    Fst<Arc>& bfst = *bfst_[j];
    ComposeFst<Arc> cfst1(afst, bfst);
    ComposeFst<Arc> cfst2(xfst, yfst);
    ASSERT_TRUE(Verify(cfst1));
    ASSERT_TRUE(Verify(cfst2));
    ASSERT_TRUE(IsEqual(cfst1, cfst2, i, j, type, equal, -1));

    // Copy test.
    for (const bool safe : {false, true}) {
      ComposeFst<Arc> cfst3(cfst1, safe);
      ComposeFst<Arc> cfst4(cfst2, safe);
      ASSERT_TRUE(Verify(cfst3));
      ASSERT_TRUE(Verify(cfst4));
      ASSERT_TRUE(IsEqual(cfst3, cfst4, i, j, type, equal, safe));
    }
  }
};

TYPED_TEST_SUITE_P(LookAheadTest);

TYPED_TEST_P(LookAheadTest, IdentityComposeTest) {
  auto& afst = this->afst_;
  auto& bfst = this->bfst_;
  for (auto i = 0; i < afst.size(); ++i)
    for (auto j = 0; j < bfst.size(); ++j)
      this->ComposeTest(*afst[i], *bfst[j], i, j, "identity", true);
}

TYPED_TEST_P(LookAheadTest, ArcLookAheadComposeTest) {
  using ArcLookAheadFst = typename TestFixture::ArcLookAheadFst;
  auto& afst = this->afst_;
  auto& bfst = this->bfst_;
  for (auto i = 0; i < afst.size(); ++i) {
    for (const bool write : {false, true}) {
      auto xfst = std::make_unique<ArcLookAheadFst>(*afst[i]);
      if (write) {
        xfst->Write(this->tmp_name_);
        xfst = absl::WrapUnique(ArcLookAheadFst::Read(this->tmp_name_));
      }
      for (auto j = 0; j < bfst.size(); ++j)
        this->ComposeTest(*xfst, *bfst[j], i, j, "arc-lookahead", false);
    }
  }
}

TYPED_TEST_P(LookAheadTest, LabelLookAheadComposeTest) {
  using OLabelLookAheadFst = typename TestFixture::OLabelLookAheadFst;
  using Arc = typename TestFixture::Arc;
  auto& afst = this->afst_;
  auto& bfst = this->bfst_;
  for (auto i = 0; i < afst.size(); ++i) {
    for (const bool write : {false, true}) {
      auto lfst = std::make_shared<OLabelLookAheadFst>(*afst[i]);
      std::shared_ptr<OLabelLookAheadFst> xfst;
      if (write) {
        lfst->Write(this->tmp_name_);
        xfst = absl::WrapUnique(OLabelLookAheadFst::Read(this->tmp_name_));
      } else {
        xfst = lfst;
      }
      for (auto j = 0; j < bfst.size(); ++j) {
        VectorFst<Arc> yfst(*bfst[j]);
        LabelLookAheadRelabeler<Arc>::Relabel(&yfst, *lfst, true);
        this->ComposeTest(*xfst, yfst, i, j, "label-lookahead", false);
      }
    }
  }
}

REGISTER_TYPED_TEST_SUITE_P(LookAheadTest, IdentityComposeTest,
                            ArcLookAheadComposeTest, LabelLookAheadComposeTest);

}  // namespace fst

#endif  // OPENFST_TEST_LOOKAHEAD_TEST_H_
