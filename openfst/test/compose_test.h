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
// Unit test for Compose.

#ifndef OPENFST_TEST_COMPOSE_TEST_H_
#define OPENFST_TEST_COMPOSE_TEST_H_

#include <memory>
#include <string>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "openfst/lib/cache.h"
#include "openfst/lib/compose-filter.h"
#include "openfst/lib/compose.h"
#include "openfst/lib/const-fst.h"
#include "openfst/lib/equal.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/matcher.h"
#include "openfst/lib/properties.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/lib/verify.h"
#include "openfst/script/compose.h"
#include "openfst/script/equal.h"
#include "openfst/script/fst-class.h"

namespace fst {

template <class F>
class ComposeTest : public testing::Test {
 protected:
  using TestFst = F;
  using Arc = typename TestFst::Arc;

  void SetUp() override {
    const std::string path =
        JoinPath(std::string("."),
                       "openfst/test/testdata/compose");
    const std::string compose1_name = JoinPath(path, "c1.fst");
    const std::string compose2_name = JoinPath(path, "c2.fst");
    const std::string compose3_name = JoinPath(path, "c3.fst");
    const std::string compose4_name = JoinPath(path, "c4.fst");
    const std::string compose5_name = JoinPath(path, "c5.fst");
    const std::string compose6_name = JoinPath(path, "c6.fst");
    const std::string compose7_name = JoinPath(path, "c7.fst");
    const std::string compose8_name = JoinPath(path, "c8.fst");

    cfst1_.reset(VectorFst<Arc>::Read(compose1_name));
    cfst2_.reset(VectorFst<Arc>::Read(compose2_name));
    // cfst3_ = ComposeFst(cfst1_, cfst2_)
    cfst3_.reset(VectorFst<Arc>::Read(compose3_name));
    cfst4_.reset(VectorFst<Arc>::Read(compose4_name));
    cfst5_.reset(VectorFst<Arc>::Read(compose5_name));
    // cfst6_ = ComposeFst(cfst4_, cfst5_)
    cfst6_.reset(VectorFst<Arc>::Read(compose6_name));
    // cfst7_ = ComposeFst(cfst4_, cfst5_) w/ null compose filter.
    cfst7_.reset(VectorFst<Arc>::Read(compose7_name));
    cfst8_.reset(VectorFst<Arc>::Read(compose8_name));
  }

