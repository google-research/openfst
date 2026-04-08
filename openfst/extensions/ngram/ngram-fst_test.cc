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

#include "openfst/extensions/ngram/ngram-fst.h"

#include <cstdint>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "absl/base/no_destructor.h"
#include "absl/flags/flag.h"
#include "absl/log/die_if_null.h"
#include "openfst/lib/arc-map.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/arcsort.h"
#include "openfst/lib/equal.h"
#include "openfst/lib/file-util.h"
#include "openfst/lib/float-weight.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/matcher.h"
#include "openfst/lib/mutable-fst.h"
#include "openfst/lib/statesort.h"
#include "openfst/lib/vector-fst.h"

ABSL_FLAG(std::string, fst_file, "", "Fst file to use for tests");

namespace fst {

static std::string Testfile() {
  if (absl::GetFlag(FLAGS_fst_file).empty()) {
    return JoinPath(
        std::string("."),
        "openfst/extensions/ngram/testdata/earnest.mod");
  }
  return absl::GetFlag(FLAGS_fst_file);
}

TEST(NGramFstTest, EmptyFst) {
  const NGramFst<StdArc> fst;
  EXPECT_EQ(fst::kNoStateId, fst.Start());
  EXPECT_EQ(0, fst.NumStates());
}

TEST(NGramFstTest, TestNGramFst) {
  std::unique_ptr<StdMutableFst> fst(StdMutableFst::Read(Testfile()));
  std::vector<StdArc::StateId> order;
  NGramFst<StdArc> loudsfst(*fst, &order);
  StateSort(fst.get(), order);
  EXPECT_TRUE(Equal(*fst, loudsfst));
}

struct CustomArc {
  typedef int16_t Label;
  typedef TropicalWeightTpl<double> Weight;
  typedef int StateId;

  CustomArc(Label i, Label o, Weight w, StateId s)
      : ilabel(i), olabel(o), weight(w), nextstate(s) {}
  CustomArc() = default;
  static const std::string& Type() {
    static const absl::NoDestructor<std::string> type("CustomArc");
    return *type;
  }
  Label ilabel;
  Label olabel;
  Weight weight;
  StateId nextstate;
};

struct Mapper {
  typedef StdArc FromArc;
  typedef CustomArc ToArc;

  ToArc operator()(const FromArc& arc) const {
    return ToArc(arc.ilabel, arc.olabel, ToArc::Weight(arc.weight.Value()),
                 arc.nextstate);
  }

  MapFinalAction FinalAction() const { return MAP_NO_SUPERFINAL; }

  MapSymbolsAction InputSymbolsAction() const { return MAP_COPY_SYMBOLS; }

  MapSymbolsAction OutputSymbolsAction() const { return MAP_COPY_SYMBOLS; }

