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
// Unit test for FST file formats.

#include <ios>
#include <memory>
#include <ostream>
#include <sstream>
#include <string>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "absl/log/log.h"
#include "absl/strings/string_view.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/const-fst.h"
#include "openfst/lib/equal.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/symbol-table.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/lib/verify.h"

namespace fst {
namespace {

using Arc = StdArc;

class FstFileTest : public testing::Test {
 protected:
  void SetUp() override {
    const std::string path =
        JoinPath(std::string("."),
                       "openfst/test/testdata/fst-file");
    vector1_name_ = JoinPath(path, "vector1.fst");
    vector2_name_ = JoinPath(path, "vector2.fst");
    vector2_uns_name_ = JoinPath(path, "vector2_uns.fst");
    const1_name_ = JoinPath(path, "const1.fst");
    const1a_name_ = JoinPath(path, "const1a.fst");
    const2_name_ = JoinPath(path, "const2.fst");
    compact1_name_ = JoinPath(path, "compact1.fst");
    compact1a_name_ = JoinPath(path, "compact1a.fst");
    compact2_name_ = JoinPath(path, "compact2.fst");
  }

  void FstTest(absl::string_view file) const {
    std::unique_ptr<const Fst<Arc>> fst(Fst<Arc>::Read(file));
    EXPECT_TRUE(Verify(*fst));
  }

  void VectorFstTest() const {
    std::unique_ptr<const Fst<Arc>> fst(Fst<Arc>::Read(vector2_name_));
    std::ostringstream oss;
    VectorFst<Arc>::WriteFst(*fst, oss, FstWriteOptions());
    std::istringstream iss(oss.str());
    std::unique_ptr<const Fst<Arc>> fst2(Fst<Arc>::Read(iss, FstReadOptions()));
    EXPECT_TRUE(Verify(*fst2));
    EXPECT_TRUE(Equal(*fst, *fst2));
    EXPECT_EQ(fst2->Type(), "vector");
  }

  void VectorAsConstFstTest() const {
    std::unique_ptr<const Fst<Arc>> fst(Fst<Arc>::Read(vector2_name_));
    std::ostringstream oss;
    ConstFst<Arc>::WriteFst(*fst, oss, FstWriteOptions());
    std::istringstream iss(oss.str());
    std::unique_ptr<const Fst<Arc>> fst2(Fst<Arc>::Read(iss, FstReadOptions()));
    EXPECT_TRUE(Verify(*fst2));
    EXPECT_TRUE(Equal(*fst, *fst2));
    EXPECT_EQ(fst2->Type(), "const");
  }

  void ConstFstTest() const {
    std::unique_ptr<const Fst<Arc>> fst(Fst<Arc>::Read(vector2_name_));
    std::ostringstream oss;
    VectorFst<Arc> vfst(*fst);
    ConstFst<Arc>::WriteFst(vfst, oss, FstWriteOptions());
    std::istringstream iss(oss.str());
    std::unique_ptr<const Fst<Arc>> fst2(Fst<Arc>::Read(iss, FstReadOptions()));
    EXPECT_TRUE(Verify(*fst2));
    EXPECT_TRUE(Equal(*fst, *fst2));
    EXPECT_EQ(fst2->Type(), "const");
  }

  void ConstAsVectorFstTest() const {
    std::unique_ptr<const Fst<Arc>> fst(Fst<Arc>::Read(vector2_name_));
    std::ostringstream oss;
    ConstFst<Arc> cfst(*fst);
    VectorFst<Arc>::WriteFst(cfst, oss, FstWriteOptions());
    std::istringstream iss(oss.str());
    std::unique_ptr<const Fst<Arc>> fst2(Fst<Arc>::Read(iss, FstReadOptions()));
    EXPECT_TRUE(Verify(*fst2));
    EXPECT_TRUE(Equal(*fst, *fst2));
    EXPECT_EQ(fst2->Type(), "vector");
  }