  std::unique_ptr<const VectorFst<Arc>> cfst1_;
  std::unique_ptr<const VectorFst<Arc>> cfst2_;
  std::unique_ptr<const VectorFst<Arc>> cfst3_;
  std::unique_ptr<const VectorFst<Arc>> cfst4_;
  std::unique_ptr<const VectorFst<Arc>> cfst5_;
  std::unique_ptr<const VectorFst<Arc>> cfst6_;
  std::unique_ptr<const VectorFst<Arc>> cfst7_;
  std::unique_ptr<const VectorFst<Arc>> cfst8_;
};

TYPED_TEST_SUITE_P(ComposeTest);

TYPED_TEST_P(ComposeTest, ComposeFstWithoutEpsilons) {
  using Arc = typename TestFixture::Arc;
  using TestFst = typename TestFixture::TestFst;

  const TestFst tfst1(*this->cfst1_);
  const TestFst tfst2(*this->cfst2_);

  VectorFst<Arc> vfst1_u(*this->cfst1_);
  VectorFst<Arc> vfst2_u(*this->cfst2_);

  // Clear sort properties.
  vfst1_u.SetProperties(0, kOLabelSorted);
  vfst2_u.SetProperties(0, kILabelSorted);

  const ComposeFst<Arc> dfst1(tfst1, tfst2);
  EXPECT_TRUE(Verify(dfst1));
  EXPECT_TRUE(Equal(*this->cfst3_, dfst1));

  const ComposeFst<Arc> dfst2(vfst1_u, tfst2);
  EXPECT_TRUE(Verify(dfst2));
  EXPECT_TRUE(Equal(*this->cfst3_, dfst2));

  const ComposeFst<Arc> dfst3(tfst1, vfst2_u);
  EXPECT_TRUE(Verify(dfst3));
  EXPECT_TRUE(Equal(*this->cfst3_, dfst3));
}

TYPED_TEST_P(ComposeTest, FstClassCompose) {
  using Arc = typename TestFixture::Arc;
  using TestFst = typename TestFixture::TestFst;
  namespace s = fst::script;

  const TestFst tfst1(*this->cfst1_);
  const TestFst tfst2(*this->cfst2_);
  const TestFst tfst3(*this->cfst3_);

  VectorFst<Arc> vfst1_u(*this->cfst1_);
  VectorFst<Arc> vfst2_u(*this->cfst2_);

  // Clears sort properties.
  vfst1_u.SetProperties(0, kOLabelSorted);
  vfst2_u.SetProperties(0, kILabelSorted);

  const TestFst tfst1_u(vfst1_u);
  const TestFst tfst2_u(vfst2_u);

  const s::FstClass cfst1_u(vfst1_u);
  const s::FstClass cfst2_u(vfst2_u);

  s::VectorFstClass ofst1(Arc::Type());
  const s::FstClass cfst1(tfst1);
  const s::FstClass cfst2(tfst2);
  const s::FstClass cfst3(tfst3);

  s::Compose(cfst1, cfst2, &ofst1);
  EXPECT_TRUE(Equal(cfst3, ofst1));

  s::VectorFstClass ofst2(Arc::Type());
  s::Compose(cfst1_u, cfst2, &ofst2);
  EXPECT_TRUE(Equal(cfst3, ofst2));

  s::VectorFstClass ofst3(Arc::Type());
  Compose(cfst1, cfst2_u, &ofst3);
  EXPECT_TRUE(Equal(cfst3, ofst3));
}

TYPED_TEST_P(ComposeTest, ComposeFstWithEpsilons) {
  using Arc = typename TestFixture::Arc;
  using TestFst = typename TestFixture::TestFst;

  const TestFst tfst4(*this->cfst4_);
  const TestFst tfst5(*this->cfst5_);

  const ComposeFst<Arc> dfst1(tfst4, tfst5);
  EXPECT_TRUE(Verify(dfst1));
  EXPECT_TRUE(Equal(*this->cfst6_, dfst1));

  for (const bool safe : {false, true}) {
    const ComposeFst<Arc> cfst(dfst1, safe);
    EXPECT_TRUE(Verify(cfst));
    EXPECT_TRUE(Equal(*this->cfst6_, cfst));
  }
}

TYPED_TEST_P(ComposeTest, ComposeFstWithOptions) {
  using Arc = typename TestFixture::Arc;
  using TestFst = typename TestFixture::TestFst;
  using M1 = Matcher<TestFst>;
  using M2 = Matcher<ConstFst<Arc>>;

  const TestFst tfst4(*this->cfst4_);
  const ConstFst<Arc> cfst5(*this->cfst5_);

  // Testing with the null composition filter.
  const ComposeFstImplOptions<M1, M2, NullComposeFilter<M1, M2>> opts;
  const ComposeFst<Arc> dfst1(tfst4, cfst5, opts);
  EXPECT_TRUE(Verify(dfst1));
  EXPECT_TRUE(Equal(*this->cfst7_, dfst1));

  for (const bool safe : {false, true}) {
    const ComposeFst<Arc> cfst(dfst1, safe);
    EXPECT_TRUE(Verify(cfst));
    EXPECT_TRUE(Equal(*this->cfst7_, cfst));
  }
}

TYPED_TEST_P(ComposeTest, ComposeFstMatcherTest) {
  using Arc = typename TestFixture::Arc;
  using TestFst = typename TestFixture::TestFst;

  const TestFst tfst1(*this->cfst1_);
  const TestFst tfst2(*this->cfst2_);
  const TestFst tfst8(*this->cfst8_);

  // Creates the ComposeFst on which we are going to use the ComposeFstMatcher.
  const ComposeFst<Arc> cfst(
      tfst1, tfst2,
      ComposeFstOptions<Arc>(CacheOptions(true, 0),
                             new Matcher<Fst<Arc>>(tfst1, MATCH_INPUT),
                             new Matcher<Fst<Arc>>(tfst2, MATCH_INPUT)));
  EXPECT_FALSE(tfst8.Properties(kOLabelSorted, true));
  // Since 'tfst8' is not olabel-sorted, it cannot be matched on during
  // composition, hence 'cfst' is going to be matched on, which is possible
  // only by creating a ComposeFstMatcher since 'cfst' is a ComposeFst.
  const ComposeFst<Arc> ccfst(tfst8, cfst);
  EXPECT_TRUE(Verify(ccfst));
  EXPECT_TRUE(Equal(*this->cfst3_, ccfst));
  // The same test as above but this time explicitly creating the matchers.
  const ComposeFst<Arc> ccmfst(
      tfst8, cfst,
      ComposeFstImplOptions<Matcher<Fst<Arc>>, Matcher<ComposeFst<Arc>>>(
          CacheOptions(true, 0), new Matcher<Fst<Arc>>(tfst8, MATCH_NONE),
          new Matcher<ComposeFst<Arc>>(cfst, MATCH_INPUT)));
  EXPECT_TRUE(Verify(ccmfst));
  EXPECT_TRUE(Equal(*this->cfst3_, ccmfst));
}

TYPED_TEST_P(ComposeTest, CopyTest) {
  using Arc = typename TestFixture::Arc;
  using TestFst = typename TestFixture::TestFst;

  const TestFst tfst4(*this->cfst4_);
  const TestFst tfst5(*this->cfst5_);

  std::unique_ptr<const ComposeFst<Arc>> dfst1(
      new ComposeFst<Arc>(tfst4, tfst5, CacheOptions(true, 0)));
  Verify(*dfst1);
  std::unique_ptr<const ComposeFst<Arc>> dfst2(dfst1->Copy(true));
  dfst1.reset();
  std::unique_ptr<const ComposeFst<Arc>> dfst3(
      new ComposeFst<Arc>(tfst4, tfst5));
  Verify(*dfst2);
  EXPECT_TRUE(Equal(*dfst2, *dfst3));
}

REGISTER_TYPED_TEST_SUITE_P(ComposeTest, ComposeFstWithoutEpsilons,
                            FstClassCompose, ComposeFstWithEpsilons,
                            ComposeFstWithOptions, ComposeFstMatcherTest,
                            CopyTest);

}  // namespace fst

#endif  // OPENFST_TEST_COMPOSE_TEST_H_
