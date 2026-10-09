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

#include <cstddef>
#include <ios>
#include <istream>
#include <memory>
#include <ostream>
#include <sstream>
#include <streambuf>
#include <string>
#include <vector>

#include "gtest/gtest.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/matcher-fst.h"
#include "openfst/lib/matcher.h"
#include "openfst/lib/util.h"
#include "openfst/lib/vector-fst.h"

namespace fst {
namespace {

inline constexpr char kTestMatcherFstName[] = "test_matcher_fst";

// A simple add-on for testing that stores an integer. Like the add-ons in the
// library (e.g. `LabelReachableData`), it does not check the stream state.
class IntAddOn {
 public:
  explicit IntAddOn(int value, bool fail_write = false)
      : value_(value), fail_write_(fail_write) {}

  int Value() const { return value_; }

  static IntAddOn* Read(std::istream& strm, const FstReadOptions& opts) {
    int value = 0;
    ReadType(strm, &value);
    return new IntAddOn(value);
  }

  bool Write(std::ostream& ostrm, const FstWriteOptions& opts) const {
    if (fail_write_) return false;
    WriteType(ostrm, value_);
    return true;
  }

 private:
  int value_;
  bool fail_write_;
};

// Output stream buffer that accepts at most `capacity` bytes; further writes
// fail and set `badbit` on the stream.
class LimitedStreamBuf : public std::streambuf {
 public:
  explicit LimitedStreamBuf(size_t capacity) : buffer_(capacity) {
    setp(buffer_.data(), buffer_.data() + buffer_.size());
  }

 private:
  std::vector<char> buffer_;
};

class IntAddOnMatcher : public SortedMatcher<StdVectorFst> {
 public:
  using FST = StdVectorFst;
  using Arc = StdArc;
  using MatcherData = IntAddOn;

  IntAddOnMatcher(const FST& fst, MatchType match_type,
                  std::shared_ptr<MatcherData> data = nullptr)
      : SortedMatcher<StdVectorFst>(fst, match_type) {}

  IntAddOnMatcher(const FST* fst, MatchType match_type,
                  std::shared_ptr<MatcherData> data = nullptr)
      : SortedMatcher<StdVectorFst>(fst, match_type) {}
};

using TestMatcherFst =
    MatcherFst<StdVectorFst, IntAddOnMatcher, kTestMatcherFstName>;

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
  using Pair = AddOnPair<IntAddOn, IntAddOn>;
  Pair pair(a1, a2);

  EXPECT_EQ(pair.First()->Value(), 42);
  EXPECT_EQ(pair.Second()->Value(), 24);

  std::stringstream strm;
  FstWriteOptions w_opts;
  ASSERT_TRUE(pair.Write(strm, w_opts));
  const std::string serialized = strm.str();

  FstReadOptions r_opts;
  std::unique_ptr<Pair> read_pair(Pair::Read(strm, r_opts));
  ASSERT_TRUE(read_pair != nullptr);
  EXPECT_EQ(read_pair->First()->Value(), 42);
  EXPECT_EQ(read_pair->Second()->Value(), 24);

  // Every truncated prefix of a two-element AddOnPair must fail to read.
  for (size_t len = 0; len < serialized.size(); ++len) {
    std::istringstream truncated(serialized.substr(0, len));
    EXPECT_EQ(Pair::Read(truncated, r_opts), nullptr) << "len=" << len;
  }

  // Write failures from either child or the stream itself must return false.
  Pair fail_first(std::make_shared<IntAddOn>(1, /*fail_write=*/true), a2);
  std::ostringstream out1;
  EXPECT_FALSE(fail_first.Write(out1, w_opts));

  Pair fail_second(a1, std::make_shared<IntAddOn>(2, /*fail_write=*/true));
  std::ostringstream out2;
  EXPECT_FALSE(fail_second.Write(out2, w_opts));

  std::ostringstream bad_out;
  bad_out.setstate(std::ios::badbit);
  EXPECT_FALSE(pair.Write(bad_out, w_opts));
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
  const std::string serialized = strm.str();

  FstReadOptions r_opts;
  strm.seekg(0);
  std::unique_ptr<Impl> read_impl(Impl::Read(strm, r_opts));
  ASSERT_TRUE(read_impl != nullptr);
  EXPECT_EQ(read_impl->Start(), 0);
  EXPECT_EQ(read_impl->Final(0), StdArc::Weight::One());
  ASSERT_NE(read_impl->GetAddOn(), nullptr);
  EXPECT_EQ(read_impl->GetAddOn()->Value(), 42);

  // Every truncated prefix of a serialized AddOnImpl must fail to read.
  for (size_t len = 0; len < serialized.size(); ++len) {
    std::istringstream truncated(serialized.substr(0, len));
    EXPECT_EQ(Impl::Read(truncated, r_opts), nullptr) << "len=" << len;
  }

  // Write failures from the add-on must return false.
  Impl fail_impl(vfst, "addon_test",
                 std::make_shared<IntAddOn>(42, /*fail_write=*/true));
  std::ostringstream out;
  EXPECT_FALSE(fail_impl.Write(out, w_opts));

  // Every write cut off before the end of the serialization must fail, even
  // when the cut lands in the add-on, whose `Write` ignores the stream state.
  for (size_t len = 0; len < serialized.size(); ++len) {
    LimitedStreamBuf buf(len);
    std::ostream limited(&buf);
    EXPECT_FALSE(impl.Write(limited, w_opts)) << "len=" << len;
  }
}

TEST(AddOnTest, MatcherFstNullAddOnData) {
  TestMatcherFst default_mfst;
  EXPECT_EQ(default_mfst.GetAddOn(), nullptr);
  EXPECT_EQ(default_mfst.GetData(MATCH_INPUT), nullptr);
  EXPECT_EQ(default_mfst.GetData(MATCH_OUTPUT), nullptr);
  EXPECT_EQ(default_mfst.GetSharedData(MATCH_INPUT), nullptr);
  EXPECT_EQ(default_mfst.GetSharedData(MATCH_OUTPUT), nullptr);
  std::unique_ptr<IntAddOnMatcher> matcher(
      default_mfst.InitMatcher(MATCH_INPUT));
  EXPECT_NE(matcher, nullptr);
}

}  // namespace
}  // namespace fst
