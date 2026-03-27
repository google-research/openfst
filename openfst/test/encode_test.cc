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
// Unit test for Encode.

#include "openfst/lib/encode.h"

#include <memory>
#include <string>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/equal.h"
#include "openfst/lib/properties.h"
#include "openfst/lib/topsort.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/lib/verify.h"

namespace fst {
namespace {

using Arc = StdArc;
using StateId = Arc::StateId;
using Weight = Arc::Weight;
using Label = Arc::Label;

class EncodeTest : public testing::Test {
 protected:
  void SetUp() override {
    const std::string path =
        JoinPath(std::string("."),
                       "openfst/test/testdata/encode");
    const std::string encode1_name = JoinPath(path, "e1.fst");
    const std::string e_cd_name = JoinPath(path, "e1_cd.fst");
    const std::string d_cd_name = JoinPath(path, "delayed_e_cd.fst");

    pfst_.reset(VectorFst<Arc>::Read(encode1_name));
    pfst_cd_.reset(VectorFst<Arc>::Read(e_cd_name));
    dfst_cd_.reset(VectorFst<Arc>::Read(d_cd_name));
  }

  std::unique_ptr<const VectorFst<Arc>> pfst_;
  std::unique_ptr<const VectorFst<Arc>> pfst_cd_;
  std::unique_ptr<const VectorFst<Arc>> dfst_cd_;
};

TEST_F(EncodeTest, EncodeLabels) {
  VectorFst<Arc> nfst;

  VectorFst<Arc> vfst(*pfst_);

  EncodeMapper<Arc> encoder(kEncodeLabels);
  EXPECT_EQ(encoder.Type(), ENCODE);
  Encode(&vfst, &encoder);
  ASSERT_TRUE(Verify(vfst));
  ASSERT_TRUE(vfst.Properties(kAcceptor, false));

  Decode(&vfst, encoder);

  ASSERT_TRUE(Verify(vfst));
  ASSERT_FALSE(vfst.Properties(kAcceptor, false));
  ASSERT_TRUE(Equal(*pfst_, vfst));

  Encode(&nfst, &encoder);
  ASSERT_TRUE(Verify(nfst));
  ASSERT_TRUE(nfst.Properties(kAcceptor, false));
}

TEST_F(EncodeTest, EncodeWeights) {
  VectorFst<Arc> vfst(*pfst_);

  EncodeMapper<Arc> encoder(kEncodeWeights);
  Encode(&vfst, &encoder);
  ASSERT_FALSE(vfst.Properties(kAcceptor | kWeighted, false));
  ASSERT_EQ(kUnweighted | kUnweightedCycles,
            vfst.Properties(kUnweighted | kUnweightedCycles, false));
  ASSERT_EQ(kNotAcceptor, vfst.Properties(kNotAcceptor, true));
  ASSERT_TRUE(Verify(vfst));

  Decode(&vfst, encoder);
  ASSERT_TRUE(Verify(vfst));
  constexpr auto dprops = kNotAcceptor | kWeighted;
  ASSERT_EQ(dprops, vfst.Properties(dprops, true));
  ASSERT_TRUE(Equal(*pfst_cd_, vfst));
}

TEST_F(EncodeTest, EncodeLabelWeights) {
  VectorFst<Arc> vfst(*pfst_);

  EncodeMapper<Arc> encoder(kEncodeWeights | kEncodeLabels);
  Encode(&vfst, &encoder);
  ASSERT_TRUE(Verify(vfst));
  constexpr auto eprops = kAcceptor | kUnweighted | kUnweightedCycles;
  ASSERT_EQ(eprops, vfst.Properties(eprops, false));

  Decode(&vfst, encoder);
  ASSERT_TRUE(Verify(vfst));
  constexpr auto dprops = kNotAcceptor | kWeighted;
  ASSERT_EQ(dprops, vfst.Properties(dprops, true));
  ASSERT_TRUE(Equal(*pfst_cd_, vfst));
}

TEST_F(EncodeTest, EncodeLabelsFst) {
  VectorFst<Arc> nfst;

  EncodeMapper<Arc> encoder(kEncodeLabels);
  EncodeFst<Arc> dfst1(*pfst_, &encoder);
  ASSERT_TRUE(Verify(dfst1));
  ASSERT_TRUE(dfst1.Properties(kAcceptor, false));

  DecodeFst<Arc> dfst2(dfst1, encoder);
  ASSERT_FALSE(dfst2.Properties(kAcceptor, false));
  ASSERT_TRUE(Verify(dfst2));
  ASSERT_TRUE(Equal(*pfst_, dfst2));

  DecodeFst<Arc> ndfst(nfst, encoder);
  ASSERT_TRUE(Verify(ndfst));
  ASSERT_TRUE(ndfst.Properties(kAcceptor, false));
  ASSERT_TRUE(Equal(nfst, ndfst));
}

TEST_F(EncodeTest, EncodeWeightsFst) {
  EncodeMapper<Arc> encoder(kEncodeWeights);
  EncodeFst<Arc> dfst1(*pfst_, &encoder);
  VectorFst<Arc> vfst1(dfst1);
  TopSort(&vfst1);
  ASSERT_TRUE(Verify(dfst1));
  ASSERT_EQ(kUnweighted | kUnweightedCycles,
            vfst1.Properties(kUnweighted | kUnweightedCycles, false));
  ASSERT_EQ(kNotAcceptor, vfst1.Properties(kNotAcceptor, true));

  DecodeFst<Arc> dfst2(dfst1, encoder);
  VectorFst<Arc> vfst2(dfst2);
  TopSort(&vfst2);
  ASSERT_TRUE(Verify(dfst2));
  ASSERT_TRUE(Equal(*dfst_cd_, vfst2));
}

TEST_F(EncodeTest, EncodeLabelWeightsFst) {
  EncodeMapper<Arc> encoder(kEncodeLabels | kEncodeWeights);
  EncodeFst<Arc> dfst1(*pfst_, &encoder);
  VectorFst<Arc> vfst1(dfst1);
  TopSort(&vfst1);
  ASSERT_TRUE(Verify(dfst1));
  constexpr auto props = kAcceptor | kUnweighted | kUnweightedCycles;
  ASSERT_EQ(props, dfst1.Properties(props, false));

  DecodeFst<Arc> dfst2(vfst1, encoder);
  VectorFst<Arc> vfst2(dfst2);
  TopSort(&vfst2);
  ASSERT_TRUE(Verify(dfst2));
  ASSERT_TRUE(Equal(*dfst_cd_, vfst2));

  for (const bool safe : {false, true}) {
    DecodeFst<Arc> cfst(dfst2, safe);
    VectorFst<Arc> vfst3(cfst);
    TopSort(&vfst3);
    ASSERT_TRUE(Verify(dfst2));
    ASSERT_TRUE(Equal(*dfst_cd_, vfst3));
  }
}

TEST_F(EncodeTest, EncodeDeterminismTest) {
  VectorFst<Arc> fst;
  const auto s = fst.AddState();
  fst.SetStart(s);
  fst.EmplaceArc(s, 0, 0, Weight(1.0f), s);
  fst.SetFinal(s, Weight(1.0f));

  EXPECT_EQ(kIDeterministic, fst.Properties(kIDeterministic, true));
  EXPECT_EQ(1, fst.NumStates());

  EncodeMapper<Arc> encoder(kEncodeLabels | kEncodeWeights);
  Encode(&fst, &encoder);
  constexpr auto eprops =
      kIDeterministic | kUnweighted | kUnweightedCycles | kAcceptor;
  EXPECT_EQ(eprops, fst.Properties(eprops, false));
  EXPECT_EQ(2, fst.NumStates());

  Decode(&fst, encoder);
  constexpr auto dprops = kIDeterministic | kWeighted;
  EXPECT_EQ(dprops, fst.Properties(dprops, true));
  EXPECT_EQ(1, fst.NumStates());
}

TEST_F(EncodeTest, EncodeIOTest) {
  EncodeMapper<Arc> encoder1(kEncodeLabels | kEncodeWeights);
  EncodeFst<Arc> dfst1(*pfst_, &encoder1);
  VectorFst<Arc> vfst1(dfst1);
  TopSort(&vfst1);
  ASSERT_TRUE(Verify(dfst1));
  constexpr auto eprops = kAcceptor | kUnweighted | kUnweightedCycles;
  ASSERT_EQ(eprops, dfst1.Properties(eprops, false));

  const std::string filename =
      JoinPath(::testing::TempDir(), "test.codex");
  encoder1.Write(filename);

  std::unique_ptr<EncodeMapper<Arc>> encoder2(
      EncodeMapper<Arc>::Read(filename));
  DecodeFst<Arc> dfst2(dfst1, *encoder2);
  VectorFst<Arc> vfst2(dfst2);
  TopSort(&vfst2);
  ASSERT_TRUE(Verify(dfst2));
  ASSERT_TRUE(Equal(*dfst_cd_, vfst2));
}

}  // namespace
}  // namespace fst
