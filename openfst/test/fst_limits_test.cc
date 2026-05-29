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
// Unit test for FST limits (CompactFst and ConstFst).

#include <cstdint>
#include <sstream>

#include "gtest/gtest.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/compact-fst.h"
#include "openfst/lib/const-fst.h"
#include "openfst/lib/fst.h"

namespace fst {
namespace {

// CompactFst limits tests.

TEST(CompactLimitsTest, MaxStatesTest) {
  FstHeader hdr;
  // kMaxStates for CompactArcStore is 1L << 54.
  hdr.SetNumStates((1L << 54) + 1);
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
  // kMaxArcs for CompactArcStore is 1L << 54.
  hdr.SetNumArcs((1L << 54) + 1);
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

// ConstFst limits tests.

TEST(ConstLimitsTest, MaxStatesTest) {
  FstHeader hdr;
  hdr.SetFstType("const");
  hdr.SetArcType(StdArc::Type());
  hdr.SetVersion(2);
  // kMaxStates for ConstFst is 1L << 54.
  hdr.SetNumStates((1L << 54) + 1);
  hdr.SetNumArcs(0);
  hdr.SetStart(0);

  std::istringstream iss;
  FstReadOptions opts;
  opts.header = &hdr;

  auto* impl = internal::ConstFstImpl<StdArc, uint32_t>::Read(iss, opts);
  EXPECT_EQ(impl, nullptr);
}

TEST(ConstLimitsTest, MaxArcsTest) {
  FstHeader hdr;
  hdr.SetFstType("const");
  hdr.SetArcType(StdArc::Type());
  hdr.SetVersion(2);
  hdr.SetNumStates(10);
  // kMaxArcs for ConstFst is 1L << 54.
  hdr.SetNumArcs((1L << 54) + 1);
  hdr.SetStart(0);

  std::istringstream iss;
  FstReadOptions opts;
  opts.header = &hdr;

  auto* impl = internal::ConstFstImpl<StdArc, uint32_t>::Read(iss, opts);
  EXPECT_EQ(impl, nullptr);
}

TEST(ConstLimitsTest, StartStateOutOfRangeTest) {
  FstHeader hdr;
  hdr.SetFstType("const");
  hdr.SetArcType(StdArc::Type());
  hdr.SetVersion(2);
  hdr.SetNumStates(10);
  hdr.SetNumArcs(0);
  hdr.SetStart(10);

  std::istringstream iss;
  FstReadOptions opts;
  opts.header = &hdr;

  auto* impl = internal::ConstFstImpl<StdArc, uint32_t>::Read(iss, opts);
  EXPECT_EQ(impl, nullptr);
}

}  // namespace
}  // namespace fst
