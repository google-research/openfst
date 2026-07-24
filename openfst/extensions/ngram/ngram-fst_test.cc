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
#include <ios>
#include <limits>
#include <memory>
#include <sstream>
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
#include "openfst/lib/power-weight.h"
#include "openfst/lib/properties.h"
#include "openfst/lib/statesort.h"
#include "openfst/lib/util.h"
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

TEST(NGramFstDeathTest, FailsOnEmptyFst) {
  absl::SetFlag(&FLAGS_fst_error_fatal, false);
  const VectorFst<StdArc> fst;
  NGramFst<StdArc> ngram_fst(fst);
  EXPECT_TRUE(ngram_fst.Properties(kError, true));
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

TEST(NGramFstTest, ReadFailsOnLargeSize) {
  FstHeader hdr;
  hdr.SetFstType("ngram");
  hdr.SetArcType(StdArc::Type());
  hdr.SetVersion(4);
  std::ostringstream ostrm;
  hdr.Write(ostrm, "");
  const uint64_t large_size = std::numeric_limits<uint64_t>::max();
  const uint64_t zero = 0;
  ostrm.write(reinterpret_cast<const char*>(&large_size), sizeof(large_size));
  ostrm.write(reinterpret_cast<const char*>(&zero), sizeof(zero));
  ostrm.write(reinterpret_cast<const char*>(&zero), sizeof(zero));
  std::string in_data = ostrm.str();
  std::istringstream istrm(in_data);
  EXPECT_EQ(NGramFst<StdArc>::Read(istrm, FstReadOptions()), nullptr);
}

TEST(NGramFstTest, ReadFailsWhenNumFinalExceedsNumStates) {
  FstHeader hdr;
  hdr.SetFstType("ngram");
  hdr.SetArcType(StdArc::Type());
  hdr.SetVersion(4);
  std::ostringstream ostrm;
  hdr.Write(ostrm, "");
  const uint64_t num_states = 1;
  const uint64_t num_futures = 0;
  const uint64_t num_final = 2;
  ostrm.write(reinterpret_cast<const char*>(&num_states), sizeof(num_states));
  ostrm.write(reinterpret_cast<const char*>(&num_futures), sizeof(num_futures));
  ostrm.write(reinterpret_cast<const char*>(&num_final), sizeof(num_final));
  std::string in_data = ostrm.str();
  std::istringstream istrm(in_data);
  EXPECT_EQ(NGramFst<StdArc>::Read(istrm, FstReadOptions()), nullptr);
}

TEST(NGramFstTest, ReadFailsWhenStorageCalculationOverflows) {
  using LargeWeight = PowerWeight<TropicalWeight, 16>;
  using LargeArc = ArcTpl<LargeWeight>;
  FstHeader hdr;
  hdr.SetFstType("ngram");
  hdr.SetArcType(LargeArc::Type());
  hdr.SetVersion(4);
  std::ostringstream ostrm;
  hdr.Write(ostrm, "");
  // Choose values <= kMaxStates that cause 64-bit wrap-around in Storage()
  // so that the calculated size is 0 (< offset == 24), triggering overflow.
  const uint64_t num_states = (1ULL << 58) - 1;  // kMaxStates
  const uint64_t num_futures = 0;
  const uint64_t num_final = (1ULL << 58) - (1ULL << 54) - (1ULL << 51) - 2;
  ostrm.write(reinterpret_cast<const char*>(&num_states), sizeof(num_states));
  ostrm.write(reinterpret_cast<const char*>(&num_futures), sizeof(num_futures));
  ostrm.write(reinterpret_cast<const char*>(&num_final), sizeof(num_final));
  std::string in_data = ostrm.str();
  std::istringstream istrm(in_data);
  EXPECT_EQ(NGramFst<LargeArc>::Read(istrm, FstReadOptions()), nullptr);
}

TEST(NGramFstImplTest, StorageTest) {
  EXPECT_EQ(internal::NGramFstImpl<StdArc>::Storage(2, 2, 2), 100);
  EXPECT_EQ(internal::NGramFstImpl<StdArc>::Storage(3, 2, 2), 108);
  EXPECT_EQ(internal::NGramFstImpl<StdArc>::Storage(2, 3, 2), 108);
  EXPECT_EQ(internal::NGramFstImpl<StdArc>::Storage(2, 2, 3), 104);
}

TEST(NGramFstImplTest, StorageReturnsZeroOnInvalidOrOverflow) {
  const uint64_t max_u64 = std::numeric_limits<uint64_t>::max();
  EXPECT_EQ(internal::NGramFstImpl<StdArc>::Storage(max_u64, max_u64, max_u64),
            0);
  EXPECT_EQ(internal::NGramFstImpl<StdArc>::Storage(0, 0, 0), 0);
  EXPECT_EQ(internal::NGramFstImpl<StdArc>::Storage(max_u64, 1, 1), 0);
  EXPECT_EQ(internal::NGramFstImpl<StdArc>::Storage(1, max_u64, 1), 0);
  EXPECT_EQ(internal::NGramFstImpl<StdArc>::Storage(1, 1, max_u64), 0);
}

}  // namespace fst