  std::string vector1_name_;
  std::string vector2_name_;
  std::string vector2_uns_name_;
  std::string const1_name_;
  std::string const1a_name_;
  std::string const2_name_;
  std::string compact1_name_;
  std::string compact1a_name_;
  std::string compact2_name_;
};

// Tests VectorFst, version 1.
TEST_F(FstFileTest, Vector1Test) { FstTest(vector1_name_); }

// Tests VectorFst, version 2.
TEST_F(FstFileTest, Vector2Test) { FstTest(vector2_name_); }

// Tests VectorFst, version 2, with unknown number of states.
TEST_F(FstFileTest, Vector2UnknownNumStatesTest) { FstTest(vector2_uns_name_); }

// Tests ConstFst, version 1, IS_ALIGNED bit unset
// This is the original version 1 format.
TEST_F(FstFileTest, Const1Test) { FstTest(const1_name_); }

// Tests ConstFst, version 1, IS_ALIGNED bit set.
// This is a modified version 1 format.
TEST_F(FstFileTest, Const1ATest) { FstTest(const1a_name_); }

// Tests ConstFst, version 2.
TEST_F(FstFileTest, Const2Test) { FstTest(const2_name_); }

// Tests CompactFst, version 1, IS_ALIGNED bit unset.
// This is the original version 1 format.
TEST_F(FstFileTest, Compact1Test) { FstTest(compact1_name_); }

// Tests CompactFst, version 1, IS_ALIGNED bit set.
// This is a modified version 1 format.
TEST_F(FstFileTest, Compact1ATest) { FstTest(compact1a_name_); }

// Tests CompactFst, version 2.
TEST_F(FstFileTest, Compact2Test) { FstTest(compact2_name_); }

// Test serializing cross-type to Vector.
TEST_F(FstFileTest, WriteAsVector) { VectorFstTest(); }

// Test serializing cross-type to Const.
TEST_F(FstFileTest, WriteAsConst) { ConstFstTest(); }

TEST_F(FstFileTest, WriteVectorAsConst) { VectorAsConstFstTest(); }

TEST_F(FstFileTest, WriteConstAsVector) { ConstAsVectorFstTest(); }

TEST_F(FstFileTest, StripSymbolsTest) {
  std::ostringstream oss;
  {
    std::unique_ptr<VectorFst<Arc>> fst(VectorFst<Arc>::Read(vector2_name_));
    SymbolTable syms;
    fst->SetInputSymbols(&syms);
    fst->SetOutputSymbols(&syms);
    VectorFst<Arc>::WriteFst(*fst, oss, FstWriteOptions());
  }
  {
    std::istringstream iss(oss.str());
    FstReadOptions opts;
    opts.read_isymbols = false;
    std::unique_ptr<const Fst<Arc>> fst2(Fst<Arc>::Read(iss, opts));
    EXPECT_TRUE(fst2->InputSymbols() == nullptr);
    EXPECT_FALSE(fst2->OutputSymbols() == nullptr);
  }
  {
    std::istringstream iss(oss.str());
    FstReadOptions opts;
    opts.read_osymbols = false;
    std::unique_ptr<const Fst<Arc>> fst2(Fst<Arc>::Read(iss, opts));
    EXPECT_FALSE(fst2->InputSymbols() == nullptr);
    EXPECT_TRUE(fst2->OutputSymbols() == nullptr);
  }
}

class noseekstreambuf : public std::basic_stringbuf<char> {
 protected:
  pos_type seekpos(pos_type pos, std::ios_base::openmode) override {
    LOG(FATAL) << "Can't seek a no seek stream.";
  }
};

class noseekostream : public std::ostream {
 public:
  noseekostream() : std::ostream(&noseekstreambuf_) {}

  noseekstreambuf noseekstreambuf_;
};

TEST_F(FstFileTest, NoSeek) {
  std::unique_ptr<const Fst<Arc>> fst(Fst<Arc>::Read(vector2_name_));
  noseekostream oss;
  FstWriteOptions opts;
  opts.stream_write = true;
  ConstFst<Arc>::WriteFst(*fst, oss, opts);
}

}  // namespace
}  // namespace fst
