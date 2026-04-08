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
// Regression test for FST classes.

#include "openfst/test/fst_test.h"

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <ostream>
#include <string>

#include "gtest/gtest.h"
#include "absl/base/no_destructor.h"
#include "absl/flags/flag.h"
#include "absl/log/flags.h"
#include "absl/log/log.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/compact-fst.h"
#include "openfst/lib/const-fst.h"
#include "openfst/lib/edit-fst.h"
#include "openfst/lib/float-weight.h"
#include "openfst/lib/fst-decl.h"
#include "openfst/lib/matcher-fst.h"
#include "openfst/lib/product-weight.h"
#include "openfst/lib/register.h"
#include "openfst/lib/test-properties.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/test/compactors.h"

namespace fst {
namespace {

// A user-defined arc type.
struct CustomArc {
  using Label = int16_t;
  using Weight = ProductWeight<TropicalWeight, LogWeight>;
  using StateId = int64_t;

  CustomArc(Label i, Label o, Weight w, StateId s)
      : ilabel(i), olabel(o), weight(w), nextstate(s) {}
  CustomArc() = default;

  static const std::string& Type() {  // Arc type name
    static const absl::NoDestructor<std::string> type("my");
    return *type;
  }

  Label ilabel;       // Transition input label
  Label olabel;       // Transition output label
  Weight weight;      // Transition weight
  StateId nextstate;  // Transition destination state
};

REGISTER_FST(VectorFst, CustomArc);
REGISTER_FST(ConstFst, CustomArc);

static FstRegisterer<CompactArcFst<StdArc, TrivialArcCompactor<StdArc>>>
    CompactFst_StdArc_TrivialCompactor_registerer;
static FstRegisterer<CompactArcFst<CustomArc, TrivialArcCompactor<CustomArc>>>
    CompactFst_CustomArc_TrivialCompactor_registerer;
static FstRegisterer<ConstFst<StdArc, uint16_t>>
    ConstFst_StdArc_uint16_registerer;
static FstRegisterer<
    CompactArcFst<StdArc, TrivialArcCompactor<StdArc>, uint16_t>>
    CompactFst_StdArc_TrivialCompactor_uint16_registerer;
static FstRegisterer<CompactFst<StdArc, TrivialCompactor<StdArc>>>
    CompactFst_StdArc_CustomCompactor_registerer;
static FstRegisterer<CompactFst<CustomArc, TrivialCompactor<CustomArc>>>
    CompactFst_CustomArc_CustomCompactor_registerer;

TEST(FstTest, VectorFstStdArc) {
  for (const size_t num_states : {0, 1, 2, 3, 128}) {
    FstTester<VectorFst<StdArc>> std_vector_tester(num_states);
    std_vector_tester.TestBase();
    std_vector_tester.TestExpanded();
    std_vector_tester.TestAssign();
    std_vector_tester.TestCopy();
    std_vector_tester.TestIO();
    std_vector_tester.TestMutable();
  }
}

TEST(FstTest, EmptyVectorFstStdArc) {
  FstTester<VectorFst<StdArc>> empty_tester(/*num_states=*/0);
  {
    const VectorFst<StdArc> empty_fst;
    empty_tester.TestBase(empty_fst);
    empty_tester.TestExpanded(empty_fst);
    empty_tester.TestCopy(empty_fst);
    empty_tester.TestIO(empty_fst);
    empty_tester.TestAssign(empty_fst);
  }
  {
    VectorFst<StdArc> empty_fst;
    empty_tester.TestMutable(&empty_fst);
  }
}

TEST(FstTest, ConstFstStdArc) {
  FstTester<ConstFst<StdArc>> std_const_tester;
  std_const_tester.TestBase();
  std_const_tester.TestExpanded();
  std_const_tester.TestCopy();
  std_const_tester.TestIO();
}

TEST(FstTest, CompactArcFstStdArcTrivialArcCompactor) {
  FstTester<CompactArcFst<StdArc, TrivialArcCompactor<StdArc>>>
      std_compact_tester;
  std_compact_tester.TestBase();
  std_compact_tester.TestExpanded();
  std_compact_tester.TestCopy();
  std_compact_tester.TestIO();
}

TEST(FstTest, CompactFstStdArcTrivialCompactor) {
  for (const size_t num_states : {0, 1, 2, 3, 128}) {
    FstTester<CompactFst<StdArc, TrivialCompactor<StdArc>>> std_compact_tester(
        num_states);
    std_compact_tester.TestBase();
    std_compact_tester.TestExpanded();
    std_compact_tester.TestCopy();
    std_compact_tester.TestIO();
  }
}

TEST(FstTest, EmptyCompactFstStdArcTrivialCompactor) {
  FstTester<CompactFst<StdArc, TrivialCompactor<StdArc>>> empty_tester(
      /*num_states=*/0);
  {
    const CompactFst<StdArc, TrivialCompactor<StdArc>> empty_fst;
    empty_tester.TestBase(empty_fst);
    empty_tester.TestExpanded(empty_fst);
    empty_tester.TestCopy(empty_fst);
    empty_tester.TestIO(empty_fst);
  }
}

TEST(FstTest, VectorFstCustomArc) {
  FstTester<VectorFst<CustomArc>> std_vector_tester;
  std_vector_tester.TestBase();
  std_vector_tester.TestExpanded();
  std_vector_tester.TestAssign();
  std_vector_tester.TestCopy();
  std_vector_tester.TestIO();
  std_vector_tester.TestMutable();
}

TEST(FstTest, ConstFstCustomArc) {
  FstTester<ConstFst<CustomArc>> std_const_tester;
  std_const_tester.TestBase();
  std_const_tester.TestExpanded();
  std_const_tester.TestCopy();
  std_const_tester.TestIO();
}

TEST(FstTest, CompactArcFstCustomArcTrivialArcCompactor) {
  for (const size_t num_states : {0, 1, 2, 3, 128}) {
    FstTester<CompactArcFst<CustomArc, TrivialArcCompactor<CustomArc>>>
        std_compact_tester(num_states);
    std_compact_tester.TestBase();
    std_compact_tester.TestExpanded();
    std_compact_tester.TestCopy();
    std_compact_tester.TestIO();
  }
}

// TODO: Make this work.
TEST(FstTest, DISABLED_EmptyCompactArcFstCustomArcTrivialArcCompactor) {
  // Test with a default-constructed Fst, not a copied Fst.
  FstTester<CompactArcFst<CustomArc, TrivialArcCompactor<CustomArc>>>
      empty_tester(/*num_states=*/0);
  const CompactArcFst<CustomArc, TrivialArcCompactor<CustomArc>> empty_fst;
  empty_tester.TestBase(empty_fst);
  empty_tester.TestExpanded(empty_fst);
  empty_tester.TestCopy(empty_fst);
  empty_tester.TestIO(empty_fst);
}

TEST(FstTest, CompactFstCustomArcTrivialCompactor) {
  for (const size_t num_states : {0, 1, 2, 3, 128}) {
    FstTester<CompactFst<CustomArc, TrivialCompactor<CustomArc>>>
        std_compact_tester(num_states);
    std_compact_tester.TestBase();
    std_compact_tester.TestExpanded();
    std_compact_tester.TestCopy();
    std_compact_tester.TestIO();
  }

  // TODO: Add tests on default-constructed Fst.
}

TEST(FstTest, ConstFstStdArcUint16) {
  FstTester<ConstFst<StdArc, uint16_t>> std_const_tester;
  std_const_tester.TestBase();
  std_const_tester.TestExpanded();
  std_const_tester.TestCopy();
  std_const_tester.TestIO();
}

TEST(FstTest, CompactArcFstStdArcTrivialArcCompactorUint16) {
  FstTester<CompactArcFst<StdArc, TrivialArcCompactor<StdArc>, uint16_t>>
      std_compact_tester;
  std_compact_tester.TestBase();
  std_compact_tester.TestExpanded();
  std_compact_tester.TestCopy();
  std_compact_tester.TestIO();
}

TEST(FstTest, StdArcLookAheadFst) {
  FstTester<StdArcLookAheadFst> std_matcher_tester;
  std_matcher_tester.TestBase();
  std_matcher_tester.TestExpanded();
  std_matcher_tester.TestCopy();
}

TEST(FstTest, EditFstStdArc) {
  FstTester<EditFst<StdArc>> std_edit_tester;
  std_edit_tester.TestBase();
  std_edit_tester.TestExpanded();
  std_edit_tester.TestAssign();
  std_edit_tester.TestCopy();
  std_edit_tester.TestMutable();
}

}  // namespace
}  // namespace fst

int main(int argc, char** argv) {
  absl::SetFlag(&FLAGS_fst_verify_properties, true);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
