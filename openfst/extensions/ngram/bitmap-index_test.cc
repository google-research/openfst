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

#include "openfst/extensions/ngram/bitmap-index.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <tuple>
#include <utility>

#include "gtest/gtest.h"
#include "absl/flags/flag.h"
#include "absl/log/check.h"
#include "absl/memory/memory.h"
#include "absl/random/random.h"
#include "absl/strings/str_cat.h"
#include "benchmark/benchmark.h"

using ::benchmark::State;

ABSL_FLAG(int32_t, bm_nthbit_bitmap_size, 80000,
          "Size of bitmap in bits for nth bit tests");
ABSL_FLAG(int32_t, bm_nthbit_set_bit_percent, 50,
          "Set this percent of bits in the bitmap for benchmarks.");
// When `Select0`/`Select1` is called, `BitmapIndex` should almost always be
// built with an index.  Default these to true, but allow benchmarking of
// the no-index code path.
ABSL_FLAG(bool, bm_enable_select0_index, true,
          "Enable index for Select0 in benchmarks.");
ABSL_FLAG(bool, bm_enable_select1_index, true,
          "Enable index for Select1 in benchmarks.");
// Use `--perf-counters=branch-misses` to find a value near the plateau
// of the branch-misses per op curve.  10k was enough in 2023.
ABSL_FLAG(int32_t, bm_batch_size, 10000,
          "Number of random operations to perform in benchmark.");

