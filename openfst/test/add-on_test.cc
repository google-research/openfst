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
// Unit tests for add-on FST infrastructure.

#include "openfst/lib/add-on.h"

#include <istream>
#include <memory>
#include <ostream>
#include <sstream>

#include "gtest/gtest.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/util.h"
#include "openfst/lib/vector-fst.h"

namespace fst {
namespace {

// A simple add-on for testing that stores an integer.
class IntAddOn {
 public:
  explicit IntAddOn(int value) : value_(value) {}

  int Value() const { return value_; }

  static IntAddOn* Read(std::istream& strm, const FstReadOptions& opts) {
    int value;
    ReadType(strm, &value);
    return new IntAddOn(value);
  }

  bool Write(std::ostream& ostrm, const FstWriteOptions& opts) const {
    WriteType(ostrm, value_);
    return true;
  }

 private:
  int value_;
};

TEST(AddOnTest, NullAddOn) {
  NullAddOn addon;
  std::stringstream strm;
  FstWriteOptions w_opts;
  ASSERT_TRUE(addon.Write(strm, w_opts));

  FstReadOptions r_opts;
  std::unique_ptr<NullAddOn> read_addon(NullAddOn::Read(strm, r_opts));
  EXPECT_TRUE(read_addon != nullptr);
}

TEST(AddOnTest, AddOnPair) {
  auto a1 = std::make_shared<IntAddOn>(42);
  auto a2 = std::make_shared<IntAddOn>(24);
  AddOnPair<IntAddOn, IntAddOn> pair(a1, a2);

  EXPECT_EQ(pair.First()->Value(), 42);
  EXPECT_EQ(pair.Second()->Value(), 24);

  std::stringstream strm;
  FstWriteOptions w_opts;
  ASSERT_TRUE(pair.Write(strm, w_opts));

  FstReadOptions r_opts;
  std::unique_ptr<AddOnPair<IntAddOn, IntAddOn>> read_pair(
      AddOnPair<IntAddOn, IntAddOn>::Read(strm, r_opts));
  ASSERT_TRUE(read_pair != nullptr);
  EXPECT_EQ(read_pair->First()->Value(), 42);
  EXPECT_EQ(read_pair->Second()->Value(), 24);
}

TEST(AddOnTest, AddOnImpl) {
  VectorFst<StdArc> vfst;
  vfst.AddState();
  vfst.SetStart(0);
  vfst.SetFinal(0, StdArc::Weight::One());

  auto addon = std::make_shared<IntAddOn>(42);
  using Impl = internal::AddOnImpl<VectorFst<StdArc>, IntAddOn>;
  Impl impl(vfst, "addon_test", addon);

  EXPECT_EQ(impl.Start(), 0);
  EXPECT_EQ(impl.Final(0), StdArc::Weight::One());
  EXPECT_EQ(impl.GetAddOn()->Value(), 42);

  std::stringstream strm;
  FstWriteOptions w_opts;
  ASSERT_TRUE(impl.Write(strm, w_opts));

  FstReadOptions r_opts;
  FstHeader hdr;
  hdr.SetFstType("addon_test");
  hdr.SetArcType(StdArc::Type());
  hdr.SetVersion(1);

  strm.seekg(0);
  std::unique_ptr<Impl> read_impl(Impl::Read(strm, r_opts));
  ASSERT_TRUE(read_impl != nullptr);
  EXPECT_EQ(read_impl->Start(), 0);
  EXPECT_EQ(read_impl->Final(0), StdArc::Weight::One());
  EXPECT_EQ(read_impl->GetAddOn()->Value(), 42);
}

}  // namespace
}  // namespace fst
