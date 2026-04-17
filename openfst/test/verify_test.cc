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

#include "openfst/lib/verify.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/status/status.h"
#include "absl/status/status_matchers.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/properties.h"
#include "openfst/lib/symbol-table.h"
#include "openfst/lib/test-properties.h"
#include "openfst/lib/vector-fst.h"

namespace fst {
namespace {

using ::absl_testing::IsOk;
using ::absl_testing::StatusIs;
using ::testing::HasSubstr;

TEST(VerifyTest, Success) {
  VectorFst<StdArc> fst;
  fst.AddState();
  fst.SetStart(0);
  fst.SetFinal(0, StdArc::Weight::One());
  fst.SetProperties(internal::ComputeProperties(fst, kFstProperties, nullptr),
                    kFstProperties);
  EXPECT_THAT(VerifyWithStatus(fst), IsOk());
}

TEST(VerifyTest, StartStateNotSet) {
  VectorFst<StdArc> fst;
  fst.AddState();
  EXPECT_THAT(VerifyWithStatus(fst),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       HasSubstr("FST start state ID not set")));
}

TEST(VerifyTest, StartStateExceedsStates) {
  VectorFst<StdArc> fst;
  fst.SetStart(0);  // 0 >= ns (which is 0)
  EXPECT_THAT(
      VerifyWithStatus(fst),
      StatusIs(absl::StatusCode::kInvalidArgument,
               HasSubstr("FST start state ID exceeds number of states")));
}

TEST(VerifyTest, InputLabelNegative) {
  VectorFst<StdArc> fst;
  fst.AddState();
  fst.SetStart(0);
  fst.AddArc(0, StdArc(-1, 0, StdArc::Weight::One(), 0));
  EXPECT_THAT(
      VerifyWithStatus(fst, /*allow_negative_labels=*/false),
      StatusIs(
          absl::StatusCode::kInvalidArgument,
          HasSubstr(
              "input label ID of arc at position 0 of state 0 is negative")));
}

TEST(VerifyTest, InputLabelMissingFromSymbolTable) {
  VectorFst<StdArc> fst;
  fst.AddState();
  fst.SetStart(0);
  fst.AddArc(0, StdArc(1, 0, StdArc::Weight::One(), 0));
  SymbolTable syms("test");
  syms.AddSymbol("eps", 0);
  fst.SetInputSymbols(&syms);
  EXPECT_THAT(VerifyWithStatus(fst),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       HasSubstr("is missing from input symbol table")));
}

TEST(VerifyTest, OutputLabelNegative) {
  VectorFst<StdArc> fst;
  fst.AddState();
  fst.SetStart(0);
  fst.AddArc(0, StdArc(0, -1, StdArc::Weight::One(), 0));
  EXPECT_THAT(
      VerifyWithStatus(fst, /*allow_negative_labels=*/false),
      StatusIs(
          absl::StatusCode::kInvalidArgument,
          HasSubstr(
              "output label ID of arc at position 0 of state 0 is negative")));
}

TEST(VerifyTest, OutputLabelMissingFromSymbolTable) {
  VectorFst<StdArc> fst;
  fst.AddState();
  fst.SetStart(0);
  fst.AddArc(0, StdArc(0, 1, StdArc::Weight::One(), 0));
  SymbolTable syms("test");
  syms.AddSymbol("eps", 0);
  fst.SetOutputSymbols(&syms);
  EXPECT_THAT(VerifyWithStatus(fst),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       HasSubstr("is missing from output symbol table")));
}

TEST(VerifyTest, InvalidWeight) {
  VectorFst<StdArc> fst;
  fst.AddState();
  fst.SetStart(0);
  fst.AddArc(0, StdArc(0, 0, StdArc::Weight::NoWeight(), 0));
  EXPECT_THAT(
      VerifyWithStatus(fst),
      StatusIs(absl::StatusCode::kInvalidArgument,
               HasSubstr("weight of arc at position 0 of state 0 is invalid")));
}

TEST(VerifyTest, DestinationStateNegative) {
  VectorFst<StdArc> fst;
  fst.AddState();
  fst.SetStart(0);
  fst.AddArc(0, StdArc(0, 0, StdArc::Weight::One(), -1));
  EXPECT_THAT(VerifyWithStatus(fst),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       HasSubstr("destination state ID of arc at position 0 of "
                                 "state 0 is negative")));
}

TEST(VerifyTest, DestinationStateExceedsStates) {
  VectorFst<StdArc> fst;
  fst.AddState();
  fst.SetStart(0);
  fst.AddArc(0, StdArc(0, 0, StdArc::Weight::One(), 1));
  EXPECT_THAT(VerifyWithStatus(fst),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       HasSubstr("destination state ID of arc at position 0 of "
                                 "state 0 exceeds number of states")));
}

TEST(VerifyTest, InvalidFinalWeight) {
  VectorFst<StdArc> fst;
  fst.AddState();
  fst.SetStart(0);
  fst.SetFinal(0, StdArc::Weight::NoWeight());
  EXPECT_THAT(VerifyWithStatus(fst),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       HasSubstr("final weight of state 0 is invalid")));
}

TEST(VerifyTest, ErrorPropertySet) {
  VectorFst<StdArc> fst;
  fst.AddState();
  fst.SetStart(0);
  fst.SetProperties(kError, kError);
  EXPECT_THAT(VerifyWithStatus(fst),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       HasSubstr("error property is set")));
}

TEST(VerifyTest, StoredPropertiesIncorrect) {
  VectorFst<StdArc> fst;
  fst.AddState();
  fst.SetStart(0);
  fst.SetProperties(kIEpsilons,
                    kIEpsilons);  // set to have epsilons but it doesn't
  EXPECT_THAT(VerifyWithStatus(fst),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       HasSubstr("Stored FST properties incorrect")));
}

}  // namespace
}  // namespace fst
