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

#include "openfst/extensions/ngram/nthbit.h"

#if !defined(__BMI2__) && SIZE_MAX == UINT32_MAX

#if __cplusplus >= 202002L
#include <bit>
#endif
#include <cstdint>

namespace fst {
namespace {

// Returns the number of set bits in v.  This is a slow implementation that
// should only be used for constructing constexpr tables.
constexpr int SlowPopcount(uint8_t v) {
#if __cplusplus >= 202002L
  return std::popcount(v);
#else
  int c = 0;
  for (; v != 0; v &= v - 1) c++;
  return c;
#endif
}

// Generates a table mapping a byte to its popcount.
constexpr std::array<uint8_t, 256> GeneratePopcountTable() {
  std::array<uint8_t, 256> table{};
  for (int i = 0; i < 256; ++i) {
    table[i] = SlowPopcount(static_cast<uint8_t>(i));
  }
  return table;
}

// Generates a 2D table mapping [rank][byte] to the bit position.
// If the rank is out of bounds for the byte, 255 is stored.
constexpr std::array<std::array<uint8_t, 256>, 8> GenerateBitPosTable2D() {
  std::array<std::array<uint8_t, 256>, 8> table{};
  for (int r = 0; r < 8; ++r) {
    for (int v = 0; v < 256; ++v) {
      int pos = fst_internal::SlowSelectInByte(static_cast<uint8_t>(v), r);
      table[r][v] = (pos == -1) ? 255 : static_cast<uint8_t>(pos);
    }
  }
  return table;
}

constexpr std::array<uint8_t, 256> kBitCount = GeneratePopcountTable();
constexpr std::array<std::array<uint8_t, 256>, 8> kBitPos =
    GenerateBitPosTable2D();

}  // namespace

// Returns the position (0-63) of the r-th 1 bit in v.
// 0 <= r < CountOnes(v) <= 64. Therefore, v must not be 0.
int nth_bit(uint64_t v, uint32_t r) {
  DCHECK_NE(v, 0);
  DCHECK_LE(0, r);
  DCHECK_LT(r, absl::popcount(v));

  uint32_t next_byte = v & 255;
  uint32_t byte_popcount = kBitCount[next_byte];
  if (r < byte_popcount) return kBitPos[r][next_byte];
  r -= byte_popcount;

  next_byte = (v >> 8) & 255;
  byte_popcount = kBitCount[next_byte];
  if (r < byte_popcount) return 8 + kBitPos[r][next_byte];
  r -= byte_popcount;

  next_byte = (v >> 16) & 255;
  byte_popcount = kBitCount[next_byte];
  if (r < byte_popcount) return 16 + kBitPos[r][next_byte];
  r -= byte_popcount;

  next_byte = (v >> 24) & 255;
  byte_popcount = kBitCount[next_byte];
  if (r < byte_popcount) return 24 + kBitPos[r][next_byte];
  r -= byte_popcount;

  next_byte = (v >> 32) & 255;
  byte_popcount = kBitCount[next_byte];
  if (r < byte_popcount) return 32 + kBitPos[r][next_byte];
  r -= byte_popcount;

  next_byte = (v >> 40) & 255;
  byte_popcount = kBitCount[next_byte];
  if (r < byte_popcount) return 40 + kBitPos[r][next_byte];
  r -= byte_popcount;

  next_byte = (v >> 48) & 255;
  byte_popcount = kBitCount[next_byte];
  if (r < byte_popcount) return 48 + kBitPos[r][next_byte];
  r -= byte_popcount;

  next_byte = (v >> 56) & 255;
  byte_popcount = kBitCount[next_byte];
  if (r < byte_popcount) return 56 + kBitPos[r][next_byte];
  return -1;
}

}  // namespace fst

#endif
