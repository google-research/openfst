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

#include "openfst/extensions/ngram/nthbit.h"

#include <cstdint>

#include "gtest/gtest.h"
#include "absl/flags/flag.h"
#include "absl/log/check.h"
#include "absl/log/log.h"
#include "openfst/compat/seed_sequences.h"
#include "absl/numeric/bits.h"
#include "absl/random/random.h"
#include "openfst/compat/seed_sequences.h"

ABSL_FLAG(int32_t, test_size, 10000,
          "Number of random values to compute per test.");

namespace fst {
namespace {

unsigned int nth_bit_scan(uint64_t v, unsigned int r) {
  CHECK_LE(0, r);
  CHECK_LT(r, 64);
  r++;  // This version is more natural with 1-based rank.
  int i = 0;
  for (; i < 64; ++i) {
    if ((r -= v & 1) == 0) return i;
    v >>= 1;
  }
  LOG(FATAL) << "Max allowed rank exceeded; v: " << v << ", r: " << r;
  return i;
}

TEST(NthbitTest, OneBitSet) {
  for (int pos = 0; pos < 64; ++pos) {
    const uint64_t value = uint64_t{1} << pos;
    const unsigned int reference = nth_bit_scan(value, 0);
    const unsigned int test = nth_bit(value, 0);
    EXPECT_EQ(pos, reference);
    EXPECT_EQ(pos, test);
  }
}

TEST(NthbitTest, AllBitsSet) {
  const uint64_t value = 0xFFFFFFFFFFFFFFFFULL;
  for (int rank = 0; rank < 64; ++rank) {
    const unsigned int reference = nth_bit_scan(value, rank);
    const unsigned int test = nth_bit(value, rank);
    EXPECT_EQ(rank, reference);
    EXPECT_EQ(rank, test);
  }
}

TEST(NthbitTest, OneInNBitsSet) {
  for (int one_in_n = 1; one_in_n <= 64; ++one_in_n) {
    const int popcount = 64 / one_in_n;
    uint64_t value = 0;
    for (int rank = 0; rank < popcount; ++rank) {
      value |= uint64_t{1} << (rank * one_in_n);
    }
    for (int rank = 0; rank < popcount; ++rank) {
      const unsigned int reference = nth_bit_scan(value, rank);
      const unsigned int test = nth_bit(value, rank);
      EXPECT_EQ(rank * one_in_n, reference);
      EXPECT_EQ(rank * one_in_n, test);
    }
  }
}

TEST(NthbitTest, RandomCases) {
  absl::BitGen bit_gen(fst::MakeTaggedSeedSeq(
      "RANDOM_CASES"));
  for (int i = 0; i < absl::GetFlag(FLAGS_test_size); ++i) {
    const auto value = absl::Uniform<uint64_t>(bit_gen);
    for (int rank = 0; rank < absl::popcount(value); ++rank) {
      const unsigned int reference = nth_bit_scan(value, rank);
      const unsigned int test = nth_bit(value, rank);
      EXPECT_EQ(reference, test);
    }
  }
}

}  // namespace
}  // namespace fst
