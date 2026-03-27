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
// Unit test for Equal.

#include "openfst/lib/equal.h"

#include <memory>
#include <string>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/const-fst.h"
#include "openfst/lib/properties.h"
#include "openfst/lib/symbol-table.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/lib/weight.h"

namespace fst {
namespace {

using Arc = StdArc;
using StateId = Arc::StateId;
using Weight = Arc::Weight;
using Label = Arc::Label;

class EqualTest : public testing::Test {
 protected:
  void SetUp() override {
    const std::string i1_name = JoinPath(
        std::string("."),
        "openfst/test/testdata/intersect/i1.fst");
    fst_.reset(VectorFst<Arc>::Read(i1_name));
  }

  std::unique_ptr<const VectorFst<Arc>> fst_;
};

TEST_F(EqualTest, MismatchedStart) {
  VectorFst<Arc> fst(*fst_);
  fst.SetStart(1);
  ASSERT_FALSE(Equal(*fst_, fst));
  ASSERT_FALSE(Equal(*fst_, fst, kDelta, kEqualAll));
  ASSERT_TRUE(Equal(*fst_, fst, kDelta, kEqualAll & ~kEqualFsts));
}

// Test version with a double arg.  This is actually the common case, since
// floating points literals are doubles if they don't have the "f" suffix.
TEST_F(EqualTest, MismatchedStart_DoubleDelta) {
  VectorFst<Arc> fst(*fst_);
  fst.SetStart(1);
  ASSERT_FALSE(Equal(*fst_, fst));
  ASSERT_FALSE(Equal(*fst_, fst, 1.0 * kDelta, kEqualAll));
  ASSERT_TRUE(Equal(*fst_, fst, 1.0 * kDelta, kEqualAll & ~kEqualFsts));
}

// Use version with WeightEqual comparator, not delta.
TEST_F(EqualTest, MismatchedStart_WeightEqual) {
  VectorFst<Arc> fst(*fst_);
  fst.SetStart(1);
  ASSERT_FALSE(Equal(*fst_, fst));
  ASSERT_FALSE(Equal(*fst_, fst, WeightApproxEqual(kDelta), kEqualAll));
  ASSERT_TRUE(
      Equal(*fst_, fst, WeightApproxEqual(kDelta), kEqualAll & ~kEqualFsts));
}

TEST_F(EqualTest, MismatchedFinal) {
  VectorFst<Arc> fst(*fst_);
  fst.SetFinal(1, Weight::One());
  ASSERT_FALSE(Equal(*fst_, fst));
  ASSERT_FALSE(Equal(*fst_, fst, kDelta, kEqualAll));
  ASSERT_TRUE(Equal(*fst_, fst, kDelta, kEqualAll & ~kEqualFsts));
}

TEST_F(EqualTest, MismatchedArc) {
  VectorFst<Arc> fst(*fst_);
  fst.AddArc(1, Arc(0, 0, Weight::One(), 3));
  ASSERT_FALSE(Equal(*fst_, fst));
  ASSERT_FALSE(Equal(*fst_, fst, kDelta, kEqualAll));
  ASSERT_TRUE(Equal(*fst_, fst, kDelta, kEqualAll & ~kEqualFsts));
}

TEST_F(EqualTest, MismatchedType) {
  ConstFst<Arc> fst(*fst_);
  ASSERT_FALSE(Equal(*fst_, fst, kDelta, kEqualFstTypes));
  ASSERT_FALSE(Equal(*fst_, fst, kDelta, kEqualAll));
  ASSERT_TRUE(Equal(*fst_, fst, kDelta, kEqualAll & ~kEqualFstTypes));
}

TEST_F(EqualTest, IncompatibleProperties) {
  ConstFst<Arc> fst1(*fst_);
  VectorFst<Arc> fst2(fst1);  // Forces copy of impl.
  fst2.SetProperties(kNotAcceptor, kAcceptor | kNotAcceptor);
  ASSERT_FALSE(Equal(*fst_, fst2, kDelta, kEqualCompatProperties));
  ASSERT_FALSE(Equal(*fst_, fst2, kDelta, kEqualAll));
  ASSERT_TRUE(Equal(*fst_, fst2, kDelta, kEqualAll & ~kEqualCompatProperties));
}

TEST_F(EqualTest, IncompatibleSymbols) {
  SymbolTable syms1;
  syms1.AddSymbol("foo", 1);
  SymbolTable syms2;
  syms2.AddSymbol("bar", 1);
  VectorFst<Arc> fst1(*fst_);
  fst1.SetInputSymbols(&syms1);
  VectorFst<Arc> fst2(*fst_);
  fst2.SetOutputSymbols(&syms2);
  ASSERT_TRUE(Equal(fst1, fst2, kDelta, kEqualCompatSymbols));
  fst1.SetOutputSymbols(&syms1);
  fst2.SetInputSymbols(&syms2);
  ASSERT_FALSE(Equal(fst1, fst2, kDelta, kEqualCompatSymbols));
  fst2.SetInputSymbols(&syms1);
  fst2.SetOutputSymbols(&syms1);
  ASSERT_TRUE(Equal(fst1, fst2, kDelta, kEqualCompatSymbols));
}

}  // namespace
}  // namespace fst
