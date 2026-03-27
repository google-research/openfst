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
// Unit test for Reverse.

#include "openfst/lib/reverse.h"

#include <memory>
#include <string>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/equal.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/lib/verify.h"
#include "openfst/script/equal.h"
#include "openfst/script/fst-class.h"
#include "openfst/script/reverse.h"

namespace fst {
namespace {

using Arc = StdArc;

class ReverseTest : public testing::Test {
 protected:
  void SetUp() override {
    const std::string path =
        JoinPath(std::string("."),
                       "openfst/test/testdata/reverse");
    const std::string reverse1_name = JoinPath(path, "r1.fst");
    const std::string reverse2_name = JoinPath(path, "r2.fst");
    const std::string reverse3_name = JoinPath(path, "r3.fst");
    const std::string reverse4_name = JoinPath(path, "r4.fst");

    rfst1_.reset(VectorFst<Arc>::Read(reverse1_name));
    // rfst2_ = Reverse(rfst1_)
    rfst2_.reset(VectorFst<Arc>::Read(reverse2_name));
    // Final-acyclic test example.
    rfst3_.reset(VectorFst<Arc>::Read(reverse3_name));
    // rfst4_ = Reverse(rfst3_, false)
    rfst4_.reset(VectorFst<Arc>::Read(reverse4_name));
  }

  std::unique_ptr<const VectorFst<Arc>> rfst1_;
  std::unique_ptr<const VectorFst<Arc>> rfst2_;
  std::unique_ptr<const VectorFst<Arc>> rfst3_;
  std::unique_ptr<const VectorFst<Arc>> rfst4_;
};

TEST_F(ReverseTest, Reverse) {
  VectorFst<Arc> nfst;

  // Container for result (assumes weights self-reverse).
  VectorFst<Arc> vfst1;

  // Container for result (does not assume weights self-reverse).
  VectorFst<ReverseArc<Arc>> vfst2;

  // rnfst = Reverse(nfst); (assumes weights self-reverse).
  VectorFst<Arc> rnfst;
  rnfst.AddState();
  rnfst.SetStart(0);

  Reverse(*rfst1_, &vfst1);
  ASSERT_TRUE(Verify(vfst1));
  ASSERT_TRUE(Equal(*rfst2_, vfst1));

  Reverse(nfst, &vfst1);
  ASSERT_TRUE(Verify(vfst1));
  ASSERT_TRUE(Equal(rnfst, vfst1));

  Reverse(*rfst1_, &vfst2);
  ASSERT_TRUE(Verify(vfst2));

  // Reverse with optional superinitial state.
  // (1) superinitial needed
  Reverse(*rfst1_, &vfst1, false);
  ASSERT_TRUE(Verify(vfst1));
  ASSERT_TRUE(Equal(*rfst2_, vfst1));
  // (2) superinitial not needed
  Reverse(*rfst3_, &vfst1, false);
  ASSERT_TRUE(Verify(vfst1));
  ASSERT_TRUE(Equal(*rfst4_, vfst1));
}

TEST_F(ReverseTest, ReverseFstClass) {
  namespace s = fst::script;

  s::FstClass rfst1(*rfst1_);
  s::FstClass rfst2(*rfst2_);
  s::FstClass rfst3(*rfst3_);
  s::FstClass rfst4(*rfst4_);

  std::unique_ptr<s::MutableFstClass> container(
      new s::VectorFstClass(rfst1.ArcType()));
  ASSERT_TRUE(container);
  s::Reverse(rfst1, container.get());
  ASSERT_TRUE(Equal(*container, rfst2));

  container = std::make_unique<s::VectorFstClass>(rfst1.ArcType());
  script::Reverse(rfst1, container.get(), false);
  ASSERT_TRUE(Equal(*container, rfst2));

  container = std::make_unique<s::VectorFstClass>(rfst3.ArcType());
  script::Reverse(rfst3, container.get(), false);
  ASSERT_TRUE(Equal(*container, rfst4));
}

}  // namespace
}  // namespace fst
