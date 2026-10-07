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
// Unit test for FST limits (CompactFst, ConstFst, and VectorFst).

#include <cstddef>
#include <cstdint>
#include <memory>
#include <sstream>
#include <string>

#include "gtest/gtest.h"
#include "absl/types/span.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/compact-fst.h"
#include "openfst/lib/const-fst.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/vector-fst.h"

namespace fst {
namespace {

// CompactFst limits tests.

TEST(CompactLimitsTest, MaxStatesTest) {
  FstHeader hdr;
  // kMaxStates for CompactArcStore is 1LL << 54.
  hdr.SetNumStates((1LL << 54) + 1);
  hdr.SetNumArcs(0);
  hdr.SetStart(0);

  std::istringstream iss;
  FstReadOptions opts;
  StringCompactor<StdArc> compactor;

  auto* store =
      CompactArcStore<StdArc::Label, uint32_t>::Read(iss, opts, hdr, compactor);
  EXPECT_EQ(store, nullptr);
}

TEST(CompactLimitsTest, MaxArcsTest) {
  FstHeader hdr;
  hdr.SetNumStates(10);
  // kMaxArcs for CompactArcStore is 1LL << 54.
  hdr.SetNumArcs((1LL << 54) + 1);
  hdr.SetStart(0);

  std::istringstream iss;
  FstReadOptions opts;
  StringCompactor<StdArc> compactor;

  auto* store =
      CompactArcStore<StdArc::Label, uint32_t>::Read(iss, opts, hdr, compactor);
  EXPECT_EQ(store, nullptr);
}

TEST(CompactLimitsTest, StartStateOutOfRangeTest) {
  FstHeader hdr;
  hdr.SetNumStates(10);
  hdr.SetNumArcs(0);
  hdr.SetStart(10);

  std::istringstream iss;
  FstReadOptions opts;
  StringCompactor<StdArc> compactor;

  auto* store =
      CompactArcStore<StdArc::Label, uint32_t>::Read(iss, opts, hdr, compactor);
  EXPECT_EQ(store, nullptr);
}

// Variable-size compactor tests: the number of compacts is read from the
// serialized state offsets and must be validated.

using VarCompactor = UnweightedCompactor<StdArc>;
using VarStore = CompactArcStore<VarCompactor::Element, uint64_t>;

// Serializes an FST with the given state offsets followed by `ncompacts`
// zero-initialized elements.
std::string SerializeVarStore(absl::Span<const uint64_t> offsets,
                              size_t ncompacts) {
  std::string data;
  for (uint64_t offset : offsets) {
    data.append(reinterpret_cast<const char*>(&offset), sizeof(offset));
  }
  data.append(ncompacts * sizeof(VarCompactor::Element), '\0');
  return data;
}

std::string SerializeVarStore(uint64_t offset0, uint64_t offset1,
                              size_t ncompacts) {
  return SerializeVarStore({offset0, offset1}, ncompacts);
}

std::unique_ptr<VarStore> ReadVarStore(const std::string& data, int64_t narcs,
                                       int64_t nstates = 1) {
  FstHeader hdr;
  hdr.SetNumStates(nstates);
  hdr.SetNumArcs(narcs);
  hdr.SetStart(0);
  std::istringstream iss(data);
  FstReadOptions opts;
  return std::unique_ptr<VarStore>(
      VarStore::Read(iss, opts, hdr, VarCompactor()));
}

TEST(CompactLimitsTest, VariableSizeWellFormedTest) {
  EXPECT_NE(ReadVarStore(SerializeVarStore(0, 1, 1), /*narcs=*/1), nullptr);
  EXPECT_NE(
      ReadVarStore(SerializeVarStore({0, 1, 2}, 2), /*narcs=*/2, /*nstates=*/2),
      nullptr);
}

TEST(CompactLimitsTest, VariableSizeMaxCompactsTest) {
  // kMaxArcs for CompactArcStore is 1LL << 54.
  EXPECT_EQ(ReadVarStore(SerializeVarStore(0, (1ULL << 54) + 1, 0),
                         /*narcs=*/0),
            nullptr);
  // Would wrap around `ncompacts * sizeof(Element)` without the bound check.
  EXPECT_EQ(ReadVarStore(SerializeVarStore(0, uint64_t{1} << 63, 0),
                         /*narcs=*/0),
            nullptr);
}

TEST(CompactLimitsTest, VariableSizeNonZeroFirstOffsetTest) {
  EXPECT_EQ(ReadVarStore(SerializeVarStore(1, 1, 1), /*narcs=*/0), nullptr);
}

TEST(CompactLimitsTest, VariableSizeFewerCompactsThanArcsTest) {
  EXPECT_EQ(ReadVarStore(SerializeVarStore(0, 1, 1), /*narcs=*/2), nullptr);
}

TEST(CompactLimitsTest, VariableSizeNonMonotonicOffsetsTest) {
  // states_[1] > states_[2] (2 > 1), which would underflow
  // States(2) - States(1) when computing the arc range for state 1.
  EXPECT_EQ(
      ReadVarStore(SerializeVarStore({0, 2, 1}, 1), /*narcs=*/1, /*nstates=*/2),
      nullptr);
  // Interior offset exceeds ncompacts (states_[1] = 5 > states_[2] = 2).
  EXPECT_EQ(
      ReadVarStore(SerializeVarStore({0, 5, 2}, 2), /*narcs=*/2, /*nstates=*/2),
      nullptr);
}

// ConstFst limits tests.

using StdConstFstImpl = internal::ConstFstImpl<StdArc, uint32_t>;

FstHeader MakeConstHeader(int64_t nstates, int64_t narcs, int64_t start) {
  FstHeader hdr;
  hdr.SetFstType("const");
  hdr.SetArcType(StdArc::Type());
  hdr.SetVersion(2);
  hdr.SetNumStates(nstates);
  hdr.SetNumArcs(narcs);
  hdr.SetStart(start);
  return hdr;
}

TEST(ConstLimitsTest, MaxStatesTest) {
  // Include 1 valid ConstState (20 bytes) in the stream so that a 64-bit count
  // like (1LL << 32) + 1 that truncates to 1 when narrowed to int32_t StateId
  // would succeed if not validated before narrowing.
  const std::string one_state_data(20, '\0');
  for (const int64_t nstates :
       {static_cast<int64_t>(StdConstFstImpl::kMaxStates + 1),
        (int64_t{1} << 32) + 1, (int64_t{1} << 54) + 1, int64_t{-1}}) {
    FstHeader hdr = MakeConstHeader(nstates, /*narcs=*/0, /*start=*/0);
    std::istringstream iss(one_state_data);
    FstReadOptions opts;
    opts.header = &hdr;

    EXPECT_EQ(StdConstFstImpl::Read(iss, opts), nullptr);
  }
}

TEST(ConstLimitsTest, MaxArcsTest) {
  for (const int64_t narcs :
       {static_cast<int64_t>(StdConstFstImpl::kMaxArcs + 1), int64_t{-1}}) {
    FstHeader hdr = MakeConstHeader(/*nstates=*/10, narcs, /*start=*/0);
    std::istringstream iss;
    FstReadOptions opts;
    opts.header = &hdr;

    EXPECT_EQ(StdConstFstImpl::Read(iss, opts), nullptr);
  }
}

TEST(ConstLimitsTest, StartStateOutOfRangeTest) {
  // Include 1 valid ConstState (20 bytes) in the stream so that a 64-bit start
  // state like (1LL << 32) that truncates to 0 when narrowed to int32_t StateId
  // would succeed on a 1-state FST if not validated before narrowing.
  const std::string one_state_data(20, '\0');
  for (const int64_t start : {int64_t{1}, int64_t{-2}, int64_t{1} << 32}) {
    FstHeader hdr = MakeConstHeader(/*nstates=*/1, /*narcs=*/0, start);
    std::istringstream iss(one_state_data);
    FstReadOptions opts;
    opts.header = &hdr;

    EXPECT_EQ(StdConstFstImpl::Read(iss, opts), nullptr);
  }
}

// VectorFst limits tests.

using StdVectorFstImpl = internal::VectorFstImpl<VectorState<StdArc>>;

FstHeader MakeVectorHeader(int64_t nstates, int64_t start) {
  FstHeader hdr;
  hdr.SetFstType("vector");
  hdr.SetArcType(StdArc::Type());
  hdr.SetVersion(2);
  hdr.SetNumStates(nstates);
  hdr.SetNumArcs(0);
  hdr.SetStart(start);
  return hdr;
}

TEST(VectorLimitsTest, MaxStatesTest) {
  FstHeader hdr = MakeVectorHeader(
      /*nstates=*/StdVectorFstImpl::kMaxStates + 1, /*start=*/0);
  std::istringstream iss;
  FstReadOptions opts;
  opts.header = &hdr;

  EXPECT_EQ(StdVectorFstImpl::Read(iss, opts), nullptr);
}

TEST(VectorLimitsTest, NegativeNumStatesTest) {
  // Negative NumStates other than kNoStateId (-1) must be rejected before
  // ReserveStates converts it to size_t.
  FstHeader hdr = MakeVectorHeader(/*nstates=*/-2, /*start=*/kNoStateId);
  std::istringstream iss;
  FstReadOptions opts;
  opts.header = &hdr;

  EXPECT_EQ(StdVectorFstImpl::Read(iss, opts), nullptr);
}

TEST(VectorLimitsTest, LargeNumStatesTruncatedStreamTest) {
  // NumStates within [kMaxReserveStates + 1, kMaxStates] on a truncated stream
  // must cap upfront ReserveStates and fail cleanly with unexpected EOF rather
  // than OOMing.
  for (const int64_t nstates : {StdVectorFstImpl::kMaxReserveStates + 1,
                                StdVectorFstImpl::kMaxStates}) {
    FstHeader hdr = MakeVectorHeader(nstates, /*start=*/0);
    std::istringstream iss;
    FstReadOptions opts;
    opts.header = &hdr;

    EXPECT_EQ(StdVectorFstImpl::Read(iss, opts), nullptr);
  }
}

TEST(VectorLimitsTest, StartStateOutOfRangeTest) {
  {
    FstHeader hdr = MakeVectorHeader(/*nstates=*/0, /*start=*/0);
    std::istringstream iss;
    FstReadOptions opts;
    opts.header = &hdr;

    EXPECT_EQ(StdVectorFstImpl::Read(iss, opts), nullptr);
  }
  {
    // Start state < kNoStateId (-1) on an empty FST.
    FstHeader hdr = MakeVectorHeader(/*nstates=*/0, /*start=*/-2);
    std::istringstream iss;
    FstReadOptions opts;
    opts.header = &hdr;

    EXPECT_EQ(StdVectorFstImpl::Read(iss, opts), nullptr);
  }
  {
    // Start state < kNoStateId (-1) on a 1-state FST.
    std::ostringstream oss;
    StdArc::Weight::One().Write(oss);
    const int64_t narcs = 0;
    oss.write(reinterpret_cast<const char*>(&narcs), sizeof(narcs));

    FstHeader hdr = MakeVectorHeader(/*nstates=*/1, /*start=*/-2);
    std::istringstream iss(oss.str());
    FstReadOptions opts;
    opts.header = &hdr;

    EXPECT_EQ(StdVectorFstImpl::Read(iss, opts), nullptr);
  }
}

}  // namespace
}  // namespace fst