  uint64_t Properties(uint64_t props) const { return props; }
};

TEST(NGramFstTest, TestNGramFstWithCustomArc) {
  std::unique_ptr<const StdFst> fst(StdFst::Read(Testfile()));
  VectorFst<CustomArc> cfst;
  ArcMap(*fst, &cfst, Mapper());

  std::vector<CustomArc::StateId> order;
  NGramFst<CustomArc> loudsfst(cfst, &order);
  StateSort(&cfst, order);
  EXPECT_TRUE(Equal(cfst, loudsfst, 5000.0f));
}

TEST(NGramFstTest, NGramFstIO) {
  std::unique_ptr<const StdFst> fst(StdFst::Read(Testfile()));
  NGramFst<StdArc> loudsfst(*fst);

  std::string source = JoinPath(::testing::TempDir(), "loudslm.fst");
  loudsfst.Write(source);
  std::unique_ptr<const NGramFst<StdArc>> readloudsfst(
      NGramFst<StdArc>::Read(source));
  EXPECT_TRUE(Equal(loudsfst, *readloudsfst));
}

TEST(NGramFstTest, NGramFstAlignedIO) {
  std::unique_ptr<const StdFst> fst(StdFst::Read(Testfile()));
  NGramFst<StdArc> loudsfst(*fst);
  std::string source = JoinPath(::testing::TempDir(), "loudslm.fst");
  {
    // Create in local frame so that file is closed before reading.
    file::FileOutStream file(source, std::ios_base::out | std::ios::binary);
    FstWriteOptions opts;
    opts.align = true;
    loudsfst.Write(file, opts);
  }
  std::unique_ptr<const NGramFst<StdArc>> readloudsfst(
      ABSL_DIE_IF_NULL(NGramFst<StdArc>::Read(source)));
  EXPECT_TRUE(Equal(loudsfst, *readloudsfst));
}

TEST(NGramFstTest, NGramMatcher) {
  std::unique_ptr<StdMutableFst> fst(StdMutableFst::Read(Testfile()));
  std::vector<StdArc::StateId> order;
  NGramFst<StdArc> loudsfst(*fst, &order);
  StateSort(fst.get(), order);
  std::unique_ptr<MatcherBase<StdArc>> matcher(
      loudsfst.InitMatcher(MATCH_INPUT));
  for (StateIterator<StdFst> siter(*fst); !siter.Done(); siter.Next()) {
    if (siter.Value() > 10) break;
    matcher->SetState(siter.Value());
    // There should always be the loop arc.
    ASSERT_TRUE(matcher->Find(0));
    EXPECT_EQ(matcher->Value().ilabel, kNoLabel);
    EXPECT_EQ(matcher->Value().olabel, 0);
    EXPECT_EQ(matcher->Value().weight, StdArc::Weight::One());
    EXPECT_EQ(matcher->Value().nextstate, siter.Value());
    for (ArcIterator<StdFst> aiter(*fst, siter.Value()); !aiter.Done();
         aiter.Next()) {
      if (aiter.Value().ilabel == 0) {
        ASSERT_FALSE(matcher->Done());
        matcher->Next();
      } else {
        ASSERT_TRUE(matcher->Find(aiter.Value().ilabel));
      }
      EXPECT_EQ(aiter.Value().ilabel, matcher->Value().ilabel);
      EXPECT_EQ(aiter.Value().olabel, matcher->Value().olabel);
      EXPECT_EQ(aiter.Value().weight, matcher->Value().weight);
      EXPECT_EQ(aiter.Value().nextstate, matcher->Value().nextstate);
      matcher->Next();
      ASSERT_TRUE(matcher->Done());
    }
  }
}

TEST(NGramFstTest, NGramMatcherBackoff) {
  std::unique_ptr<StdMutableFst> fst(StdMutableFst::Read(Testfile()));
  std::vector<StdArc::StateId> order;
  NGramFst<StdArc> loudsfst(*fst, &order);
  StateSort(fst.get(), order);
  ArcSort(fst.get(), StdILabelCompare());
  SortedMatcher<StdFst> base_matcher(*fst, MATCH_INPUT);
  NGramFstMatcher<StdArc> matcher(loudsfst, MATCH_INPUT);

  matcher.SetState(1);
  matcher.Find(1);

  // Match on backoff arc to unigram state.
  for (StateIterator<StdFst> siter(*fst); !siter.Done(); siter.Next()) {
    base_matcher.SetState(siter.Value());
    matcher.SetState(siter.Value());
    base_matcher.Find(-1);
    matcher.Find(-1);
    EXPECT_EQ(base_matcher.Done(), matcher.Done());
    if (!base_matcher.Done()) {
      EXPECT_EQ(base_matcher.Value().ilabel, matcher.Value().ilabel);
      EXPECT_EQ(base_matcher.Value().weight, matcher.Value().weight);
      EXPECT_EQ(base_matcher.Value().nextstate, matcher.Value().nextstate);
      base_matcher.Next();
      matcher.Next();
      EXPECT_EQ(base_matcher.Done(), matcher.Done());
    }
  }
}

}  // namespace fst
