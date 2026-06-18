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
// Unit tests for FST data validation during Read.

#include <cstdint>
#include <cstring>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>

#include "gtest/gtest.h"
#include "absl/memory/memory.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/compact-fst.h"
#include "openfst/lib/const-fst.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/test/basic-fst.h"

namespace fst {
namespace {

// Helper to write an Fst to string and return it.
template <class F>
std::string WriteFstToString(const F& fst) {
  std::ostringstream strm;
  fst.Write(strm, FstWriteOptions("string"));
  return strm.str();
}

TEST(ValidationTest, BasicFstStartOutOfRange) {
  VectorFst<StdArc> vfst;
  vfst.AddState();
  vfst.SetStart(0);

  std::string data = WriteFstToString(vfst);

  FstHeader hdr;
  std::istringstream istrm(data);
  ASSERT_TRUE(hdr.Read(istrm, "string"));

  hdr.SetStart(100);
  hdr.SetFstType("basic");  // Match BasicFst type

  std::ostringstream ostrm;
  hdr.Write(ostrm, "string");
  ostrm << istrm.rdbuf();

  std::istringstream malformed_strm(ostrm.str());
  FstReadOptions opts;
  opts.verify = true;

  auto basic_fst =
      absl::WrapUnique(BasicFst<StdArc>::Read(malformed_strm, opts));
  EXPECT_EQ(basic_fst, nullptr);
}

TEST(ValidationTest, BasicFstNextStateOutOfRange) {
  VectorFst<StdArc> vfst;
  vfst.AddState();
  vfst.SetStart(0);
  vfst.AddArc(0, StdArc(1, 1, 0, 0));

  std::string data = WriteFstToString(vfst);

  // For BasicFst, we also need to change the type in the header.
  FstHeader hdr;
  std::istringstream istrm(data);
  ASSERT_TRUE(hdr.Read(istrm, "string"));
  hdr.SetFstType("basic");

  std::ostringstream ostrm;
  hdr.Write(ostrm, "string");
  std::string rest_of_data =
      std::string(std::istreambuf_iterator<char>(istrm), {});

  // Corrupt the nextstate (last 4 bytes of the arc).
  // StdArc::StateId is usually int32_t.
  ASSERT_GE(rest_of_data.size(), sizeof(int32_t));
  int32_t bad_nextstate = 1;
  std::memcpy(&rest_of_data[rest_of_data.size() - sizeof(int32_t)],
              &bad_nextstate, sizeof(int32_t));

  ostrm << rest_of_data;

  std::istringstream malformed_strm(ostrm.str());
  FstReadOptions opts;
  opts.verify = true;

  auto basic_fst =
      absl::WrapUnique(BasicFst<StdArc>::Read(malformed_strm, opts));
  EXPECT_EQ(basic_fst, nullptr);
}

TEST(ValidationTest, ConstFstNextStateOutOfRange) {
  VectorFst<StdArc> vfst;
  vfst.AddState();
  vfst.SetStart(0);
  vfst.AddArc(0, StdArc(1, 1, 0, 0));

  ConstFst<StdArc> cfst(vfst);
  std::string data = WriteFstToString(cfst);

  // ConstFst arcs are at the end. Corrupt the nextstate of the last arc.
  std::string malformed_data = data;
  ASSERT_GE(malformed_data.size(), sizeof(int32_t));
  int32_t bad_nextstate = 1;
  std::memcpy(&malformed_data[malformed_data.size() - sizeof(int32_t)],
              &bad_nextstate, sizeof(int32_t));

  std::istringstream malformed_strm(malformed_data);
  FstReadOptions opts;
  opts.verify = true;

  auto const_fst =
      absl::WrapUnique(ConstFst<StdArc>::Read(malformed_strm, opts));
  EXPECT_EQ(const_fst, nullptr);
}

TEST(ValidationTest, CompactFstStartOutOfRange) {
  VectorFst<StdArc> vfst;
  vfst.AddState();
  vfst.SetStart(0);

  CompactAcceptorFst<StdArc> compact_fst(vfst);
  std::string data = WriteFstToString(compact_fst);

  FstHeader hdr;
  std::istringstream istrm(data);
  ASSERT_TRUE(hdr.Read(istrm, "string"));

  hdr.SetStart(100);

  std::ostringstream ostrm;
  hdr.Write(ostrm, "string");
  ostrm << istrm.rdbuf();

  std::istringstream malformed_strm(ostrm.str());
  FstReadOptions opts;
  opts.verify = true;

  auto read_fst =
      absl::WrapUnique(CompactAcceptorFst<StdArc>::Read(malformed_strm, opts));
  EXPECT_EQ(read_fst, nullptr);
}

}  // namespace
}  // namespace fst