namespace fst {
namespace {

// Fills in the bitmap with roughly 'percent_filled' bits randomly set. Returns
// a pair of count of bits set and the sum of the indices of the set bits.
std::pair<uint32_t, int64_t> FillBitMap(int percent_filled, uint64_t* bm,
                                        int num_bits) {
  absl::BitGen gen;
  std::pair<uint32_t, int64_t> bitmap_info_pair(0, 0);
  CHECK_GE(percent_filled, 0);
  CHECK_LE(percent_filled, 100);
  for (uint32_t index = 0; index < num_bits; ++index) {
    if (absl::Uniform(gen, 0, 100) < percent_filled) {
      BitmapIndex::Set(bm, index);
      ++bitmap_info_pair.first;
      bitmap_info_pair.second += index;
    }
  }
  return bitmap_info_pair;
}

// Generates `batch_size` random `uint32_t`s with range `[0, hi_val)`.
std::unique_ptr<uint32_t[]> GenerateRandomBatch(uint32_t batch_size,
                                                uint32_t hi_val) {
  auto batch = absl::make_unique_for_overwrite<uint32_t[]>(batch_size);
  absl::BitGen bitgen;
  std::generate_n(&batch[0], batch_size,
                  [&]() { return absl::Uniform<uint32_t>(bitgen, 0, hi_val); });
  return batch;
}

// Ranks `FLAGS_bm_batch_size` random positions through the bitstring.
void BM_Rank1(State& state) {
  const int num_bits = state.range(0);
  auto bm = std::make_unique<uint64_t[]>(BitmapIndex::StorageSize(num_bits));
  FillBitMap(absl::GetFlag(FLAGS_bm_nthbit_set_bit_percent), bm.get(),
             num_bits);
  BitmapIndex bitmap(bm.get(), num_bits);

  const int32_t batch_size = absl::GetFlag(FLAGS_bm_batch_size);
  std::unique_ptr<uint32_t[]> positions =
      GenerateRandomBatch(batch_size, /*hi_val=*/num_bits);

  while (state.KeepRunningBatch(batch_size)) {
    for (int i = 0; i < batch_size; ++i) {
      benchmark::DoNotOptimize(bitmap.Rank1(positions[i]));
    }
  }
}

void BM_Rank0(State& state) {
  const int num_bits = state.range(0);
  auto bm = std::make_unique<uint64_t[]>(BitmapIndex::StorageSize(num_bits));
  FillBitMap(absl::GetFlag(FLAGS_bm_nthbit_set_bit_percent), bm.get(),
             num_bits);
  BitmapIndex bitmap(bm.get(), num_bits);

  const int32_t batch_size = absl::GetFlag(FLAGS_bm_batch_size);
  std::unique_ptr<uint32_t[]> positions =
      GenerateRandomBatch(batch_size, /*hi_val=*/num_bits);

  while (state.KeepRunningBatch(batch_size)) {
    for (int i = 0; i < batch_size; ++i) {
      benchmark::DoNotOptimize(bitmap.Rank0(positions[i]));
    }
  }
}

// Ranks `FLAGS_bm_batch_size` random positions through the bitstring with
// a loop-carried dependency to measure latency instead of throughput.
void BM_Rank1Latency(State& state) {
  const int num_bits = state.range(0);
  auto bm = std::make_unique<uint64_t[]>(BitmapIndex::StorageSize(num_bits));
  FillBitMap(absl::GetFlag(FLAGS_bm_nthbit_set_bit_percent), bm.get(),
             num_bits);
  BitmapIndex bitmap(bm.get(), num_bits);

  const int32_t batch_size = absl::GetFlag(FLAGS_bm_batch_size);
  std::unique_ptr<uint32_t[]> positions =
      GenerateRandomBatch(batch_size, /*hi_val=*/num_bits);

  // Transform positions by XORing with previous result.  When we call
  // `Rank1` in the benchmark loop, we will use the original value of
  // `positions[i]`, but we encode it so that the previous result is
  // required, thus measuring latency.
  uint32_t t = 0;
  for (int i = 0; i < batch_size; ++i) {
    uint32_t r = bitmap.Rank1(positions[i]);
    positions[i] ^= t;
    t = r;
  }

  while (state.KeepRunningBatch(batch_size)) {
    uint32_t t = 0;
    for (int i = 0; i < batch_size; ++i) {
      t = bitmap.Rank1(positions[i] ^ t);
    }
  }
}

void BM_Rank0Latency(State& state) {
  const int num_bits = state.range(0);
  auto bm = std::make_unique<uint64_t[]>(BitmapIndex::StorageSize(num_bits));
  FillBitMap(absl::GetFlag(FLAGS_bm_nthbit_set_bit_percent), bm.get(),
             num_bits);
  BitmapIndex bitmap(bm.get(), num_bits);

  const int32_t batch_size = absl::GetFlag(FLAGS_bm_batch_size);
  std::unique_ptr<uint32_t[]> positions =
      GenerateRandomBatch(batch_size, /*hi_val=*/num_bits);

  uint32_t t = 0;
  for (int i = 0; i < batch_size; ++i) {
    uint32_t r = bitmap.Rank0(positions[i]);
    positions[i] ^= t;
    t = r;
  }

  while (state.KeepRunningBatch(batch_size)) {
    uint32_t t = 0;
    for (int i = 0; i < batch_size; ++i) {
      t = bitmap.Rank0(positions[i] ^ t);
    }
  }
}

// Finds `FLAGS_bm_batch_size` ones randomly positioned through the bitstring.
void BM_Select1(State& state) {
  const int num_bits = state.range(0);
  auto bm = std::make_unique<uint64_t[]>(BitmapIndex::StorageSize(num_bits));
  FillBitMap(absl::GetFlag(FLAGS_bm_nthbit_set_bit_percent), bm.get(),
             num_bits);
  BitmapIndex bitmap(bm.get(), num_bits, /*enable_select_0_index=*/false,
                     absl::GetFlag(FLAGS_bm_enable_select1_index));

  const uint32_t num_ones = bitmap.GetOnesCount();
  const int32_t batch_size = absl::GetFlag(FLAGS_bm_batch_size);
  std::unique_ptr<uint32_t[]> positions =
      GenerateRandomBatch(batch_size, /*hi_val=*/num_ones);

  while (state.KeepRunningBatch(batch_size)) {
    for (int i = 0; i < batch_size; ++i) {
      benchmark::DoNotOptimize(bitmap.Select1(positions[i]));
    }
  }
}

// Finds `FLAGS_bm_batch_size` zeros randomly positioned through the bitstring.
void BM_Select0(State& state) {
  const int num_bits = state.range(0);
  auto bm = std::make_unique<uint64_t[]>(BitmapIndex::StorageSize(num_bits));
  FillBitMap(absl::GetFlag(FLAGS_bm_nthbit_set_bit_percent), bm.get(),
             num_bits);
  BitmapIndex bitmap(bm.get(), num_bits,
                     absl::GetFlag(FLAGS_bm_enable_select0_index),
                     /*enable_select_1_index=*/false);

  const uint32_t num_zeros = bitmap.Bits() - bitmap.GetOnesCount();
  const int32_t batch_size = absl::GetFlag(FLAGS_bm_batch_size);
  std::unique_ptr<uint32_t[]> positions =
      GenerateRandomBatch(batch_size, /*hi_val=*/num_zeros);

  while (state.KeepRunningBatch(batch_size)) {
    for (int i = 0; i < batch_size; ++i) {
      benchmark::DoNotOptimize(bitmap.Select0(positions[i]));
    }
  }
}

// Finds `FLAGS_bm_batch_size` zeros randomly positioned through the bitstring.
void BM_Select0s(State& state) {
  const int num_bits = state.range(0);
  auto bm = std::make_unique<uint64_t[]>(BitmapIndex::StorageSize(num_bits));
  FillBitMap(absl::GetFlag(FLAGS_bm_nthbit_set_bit_percent), bm.get(),
             num_bits);
  BitmapIndex bitmap(bm.get(), num_bits,
                     absl::GetFlag(FLAGS_bm_enable_select0_index),
                     /*enable_select_1_index=*/false);

  const uint32_t num_zeros = bitmap.Bits() - bitmap.GetOnesCount();
  const int32_t batch_size = absl::GetFlag(FLAGS_bm_batch_size);
  std::unique_ptr<uint32_t[]> positions =
      GenerateRandomBatch(batch_size, /*hi_val=*/num_zeros);

  while (state.KeepRunningBatch(batch_size)) {
    for (int i = 0; i < batch_size; ++i) {
      benchmark::DoNotOptimize(bitmap.Select0s(positions[i]));
    }
  }
}

void BM_Select1Latency(State& state) {
  const int num_bits = state.range(0);
  auto bm = std::make_unique<uint64_t[]>(BitmapIndex::StorageSize(num_bits));
  FillBitMap(absl::GetFlag(FLAGS_bm_nthbit_set_bit_percent), bm.get(),
             num_bits);
  BitmapIndex bitmap(bm.get(), num_bits, /*enable_select_0_index=*/false,
                     absl::GetFlag(FLAGS_bm_enable_select1_index));

  const uint32_t num_ones = bitmap.GetOnesCount();
  const int32_t batch_size = absl::GetFlag(FLAGS_bm_batch_size);
  std::unique_ptr<uint32_t[]> positions =
      GenerateRandomBatch(batch_size, /*hi_val=*/num_ones);

  uint32_t t = 0;
  for (int i = 0; i < batch_size; ++i) {
    uint32_t r = bitmap.Select1(positions[i]);
    positions[i] ^= t;
    t = r;
  }

  while (state.KeepRunningBatch(batch_size)) {
    uint32_t t = 0;
    for (int i = 0; i < batch_size; ++i) {
      t = bitmap.Select1(positions[i] ^ t);
    }
  }
}

void BM_Select0Latency(State& state) {
  const int num_bits = state.range(0);
  auto bm = std::make_unique<uint64_t[]>(BitmapIndex::StorageSize(num_bits));
  FillBitMap(absl::GetFlag(FLAGS_bm_nthbit_set_bit_percent), bm.get(),
             num_bits);
  BitmapIndex bitmap(bm.get(), num_bits,
                     absl::GetFlag(FLAGS_bm_enable_select0_index),
                     /*enable_select_1_index=*/false);

  const uint32_t num_zeros = bitmap.Bits() - bitmap.GetOnesCount();
  const int32_t batch_size = absl::GetFlag(FLAGS_bm_batch_size);
  std::unique_ptr<uint32_t[]> positions =
      GenerateRandomBatch(batch_size, /*hi_val=*/num_zeros);

  uint32_t t = 0;
  for (int i = 0; i < batch_size; ++i) {
    uint32_t r = bitmap.Select0(positions[i]);
    positions[i] ^= t;
    t = r;
  }

  while (state.KeepRunningBatch(batch_size)) {
    uint32_t t = 0;
    for (int i = 0; i < batch_size; ++i) {
      t = bitmap.Select0(positions[i] ^ t);
    }
  }
}

void BM_Select0sLatency(State& state) {
  const int num_bits = state.range(0);
  auto bm = std::make_unique<uint64_t[]>(BitmapIndex::StorageSize(num_bits));
  FillBitMap(absl::GetFlag(FLAGS_bm_nthbit_set_bit_percent), bm.get(),
             num_bits);
  BitmapIndex bitmap(bm.get(), num_bits,
                     absl::GetFlag(FLAGS_bm_enable_select0_index),
                     /*enable_select_1_index=*/false);

  const uint32_t num_zeros = bitmap.Bits() - bitmap.GetOnesCount();
  const int32_t batch_size = absl::GetFlag(FLAGS_bm_batch_size);
  std::unique_ptr<uint32_t[]> positions =
      GenerateRandomBatch(batch_size, /*hi_val=*/num_zeros);

  uint32_t t = 0;
  for (int i = 0; i < batch_size; ++i) {
    uint32_t r = bitmap.Select0s(positions[i]).second;
    positions[i] ^= t;
    t = r;
  }

  while (state.KeepRunningBatch(batch_size)) {
    uint32_t t = 0;
    for (int i = 0; i < batch_size; ++i) {
      t = bitmap.Select0s(positions[i] ^ t).second;
    }
  }
}

void BM_BuildIndex(State& state) {
  const int num_bits = state.range(0);
  auto bm = std::make_unique<uint64_t[]>(BitmapIndex::StorageSize(num_bits));
  FillBitMap(absl::GetFlag(FLAGS_bm_nthbit_set_bit_percent), bm.get(),
             num_bits);
  BitmapIndex bitmap;

  for (auto _ : state) {
    bitmap.BuildIndex(bm.get(), num_bits,
                      absl::GetFlag(FLAGS_bm_enable_select0_index),
                      absl::GetFlag(FLAGS_bm_enable_select1_index));
    CHECK_EQ(num_bits, bitmap.Bits());
  }
}

void BM_GetOnesCount(State& state) {
  const int num_bits = state.range(0);
  auto bm = std::make_unique<uint64_t[]>(BitmapIndex::StorageSize(num_bits));
  // Note: The percent_filled doesn't affect the performance of
  // GetOnesCount.
  const uint32_t expected_sum =
      FillBitMap(absl::GetFlag(FLAGS_bm_nthbit_set_bit_percent), bm.get(),
                 num_bits)
          .first;
  BitmapIndex bitmap(bm.get(), num_bits);
  for (auto _ : state) {
    CHECK_EQ(expected_sum, bitmap.GetOnesCount());
  }
}

static constexpr int kBenchmarkMinBits = 256;
static constexpr int kBenchmarkMaxBits = 16 << 20;

BENCHMARK(BM_Rank1)->Range(kBenchmarkMinBits, kBenchmarkMaxBits);
BENCHMARK(BM_Rank1Latency)->Range(kBenchmarkMinBits, kBenchmarkMaxBits);
BENCHMARK(BM_Rank0)->Range(kBenchmarkMinBits, kBenchmarkMaxBits);
BENCHMARK(BM_Rank0Latency)->Range(kBenchmarkMinBits, kBenchmarkMaxBits);

BENCHMARK(BM_Select1)->Range(kBenchmarkMinBits, kBenchmarkMaxBits);
BENCHMARK(BM_Select1Latency)->Range(kBenchmarkMinBits, kBenchmarkMaxBits);
BENCHMARK(BM_Select0)->Range(kBenchmarkMinBits, kBenchmarkMaxBits);
BENCHMARK(BM_Select0Latency)->Range(kBenchmarkMinBits, kBenchmarkMaxBits);
BENCHMARK(BM_Select0s)->Range(kBenchmarkMinBits, kBenchmarkMaxBits);
BENCHMARK(BM_Select0sLatency)->Range(kBenchmarkMinBits, kBenchmarkMaxBits);

BENCHMARK(BM_BuildIndex)->Range(kBenchmarkMinBits, kBenchmarkMaxBits);
BENCHMARK(BM_GetOnesCount)->Range(kBenchmarkMinBits, kBenchmarkMaxBits);

class BitmapIndexTest : public testing::Test {};

// Parameterized fixture for test that use Select0 or Select1.
// Params are (enable_select_0_index, enable_select_1_index).
class BitmapIndexSelectTest
    : public testing::TestWithParam<std::tuple<bool, bool>> {
 public:
  BitmapIndexSelectTest()
      : enable_select_0_index_(std::get<0>(GetParam())),
        enable_select_1_index_(std::get<1>(GetParam())) {}

 protected:
  const bool enable_select_0_index_;
  const bool enable_select_1_index_;
};

TEST_P(BitmapIndexSelectTest, Empty) {
  BitmapIndex imap(nullptr, 0, enable_select_0_index_, enable_select_1_index_);
  EXPECT_EQ(0, imap.Bits());
  EXPECT_EQ(0, imap.ArraySize());
  EXPECT_EQ(0, imap.GetOnesCount());
  EXPECT_EQ(0, imap.Rank1(0));
  EXPECT_EQ(0, imap.Rank0(0));
  EXPECT_EQ(0, imap.Select1(0));
  EXPECT_EQ(0, imap.Select0(0));

  // Get some test coverage for these functions so clshepherd doesn't delete
  // them.
  EXPECT_EQ(0, imap.ArrayBytes());
  EXPECT_LT(0, imap.IndexBytes());
}

TEST_F(BitmapIndexTest, OneZeroCounts) {
  for (int i = 1; i < 3 * 5 * 8; i++) {
    int num_bits = i * 3;
    int one_interval = (i % 5) + 1;
    auto bm = std::make_unique<uint64_t[]>(BitmapIndex::StorageSize(num_bits));
    BitmapIndex map(bm.get(), num_bits);
    int count = 0;
    for (int j = 0; j < num_bits; j += one_interval) {
      BitmapIndex::Set(bm.get(), j);
      ++count;
    }
    map.BuildIndex(bm.get(), num_bits);
    EXPECT_EQ(map.GetOnesCount(), count);
  }
}

TEST_P(BitmapIndexSelectTest, RankSelectBeforeLimit) {
  // Pick a size that is not a multiple of 32 intentionally.
  auto bm = std::make_unique<uint64_t[]>(BitmapIndex::StorageSize(40));
  BitmapIndex bitmap(bm.get(), 40, enable_select_0_index_,
                     enable_select_1_index_);

  EXPECT_EQ(0, bitmap.GetOnesCount());
  EXPECT_EQ(0, bitmap.Select0(0));

  BitmapIndex::Set(bm.get(), 24);
  bitmap.BuildIndex(bm.get(), 40, enable_select_0_index_,
                    enable_select_1_index_);
  EXPECT_EQ(1, bitmap.GetOnesCount());
  EXPECT_EQ(0, bitmap.Rank1(24));
  EXPECT_EQ(1, bitmap.Rank1(25));
  EXPECT_EQ(24, bitmap.Rank0(24));
  EXPECT_EQ(24, bitmap.Rank0(25));
  EXPECT_EQ(24, bitmap.Select1(0));
  EXPECT_EQ(23, bitmap.Select0(23));
  EXPECT_EQ(25, bitmap.Select0(24));

  BitmapIndex::Set(bm.get(), 39);
  bitmap.BuildIndex(bm.get(), 40, enable_select_0_index_,
                    enable_select_1_index_);
  EXPECT_EQ(2, bitmap.GetOnesCount());
  EXPECT_EQ(1, bitmap.Rank1(39));
  EXPECT_EQ(38, bitmap.Rank0(39));
  EXPECT_EQ(38, bitmap.Rank0(40));
  EXPECT_EQ(39, bitmap.Select1(1));
  EXPECT_EQ(38, bitmap.Select0(37));
  EXPECT_EQ(40, bitmap.Select0(38));

  for (int i = 0; i < 40; ++i) BitmapIndex::Set(bm.get(), i);
  bitmap.BuildIndex(bm.get(), 40, enable_select_0_index_,
                    enable_select_1_index_);
  for (int i = 40; i > 0; --i) {
    EXPECT_EQ(i, bitmap.Rank1(i));
  }

  // Set some bit in each group of 8 bits to 0.
  for (int i = 0; i < 5; ++i) {
    BitmapIndex::Clear(bm.get(), 8 * i + i);
  }

  bitmap.BuildIndex(bm.get(), 40, enable_select_0_index_,
                    enable_select_1_index_);
  for (int i = 0; i < 5; ++i) {
    EXPECT_EQ(i, bitmap.Rank0(8 * i));
  }

  // Test with exactly a full final block.
  constexpr uint32_t kTwoBlocks = 1024;
  bm = std::make_unique<uint64_t[]>(BitmapIndex::StorageSize(kTwoBlocks));
  BitmapIndex::Set(bm.get(), 100);
  BitmapIndex::Set(bm.get(), 511);
  BitmapIndex::Set(bm.get(), 900);
  BitmapIndex::Set(bm.get(), 901);
  BitmapIndex::Set(bm.get(), 1023);
  bitmap.BuildIndex(bm.get(), kTwoBlocks, enable_select_0_index_,
                    enable_select_1_index_);
  EXPECT_EQ(5, bitmap.Rank1(kTwoBlocks));
  EXPECT_EQ(100, bitmap.Select1(0));
  EXPECT_EQ(511, bitmap.Select1(1));
  EXPECT_EQ(900, bitmap.Select1(2));
  EXPECT_EQ(901, bitmap.Select1(3));
  EXPECT_EQ(1023, bitmap.Select1(4));
}

TEST_P(BitmapIndexSelectTest, Select1) {
  const size_t num_bits = absl::GetFlag(FLAGS_bm_nthbit_bitmap_size);
  auto bm = std::make_unique<uint64_t[]>(BitmapIndex::StorageSize(num_bits));
  BitmapIndex map(bm.get(), num_bits, enable_select_0_index_,
                  enable_select_1_index_);
  for (uint32_t i = 17; i < map.Bits(); i += 23) {
    BitmapIndex::Set(bm.get(), i);
  }
  map.BuildIndex(bm.get(), num_bits, enable_select_0_index_,
                 enable_select_1_index_);
  uint32_t bits = map.GetOnesCount();
  for (int j = 0, i = 17; j < bits; ++j, i += 23) {
    EXPECT_EQ(i, map.Select1(j));
    EXPECT_EQ(j + 1, map.Rank1(i + 1));
  }
}

TEST_P(BitmapIndexSelectTest, Select0) {
  const size_t num_bits = absl::GetFlag(FLAGS_bm_nthbit_bitmap_size);
  auto bm = std::make_unique<uint64_t[]>(BitmapIndex::StorageSize(num_bits));
  BitmapIndex map(bm.get(), num_bits, enable_select_0_index_,
                  enable_select_1_index_);
  for (uint32_t i = 0; i < map.Bits(); ++i) {
    BitmapIndex::Set(bm.get(), i);
  }
  for (uint32_t i = 17; i < map.Bits(); i += 23) {
    BitmapIndex::Clear(bm.get(), i);
  }
  map.BuildIndex(bm.get(), num_bits, enable_select_0_index_,
                 enable_select_1_index_);
  uint32_t zeroes = map.Bits() - map.GetOnesCount();
  for (int j = 0, i = 17; j < zeroes; ++j, i += 23) {
    EXPECT_EQ(i, map.Select0(j));
    EXPECT_EQ(j + 1, map.Rank0(i + 1));
  }
}

TEST_P(BitmapIndexSelectTest, Select0s) {
  const size_t num_bits = absl::GetFlag(FLAGS_bm_nthbit_bitmap_size);
  auto bm = std::make_unique<uint64_t[]>(BitmapIndex::StorageSize(num_bits));
  std::fill_n(bm.get(), BitmapIndex::StorageSize(num_bits), -1);
  BitmapIndex::Clear(bm.get(), 0);
  BitmapIndex map;
  for (int j = 1; j < num_bits - 1; j += 117) {
    BitmapIndex::Clear(bm.get(), j);
    for (int i = j + 1; i < num_bits; i += 1123) {
      BitmapIndex::Clear(bm.get(), i);
      map.BuildIndex(bm.get(), num_bits, enable_select_0_index_,
                     enable_select_1_index_);
      std::pair<size_t, size_t> zeros = map.Select0s(0);
      EXPECT_EQ(zeros.first, map.Select0(0));
      EXPECT_EQ(zeros.second, map.Select0(1));
      BitmapIndex::Set(bm.get(), i);
    }
    BitmapIndex::Clear(bm.get(), j);
  }
}

TEST_P(BitmapIndexSelectTest, TestRankSelectZeros) {
  for (const int num_bits : {1, 31, 32, 33, 63, 64, 65, 511, 512, 513, 2000}) {
    auto bits =
        std::make_unique<uint64_t[]>(BitmapIndex::StorageSize(num_bits));

    BitmapIndex imap(&bits[0], num_bits, enable_select_0_index_,
                     enable_select_1_index_);

    EXPECT_EQ(0, imap.GetOnesCount());
    for (int i = 0; i < num_bits; ++i) {
      EXPECT_EQ(0, imap.Rank1(i)) << i << " " << num_bits;
      EXPECT_EQ(i, imap.Rank0(i)) << i << " " << num_bits;

      EXPECT_EQ(i, imap.Select0(i)) << i << " " << num_bits;
    }

    // This is a special case.
    EXPECT_EQ(num_bits, imap.Select1(0));
  }
}

// Set 1 in sparsity bits to 1.
void TestRankSelectWithSparsity(int sparsity, int num_bits,
                                bool enable_select_0_index,
                                bool enable_select_1_index) {
  CHECK_GT(sparsity, 0);
  auto bits = std::make_unique<uint64_t[]>(BitmapIndex::StorageSize(num_bits));
  for (int rank = 0; rank < num_bits / sparsity; ++rank) {
    BitmapIndex::Set(&bits[0], rank * sparsity);
  }

  BitmapIndex imap(&bits[0], num_bits, enable_select_0_index,
                   enable_select_1_index);

  EXPECT_EQ(num_bits / sparsity, imap.GetOnesCount());
  for (int i = 0; i < num_bits; ++i) {
    const int rank1 = std::min(i + sparsity - 1, num_bits) / sparsity;
    EXPECT_EQ(rank1, imap.Rank1(i)) << i << " " << sparsity << " " << num_bits;

    const int rank0 = i - rank1;
    EXPECT_EQ(rank0, imap.Rank0(i)) << i << " " << sparsity << " " << num_bits;

    if (i % sparsity == 0) {
      // This bit is a 1.
      if (rank1 < imap.GetOnesCount()) {
        EXPECT_EQ(i, imap.Select1(rank1))
            << rank1 << " " << sparsity << " " << num_bits;
      } else {
        EXPECT_EQ(num_bits, imap.Select1(rank1))
            << rank1 << " " << sparsity << " " << num_bits;
      }
    } else {
      // This bit is a 0.
      if (rank0 < num_bits - imap.GetOnesCount()) {
        EXPECT_EQ(i, imap.Select0(rank0))
            << rank0 << " " << sparsity << " " << num_bits;
      } else {
        EXPECT_EQ(num_bits, imap.Select0(rank0))
            << rank0 << " " << sparsity << " " << num_bits;
      }
    }
  }
}

TEST_P(BitmapIndexSelectTest, TestRankSelectWithSparsity) {
  for (const int num_bits : {1, 31, 32, 33, 63, 64, 65, 511, 512, 513, 2000}) {
    for (const int sparsity :
         {1, 2, 3, 4, 7, 8, 9, 15, 16, 17, 31, 32, 33, 63, 64}) {
      TestRankSelectWithSparsity(sparsity, num_bits, enable_select_0_index_,
                                 enable_select_1_index_);
    }
    for (const int sparsity : {10, 20, 30, 100, 200, 1000, 2000}) {
      TestRankSelectWithSparsity(sparsity, num_bits, enable_select_0_index_,
                                 enable_select_1_index_);
    }
  }
}

INSTANTIATE_TEST_SUITE_P(
    BitmapIndexSelectTests, BitmapIndexSelectTest,
    testing::Combine(testing::Bool(), testing::Bool()),
    [](const testing::TestParamInfo<BitmapIndexSelectTest::ParamType>& info) {
      return absl::StrCat(
          std::get<0>(info.param) ? "With" : "No", "Select0Index_",
          std::get<1>(info.param) ? "With" : "No", "Select1Index");
    });

}  // namespace
}  // namespace fst
