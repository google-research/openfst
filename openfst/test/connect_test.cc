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
// Unit test for Connect.

#include "openfst/lib/connect.h"

#include <memory>
#include <string>
#include <vector>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/equal.h"
#include "openfst/lib/properties.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/lib/verify.h"
#include "openfst/script/connect.h"
#include "openfst/script/equal.h"
#include "openfst/script/fst-class.h"

namespace fst {
namespace {

using Arc = StdArc;
using StateId = Arc::StateId;

class ConnectTest : public testing::Test {
 protected:
  void SetUp() override {
    const std::string path =
        JoinPath(std::string("."),
                       "openfst/test/testdata/connect");
    const std::string connect1_name = JoinPath(path, "c1.fst");
    const std::string connect2_name = JoinPath(path, "c2.fst");
    const std::string connect3_name = JoinPath(path, "c3.fst");

    // Disconnected FST.
    cfst1_.reset(VectorFst<Arc>::Read(connect1_name));
    // cfst2_ = Connect(cfst1_)
    cfst2_.reset(VectorFst<Arc>::Read(connect2_name));
    // cfst3_ = Condense(cfst1_)
    cfst3_.reset(VectorFst<Arc>::Read(connect3_name));
  }

  std::unique_ptr<const VectorFst<Arc>> cfst1_;
  std::unique_ptr<const VectorFst<Arc>> cfst2_;
  std::unique_ptr<const VectorFst<Arc>> cfst3_;
};

TEST_F(ConnectTest, Connect) {
  VectorFst<Arc> nfst;

  VectorFst<Arc> vfst(*cfst1_);
  ASSERT_TRUE(vfst.Properties(kNotAccessible, true));
  ASSERT_TRUE(vfst.Properties(kNotCoAccessible, true));
  Connect(&vfst);
  ASSERT_TRUE(Verify(vfst));
  ASSERT_TRUE(vfst.Properties(kAccessible, true));
  ASSERT_TRUE(vfst.Properties(kCoAccessible, true));
  ASSERT_TRUE(Equal(*cfst2_, vfst));

  ASSERT_TRUE(nfst.Properties(kAccessible, true));
  ASSERT_TRUE(nfst.Properties(kCoAccessible, true));
  Connect(&nfst);
  ASSERT_TRUE(Verify(nfst));
  ASSERT_TRUE(nfst.Properties(kAccessible, true));
  ASSERT_TRUE(nfst.Properties(kCoAccessible, true));
}

TEST_F(ConnectTest, FstClassConnect) {
  namespace s = fst::script;

  s::VectorFstClass cfst1(*cfst1_);
  s::VectorFstClass cfst2(*cfst2_);
  ASSERT_TRUE(cfst1.Properties(kNotAccessible, true));
  ASSERT_TRUE(cfst1.Properties(kNotCoAccessible, true));
  Connect(&cfst1);

  ASSERT_TRUE(cfst1.Properties(kAccessible, true));
  ASSERT_TRUE(cfst1.Properties(kCoAccessible, true));
  ASSERT_TRUE(Equal(cfst2, cfst1));

  s::VectorFstClass nfst(Arc::Type());

  ASSERT_TRUE(nfst.Properties(kAccessible, true));
  ASSERT_TRUE(nfst.Properties(kCoAccessible, true));
  Connect(&nfst);
  ASSERT_TRUE(nfst.Properties(kAccessible, true));
  ASSERT_TRUE(nfst.Properties(kCoAccessible, true));
}

TEST_F(ConnectTest, Condense) {
  VectorFst<Arc> cfst3;
  std::vector<StateId> scc;
  Condense(*cfst1_, &cfst3, &scc);
  ASSERT_TRUE(cfst3.Properties(kAcyclic, false));
  ASSERT_TRUE(cfst3.Properties(kInitialAcyclic, false));
  ASSERT_TRUE(Verify(cfst3));
  ASSERT_TRUE(Equal(*cfst3_, cfst3));

  const VectorFst<Arc> nfst;
  VectorFst<Arc> cfst4;
  Condense(nfst, &cfst4, &scc);
  ASSERT_TRUE(cfst4.Properties(kAcyclic, false));
  ASSERT_TRUE(cfst4.Properties(kInitialAcyclic, false));
  ASSERT_TRUE(Verify(cfst4));
  ASSERT_EQ(cfst4.NumStates(), 0);
}

}  // namespace
}  // namespace fst
